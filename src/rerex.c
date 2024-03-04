// Copyright 2020-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "rerex/rerex.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static const char cmin = 0x20; // Inclusive minimum normal character
static const char cmax = 0x7E; // Inclusive maximum normal character

const char*
rerex_strerror(const RerexStatus status)
{
  static const char* status_strings[] = {
    "Success",
    "Expected a regular character",
    "Expected a character in a set",
    "Expected ']'",
    "Expected ')'",
    "Expected a special character (one of \"()*+-?[]^|\")",
    "Unexpected special character",
    "Unexpected end of input",
    "Range is out of order",
    "Failed to allocate memory",
  };

  return ((unsigned)status <= (unsigned)REREX_NO_MEMORY)
           ? status_strings[status]
           : "Unknown error";
}

/* State */

// The ID for a state, which is an index into the state array
typedef size_t StateIndex;

// A code point (currently only 8-bit ASCII but we use the space anyway)
typedef int Codepoint;

// Special type for states with only epsilon out arcs
typedef enum {
  REREX_MATCH = 0xE000, ///< Matching state, no out arcs
  REREX_SPLIT = 0xE001, ///< Splitting state, one or two out arcs
} StateType;

/* A state in an NFA.

   A state in Thompson's NFA can have either a single character-labeled
   transition to another state, or up to two unlabeled epsilon transitions to
   other states.  There is both a minimum and maximum label for supporting
   character ranges.  So, either `min` and `max` are ASCII characters that are
   the label of an arc to next1 (and next2 is null), or `min` is a special
   StateType and next1 and/or next2 may be set to successor states.
*/
typedef struct {
  StateIndex next1; ///< Head of first out arc (or NULL)
  StateIndex next2; ///< Head of second out arc (or NULL)
  Codepoint  min;   ///< Special type, or inclusive min label for next1
  Codepoint  max;   ///< Inclusive max label for next2
} State;

// Sentinel value for an unset state index
static const StateIndex NO_STATE = 0U;

// Create a match (end) state with no successors
static State
match_state(void)
{
  const State s = {NO_STATE, NO_STATE, REREX_MATCH, 0};
  return s;
}

// Create a split state with at most two successors
static State
split_state(const StateIndex next1, const StateIndex next2)
{
  const State s = {next1, next2, REREX_SPLIT, 0};
  return s;
}

// Create a labeled state with one successor reached by a character arc
static State
range_state(const char min, const char max, const StateIndex next)
{
  const State s = {next, NO_STATE, min, max};
  return s;
}

/* Array of states.

   States are stored in a flat array to reduce memory fragmentation, and for
   easy memory management since the automata graph may be cyclic.  This simple
   implementation calls realloc() for every state, which isn't terribly
   efficient, but works well enough.  Note that state addresses therefore change
   during compilation, so states are generally referred to by their index, and
   not by pointer.  Conveniently, using indices is also useful during matching
   for storing auxiliary information about states.
*/
typedef struct {
  State* states;
  size_t n_states;
} StateArray;

// Append a new state to the end of the state array
static StateIndex
add_state(StateArray* const array, const State state)
{
  const size_t new_index    = array->n_states;
  const size_t new_n_states = new_index + 1U;
  const size_t new_size     = new_n_states * sizeof(State);
  State* const new_states   = (State*)realloc(array->states, new_size);

  if (new_states) {
    new_states[new_index] = state;
    array->states         = new_states;
    array->n_states       = new_n_states;
  }

  return new_states ? new_index : NO_STATE;
}

/* Automata.

   This is a lightweight description of an NFA fragment.  The states are stored
   elsewhere, this is just used to provide a facade to conceptually work with
   automata for high-level operations.
*/
typedef struct {
  StateIndex start;
  StateIndex end;
} Automata;

// Simple utility function for making an automata in an expression
static Automata
make_automata(const StateIndex start, const StateIndex end)
{
  Automata result = {start, end};
  return result;
}

// Return whether `nfa` has only two simple states (used for optimizations)
static inline bool
is_trivial(const StateArray* const states, const Automata nfa)
{
  return (states->states[nfa.start].min < REREX_MATCH &&
          states->states[nfa.start].next1 == nfa.end);
}

// Kleene's Star of an NFA
static Automata
star(StateArray* const states, const Automata nfa)
{
  const StateIndex end   = add_state(states, match_state());
  const StateIndex start = add_state(states, split_state(nfa.start, end));

  states->states[nfa.end] = split_state(nfa.start, end);

  return make_automata(start, end);
}

// Zero-or-one of an NFA
static Automata
question(StateArray* const states, const Automata nfa)
{
  const StateIndex start = add_state(states, split_state(nfa.start, nfa.end));

  return make_automata(start, nfa.end);
}

// One-or-more of an NFA
static Automata
plus(StateArray* const states, const Automata nfa)
{
  const StateIndex end = add_state(states, match_state());

  states->states[nfa.end] = split_state(nfa.start, end);

  return make_automata(nfa.start, end);
}

// Concatenation of two NFAs
static Automata
concatenate(StateArray* const states, const Automata a, const Automata b)
{
  if (is_trivial(states, a)) {
    // Optimization: link a's start directly to b's start (drop a's end)
    states->states[a.start].next1 = b.start;
  } else {
    states->states[a.end] = split_state(b.start, NO_STATE);
  }

  return make_automata(a.start, b.end);
}

// Alternation (OR) of two NFAs
static Automata
alternate(StateArray* const states, const Automata a, const Automata b)
{
  const StateIndex split = add_state(states, split_state(a.start, b.start));

  if (is_trivial(states, a)) {
    // Optimization: link a's start directly to b's end (drop a's end)
    states->states[a.start].next1 = b.end;
    return make_automata(split, b.end);
  }

  if (is_trivial(states, b)) {
    // Optimization: link b's start directly to a's end (drop b's end)
    states->states[b.start].next1 = a.end;
    return make_automata(split, a.end);
  }

  const StateIndex end = add_state(states, match_state());

  states->states[a.end] = split_state(end, NO_STATE);
  states->states[b.end] = split_state(end, NO_STATE);

  return make_automata(split, end);
}

/* Parser input.

   The parser reads from a string one character at a time, though it would be
   simple to change this to read from any stream.  All reading is done by three
   operations: peek, peekahead, and eat.
*/
typedef struct {
  const char* const str;
  size_t            offset;
} Input;

// Return the next character in the input without consuming it
static inline char
peek(Input* const input)
{
  return input->str[input->offset];
}

// Return the next-next character in the input without consuming any
static inline char
peekahead(Input* const input)
{
  // Unfortunately we need 2-char lookahead for the ambiguity of '-' in sets
  return input->str[input->offset + 1];
}

// Consume and return the next character in the input
static inline char
eat(Input* const input)
{
  return input->str[input->offset++];
}

// Forward declaration for read_expr because it is called recursively
static RerexStatus
read_expr(Input* input, StateArray* states, Automata* out);

// DOT      ::= '.'
// OPERATOR ::= '*' | '+' | '?'
// SPECIAL  ::= DOT | OPERATOR | '(' | ')' | '[' | ']' | '^' | '{' | '|' | '}'
static bool
is_special(const char c)
{
  return (c == '(') || (c == ')') || (c == '*') || (c == '+') || (c == '.') ||
         (c == '?') || (c == '[') || (c == ']') || (c == '^') || (c == '{') ||
         (c == '|') || (c == '}');
}

// DOT ::= '.'
static RerexStatus
read_dot(Input* const input, StateArray* const states, Automata* const out)
{
  assert(peek(input) == '.');
  eat(input);

  const StateIndex end   = add_state(states, match_state());
  const StateIndex start = add_state(states, range_state(cmin, cmax, end));

  *out = make_automata(start, end);

  return REREX_SUCCESS;
}

// ESCAPE ::= '\' SPECIAL
static RerexStatus
read_escape(Input* const input, char* const out)
{
  assert(peek(input) == '\\');
  eat(input);

  const char c = peek(input);
  if (is_special(c) || c == '-') {
    *out = eat(input);
    return REREX_SUCCESS;
  }

  return REREX_EXPECTED_SPECIAL;
}

// CHAR ::= ESCAPE | [#x20-#x7E] - SPECIAL
static RerexStatus
read_char(Input* const input, char* const out)
{
  const char c = peek(input);

  if (c == '\0') {
    return REREX_UNEXPECTED_END;
  }

  if (c == '\\') {
    return read_escape(input, out);
  }

  if (is_special(c)) {
    return REREX_UNEXPECTED_SPECIAL;
  }

  if (c >= cmin && c <= cmax) {
    *out = eat(input);
    return REREX_SUCCESS;
  }

  return REREX_EXPECTED_CHAR;
}

// ELEMENT ::= ([#x20-#x7E] - ']') | ('\' ']')
static RerexStatus
read_element(Input* const input, char* const out)
{
  const char c = peek(input);

  if (c == '\0') {
    return REREX_UNEXPECTED_END;
  }

  if (c == ']') {
    return REREX_UNEXPECTED_SPECIAL;
  }

  if (c == '\\') {
    eat(input);
    if (peek(input) != ']') {
      return REREX_EXPECTED_RBRACKET;
    }

    *out = eat(input);
    return REREX_SUCCESS;
  }

  if (c >= cmin && c <= cmax) {
    *out = eat(input);
    return REREX_SUCCESS;
  }

  return REREX_EXPECTED_ELEMENT;
}

// Range ::= ELEMENT | ELEMENT '-' ELEMENT
static RerexStatus
read_range(Input* const      input,
           StateArray* const states,
           const bool        negated,
           Automata* const   out)
{
  RerexStatus st  = REREX_SUCCESS;
  char        min = 0;

  // Read the first (or only) character
  if ((st = read_element(input, &min))) {
    return st;
  }

  char max = min;
  if (peek(input) == '-') {
    // The '-' is only special if there's a following element
    if (peekahead(input) != ']') {
      eat(input);
      if ((st = read_element(input, &max))) {
        return st;
      }
    }
  }

  if (max < min) {
    return REREX_UNORDERED_RANGE;
  }

  const StateIndex end = add_state(states, match_state());
  if (negated) {
    const char       emin = (char)(min - 1);
    const char       emax = (char)(max + 1);
    const StateIndex low  = add_state(states, range_state(cmin, emin, end));
    const StateIndex high = add_state(states, range_state(emax, cmax, end));
    const StateIndex fork = add_state(states, split_state(low, high));

    *out = make_automata(fork, end);
  } else {
    const StateIndex start = add_state(states, range_state(min, max, end));

    *out = make_automata(start, end);
  }

  return st;
}

// Set ::= Range | Range Set
static RerexStatus
read_set(Input* const input, StateArray* const states, Automata* const out)
{
  RerexStatus st      = REREX_SUCCESS;
  bool        negated = false;

  if (peek(input) == '^') {
    eat(input);
    negated = true;
  }

  Automata nfa = {NO_STATE, NO_STATE};
  if ((st = read_range(input, states, negated, &nfa))) {
    return st;
  }

  while (peek(input) != ']') {
    Automata range_nfa = {NO_STATE, NO_STATE};
    if ((st = read_range(input, states, negated, &range_nfa))) {
      return st;
    }

    nfa = alternate(states, nfa, range_nfa);
  }

  *out = nfa;
  return st;
}

// Atom ::= CHAR | DOT | '(' Expr ')' | '[' Set ']'
static RerexStatus
read_atom(Input* const input, StateArray* const states, Automata* const out)
{
  RerexStatus st = REREX_SUCCESS;
  char        c  = peek(input);

  if (c == '(') {
    eat(input);
    if ((st = read_expr(input, states, out))) {
      return st;
    }

    if (peek(input) != ')') {
      return REREX_EXPECTED_RPAREN;
    }

    eat(input);
    return st;
  }

  if (c == '.') {
    return read_dot(input, states, out);
  }

  if (c == '[') {
    eat(input);
    if ((st = read_set(input, states, out))) {
      return st;
    }

    eat(input);
    return st;
  }

  if ((st = read_char(input, &c))) {
    return st;
  }

  const StateIndex end   = add_state(states, match_state());
  const StateIndex start = add_state(states, range_state(c, c, end));

  *out = make_automata(start, end);

  return st;
}

// OPERATOR ::= '*' | '+' | '?'
// Factor   ::= Atom | Atom OPERATOR
static RerexStatus
read_factor(Input* const input, StateArray* const states, Automata* const out)
{
  RerexStatus st       = REREX_SUCCESS;
  Automata    atom_nfa = {NO_STATE, NO_STATE};

  if (!(st = read_atom(input, states, &atom_nfa))) {
    const char c = peek(input);
    if (c == '*') {
      eat(input);
      *out = star(states, atom_nfa);
    } else if (c == '+') {
      eat(input);
      *out = plus(states, atom_nfa);
    } else if (c == '?') {
      eat(input);
      *out = question(states, atom_nfa);
    } else {
      *out = atom_nfa;
    }
  }

  return st;
}

// Term ::= Factor | Factor Term
static RerexStatus
read_term(Input* const input, StateArray* const states, Automata* const out)
{
  RerexStatus st         = REREX_SUCCESS;
  Automata    factor_nfa = {NO_STATE, NO_STATE};
  Automata    term_nfa   = {NO_STATE, NO_STATE};

  if (!(st = read_factor(input, states, &factor_nfa))) {
    const char c = peek(input);
    if (c == '\0' || c == ')' || c == '|') {
      *out = factor_nfa;
    } else if (!(st = read_term(input, states, &term_nfa))) {
      *out = concatenate(states, factor_nfa, term_nfa);
    }
  }

  return st;
}

// Expr ::= Term | Term '|' Expr
static RerexStatus
read_expr(Input* const input, StateArray* const states, Automata* const out)
{
  RerexStatus st       = REREX_SUCCESS;
  Automata    term_nfa = {NO_STATE, NO_STATE};
  Automata    expr_nfa = {NO_STATE, NO_STATE};

  if ((st = read_term(input, states, &term_nfa))) {
    return st;
  }

  if (peek(input) == '|') {
    eat(input);
    if (!(st = read_expr(input, states, &expr_nfa))) {
      *out = alternate(states, term_nfa, expr_nfa);
    }
  } else {
    *out = term_nfa;
  }

  return st;
}

/* Pattern.

   A pattern is simply an array of states and an index to the start state.  The
   end state(s) are known because they have type REREX_MATCH.  A pattern is
   immutable after construction, the matcher does not modify it.
*/
struct RerexPatternImpl {
  StateArray states;
  StateIndex start;
};

void
rerex_free_pattern(RerexPattern* const regexp)
{
  free(regexp->states.states);
  free(regexp);
}

RerexStatus
rerex_compile(const char* const    pattern,
              size_t* const        end,
              RerexPattern** const out)
{
  Input      input  = {pattern, 0};
  Automata   nfa    = {NO_STATE, NO_STATE};
  StateArray states = {NULL, 0};

  // Add null state so that no actual state has NO_STATE as an ID
  add_state(&states, split_state(NO_STATE, NO_STATE));

  RerexStatus st = states.states ? REREX_SUCCESS : REREX_NO_MEMORY;
  if (!st) {
    // Read the expression, building the NFA and its states array
    st   = read_expr(&input, &states, &nfa);
    *end = input.offset;
  }

  // "Return" a newly allocated pattern
  if (!st && (*out = (RerexPattern*)malloc(sizeof(RerexPattern)))) {
    (*out)->states = states;
    (*out)->start  = nfa.start;
    return REREX_SUCCESS;
  }

  free(states.states);
  return st;
}

/* Matcher */

typedef struct {
  StateIndex* indices;   // Array of state indices
  size_t      n_indices; // Number of elements in indices
} IndexList;

/* Matcher.

   The matcher tracks active states by keeping two lists of indices: one for
   the current iteration, and one for the next.  A separate array, keyed by
   state index, stores the number of the last iteration the state was entered
   in.  This makes it simple and fast to check if a state has already been
   entered in the current iteration, avoiding the need to search the active
   list for every entered state.
*/
struct RerexMatcherImpl {
  const RerexPattern* regexp;      // Pattern to match against
  IndexList           active[2];   // Two lists of active states
  size_t*             last_active; // Last iteration a state was active
};

RerexMatcher*
rerex_new_matcher(const RerexPattern* const regexp)
{
  const size_t        n_states = regexp->states.n_states;
  RerexMatcher* const m        = (RerexMatcher*)calloc(1, sizeof(RerexMatcher));

  if (m) {
    m->regexp            = regexp;
    m->active[0].indices = (StateIndex*)calloc(n_states, sizeof(StateIndex));
    m->active[1].indices = (StateIndex*)calloc(n_states, sizeof(StateIndex));
    m->last_active       = (size_t*)calloc(n_states, sizeof(size_t));
  }

  return m;
}

void
rerex_free_matcher(RerexMatcher* const matcher)
{
  if (matcher) {
    free(matcher->last_active);
    free(matcher->active[1].indices);
    free(matcher->active[0].indices);
    free(matcher);
  }
}

// Add `s` and any epsilon successors to the active list
static void
enter_state(RerexMatcher* const matcher,
            const size_t        step,
            IndexList* const    list,
            const StateIndex    s)
{
  const StateArray* const states = &matcher->regexp->states;

  if (s && matcher->last_active[s] != step) {
    matcher->last_active[s] = step;

    const State* const state = &states->states[s];
    if (state->min == REREX_SPLIT) {
      enter_state(matcher, step, list, state->next1);
      enter_state(matcher, step, list, state->next2);
    } else {
      list->indices[list->n_indices++] = s;
    }
  }
}

bool
rerex_match(RerexMatcher* const matcher, const char* const string)
{
  const StateArray* const states = &matcher->regexp->states;

  // Reset matcher to a consistent initial state
  matcher->active[0].n_indices = 0;
  matcher->active[1].n_indices = 0;
  for (size_t i = 0; i < states->n_states; ++i) {
    matcher->last_active[i] = SIZE_MAX;
  }

  // Enter start state
  enter_state(matcher, 0, &matcher->active[0], matcher->regexp->start);

  // Tick the matcher for every input character
  bool phase = false;
  for (size_t i = 0; string[i]; ++i) {
    const char       c         = string[i];
    IndexList* const list      = &matcher->active[phase];
    IndexList* const next_list = &matcher->active[!phase];

    // Add successor states to the next iteration's list
    next_list->n_indices = 0;
    for (size_t j = 0; j < list->n_indices; ++j) {
      const State* const state = &states->states[list->indices[j]];
      if (state->min <= c && c <= state->max) {
        enter_state(matcher, i + 1, next_list, state->next1);
      }
    }

    // Flip phase to swap active lists
    phase = !phase;
  }

  // Check if match state is entered in the end
  const IndexList* const active = &matcher->active[phase];
  for (size_t i = 0; i < active->n_indices; ++i) {
    if (states->states[active->indices[i]].min == REREX_MATCH) {
      return true;
    }
  }

  return false;
}
