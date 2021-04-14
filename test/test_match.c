/*
  Copyright 2020 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#undef NDEBUG

#include "rerex/rerex.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uintptr_t   match;   ///< Boolean, true if text should match
  const char* pattern; ///< Regular expression
  const char* text;    ///< Text to match against pattern
} MatchTestCase;

static const MatchTestCase match_tests[] = {
  {1, "\\(", "("},
  {1, "\\)", ")"},
  {1, "\\*", "*"},
  {1, "\\+", "+"},
  {1, "\\-", "-"},
  {1, "\\.", "."},
  {1, "\\?", "?"},
  {1, "\\[", "["},
  {1, "\\]", "]"},
  {1, "\\^", "^"},
  {1, "\\|", "|"},
  {0, ".", ""},
  {1, ".", "a"},
  {0, ".", "aa"},
  {0, "..", ""},
  {0, "..", "a"},
  {1, "..", "aa"},
  {1, ".*", ""},
  {1, ".*", "a"},
  {1, ".*", "aa"},
  {0, ".+", ""},
  {1, ".+", "a"},
  {1, ".+", "aa"},
  {1, ".?", ""},
  {1, ".?", "a"},
  {0, ".?", "aa"},
  {1, "a*", ""},
  {1, "a*", "a"},
  {1, "a*", "aa"},
  {0, "a*", "b"},
  {0, "a+", ""},
  {1, "a+", "a"},
  {1, "a+", "aa"},
  {0, "a+", "b"},
  {1, "a?", ""},
  {1, "a?", "a"},
  {0, "a?", "aa"},
  {0, "a?", "b"},
  {0, "[.]", "a"},
  {1, "[.]", "."},
  {0, "[\\]]", "a"},
  {1, "[\\]]", "]"},
  {0, "[b]", "a"},
  {1, "[b]", "b"},
  {0, "[b]", "c"},
  {0, "[bc]", "a"},
  {1, "[bc]", "b"},
  {1, "[bc]", "c"},
  {0, "[bc]", "d"},
  {0, "[bcd]", "a"},
  {1, "[bcd]", "b"},
  {1, "[bcd]", "c"},
  {1, "[bcd]", "d"},
  {0, "[bcd]", "e"},
  {0, "[b-d]", "a"},
  {1, "[b-d]", "b"},
  {1, "[b-d]", "d"},
  {0, "[b-d]", "e"},
  {1, "[^b-d]", "a"},
  {0, "[^b-d]", "b"},
  {0, "[^b-d]", "d"},
  {1, "[^b-d]", "e"},
  {0, "[^ -/]", "\t"},
  {1, "[^ -/]", "0"},
  {1, "[^{-~]", "z"},
  {0, "[^{-~]", "~"},
  {0, "[A-Za-z]", "5"},
  {1, "[A-Za-z]", "m"},
  {1, "[A-Za-z]", "M"},
  {0, "[A-Za-z]", "~"},
  {0, "[+-]", "*"},
  {1, "[+-]", "+"},
  {0, "[+-]", ","},
  {1, "[+-]", "-"},
  {0, "[+-]", "."},
  {1, "[b-d]*", ""},
  {0, "[b-d]*", "a"},
  {1, "[b-d]*", "b"},
  {1, "[b-d]*", "c"},
  {1, "[b-d]*", "cc"},
  {1, "[b-d]*", "d"},
  {0, "[b-d]*", "e"},
  {0, "[b-d]+", ""},
  {0, "[b-d]+", "a"},
  {1, "[b-d]+", "b"},
  {1, "[b-d]+", "c"},
  {1, "[b-d]+", "cc"},
  {1, "[b-d]+", "d"},
  {0, "[b-d]+", "e"},
  {1, "[b-d]?", ""},
  {0, "[b-d]?", "a"},
  {1, "[b-d]?", "b"},
  {1, "[b-d]?", "c"},
  {0, "[b-d]?", "cc"},
  {1, "[b-d]?", "d"},
  {0, "[b-d]?", "e"},
  {1, "h(e|a)llo", "hello"},
  {1, "h(e|a)llo", "hallo"},
  {1, "h(e|a)+llo", "haello"},
  {1, "h(e|a)*llo", "hllo"},
  {1, "h(e|a)?llo", "hllo"},
  {1, "h(e|a)?llo", "hello"},
  {1, "h(e|a)*llo*", "haeeeallooo"},
  {1, "(ab|a)(bc|c)", "abc"},
  {0, "(ab|a)(bc|c)", "acb"},
  {1, "(ab)c|abc", "abc"},
  {0, "(ab)c|abc", "ab"},
  {1, "(a*)(b?)(b+)", "aaabbbb"},
  {0, "(a*)(b?)(b+)", "aaaa"},
  {1, "((a|a)|a)", "a"},
  {0, "((a|a)|a)", "aa"},
  {1, "(a*)(a|aa)", "aaaa"},
  {0, "(a*)(a|aa)", "b"},
  {1, "a(b)|c(d)|a(e)f", "aef"},
  {0, "a(b)|c(d)|a(e)f", "adf"},
  {1, "(a|b)c|a(b|c)", "ac"},
  {0, "(a|b)c|a(b|c)", "acc"},
  {1, "(a|b)c|a(b|c)", "ab"},
  {0, "(a|b)c|a(b|c)", "acb"},
  {1, "(a|b)*c|(a|ab)*c", "abc"},
  {0, "(a|b)*c|(a|ab)*c", "bbbcabbbc"},
  {1, "a?(ab|ba)ab", "abab"},
  {0, "a?(ab|ba)ab", "aaabab"},
  {1, "(aa|aaa)*|(a|aaaaa)", "aa"},
  {1, "(a)(b)(c)", "abc"},
  {1, "((((((((((x))))))))))", "x"},
  {1, "((((((((((x))))))))))*", "xx"},
  {1, "a?(ab|ba)*", "ababababababababababababababababa"},
  {1, "a*a*a*a*a*b", "aaaaaaaab"},
  {1, "abc", "abc"},
  {1, "ab*c", "abc"},
  {1, "ab*bc", "abbc"},
  {1, "ab*bc", "abbbbc"},
  {1, "ab+bc", "abbc"},
  {1, "ab+bc", "abbbbc"},
  {1, "ab?bc", "abbc"},
  {1, "ab?bc", "abc"},
  {1, "ab|cd", "ab"},
  {1, "(a)b(c)", "abc"},
  {1, "a*", "aaa"},
  {1, "(a+|b)*", "ab"},
  {1, "(a+|b)+", "ab"},
  {1, "a|b|c|d|e", "e"},
  {1, "(a|b|c|d|e)f", "ef"},
  {1, "abcd*efg", "abcdefg"},
  {1, "(ab|ab*)bc", "abc"},
  {1, "(ab|a)b*c", "abc"},
  {1, "((a)(b)c)(d)", "abcd"},
  {1, "(a|ab)(c|bcd)", "abcd"},
  {1, "(a|ab)(bcd|c)", "abcd"},
  {1, "(ab|a)(c|bcd)", "abcd"},
  {1, "(ab|a)(bcd|c)", "abcd"},
  {1, "((a|ab)(c|bcd))(d*)", "abcd"},
  {1, "((a|ab)(bcd|c))(d*)", "abcd"},
  {1, "((ab|a)(c|bcd))(d*)", "abcd"},
  {1, "((ab|a)(bcd|c))(d*)", "abcd"},
  {1, "(a|ab)((c|bcd)(d*))", "abcd"},
  {1, "(a|ab)((bcd|c)(d*))", "abcd"},
  {1, "(ab|a)((c|bcd)(d*))", "abcd"},
  {1, "(ab|a)((bcd|c)(d*))", "abcd"},
  {1, "(a*)(b|abc)", "abc"},
  {1, "(a*)(abc|b)", "abc"},
  {1, "((a*)(b|abc))(c*)", "abc"},
  {1, "((a*)(abc|b))(c*)", "abc"},
  {1, "(a*)((b|abc))(c*)", "abc"},
  {1, "(a*)((abc|b)(c*))", "abc"},
  {1, "(a*)(b|abc)", "abc"},
  {1, "(a*)(abc|b)", "abc"},
  {1, "((a*)(b|abc))(c*)", "abc"},
  {1, "((a*)(abc|b))(c*)", "abc"},
  {1, "(a*)((b|abc)(c*))", "abc"},
  {1, "(a*)((abc|b)(c*))", "abc"},
  {1, "(a|ab)", "ab"},
  {1, "(ab|a)", "ab"},
  {1, "(a|ab)(b*)", "ab"},
  {1, "(ab|a)(b*)", "ab"},
  {1, "(a|b)*c|(a|ab)*c", "abc"},
};

int
main(void)
{
  const size_t n_tests = sizeof(match_tests) / sizeof(*match_tests);

  for (size_t i = 0; i < n_tests; ++i) {
    const char* const regexp       = match_tests[i].pattern;
    const char* const text         = match_tests[i].text;
    const bool        should_match = match_tests[i].match;

    RerexPattern*     pattern = NULL;
    size_t            end     = 0;
    const RerexStatus st      = rerex_compile(regexp, &end, &pattern);

    assert(!st);

    RerexMatcher* const matcher = rerex_new_matcher(pattern);
    const bool          matches = rerex_match(matcher, text);

    assert(matches == should_match);

    rerex_free_matcher(matcher);
    rerex_free_pattern(pattern);
  }

  return 0;
}
