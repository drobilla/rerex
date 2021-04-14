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

// Tests that regex syntax errors are reported correctly

#undef NDEBUG

#include "rerex/rerex.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  RerexStatus status;
  unsigned    offset;
  const char* pattern;
} BadSyntaxTestCase;

static const BadSyntaxTestCase syntax_tests[] = {
  {REREX_EXPECTED_CHAR, 1, "a\b"},
  {REREX_EXPECTED_CHAR, 1, "a\x7F"},
  {REREX_EXPECTED_ELEMENT, 1, "[\b]"},
  {REREX_EXPECTED_ELEMENT, 1, "[\x7F]"},
  {REREX_EXPECTED_ELEMENT, 2, "[a\b]"},
  {REREX_EXPECTED_ELEMENT, 2, "[a\x7F]"},
  {REREX_EXPECTED_ELEMENT, 3, "[a-\b]"},
  {REREX_EXPECTED_ELEMENT, 3, "[a-\x7F]"},
  {REREX_EXPECTED_RBRACKET, 2, "[\\n]"},
  {REREX_EXPECTED_RPAREN, 2, "(a"},
  {REREX_EXPECTED_SPECIAL, 1, "\\n"},
  {REREX_UNEXPECTED_END, 1, "("},
  {REREX_UNEXPECTED_END, 1, "["},
  {REREX_UNEXPECTED_END, 2, "[a"},
  {REREX_UNEXPECTED_END, 3, "(a|"},
  {REREX_UNEXPECTED_END, 3, "[a-"},
  {REREX_UNEXPECTED_END, 4, "[a-z"},
  {REREX_UNEXPECTED_END, 4, "[a-z"},
  {REREX_UNEXPECTED_SPECIAL, 0, "{"},
  {REREX_UNEXPECTED_SPECIAL, 0, "}"},
  {REREX_UNEXPECTED_SPECIAL, 0, "?"},
  {REREX_UNEXPECTED_SPECIAL, 1, "[]]"},
  {REREX_UNEXPECTED_SPECIAL, 2, "a|?"},
  {REREX_UNEXPECTED_SPECIAL, 3, "(a|?)"},
  {REREX_UNEXPECTED_SPECIAL, 3, "[[]]"},
  {REREX_UNEXPECTED_SPECIAL, 3, "[a]]"},
  {REREX_UNEXPECTED_SPECIAL, 4, "[A-]]"},
  {REREX_UNEXPECTED_SPECIAL, 4, "[a[]]"},
  {REREX_UNEXPECTED_SPECIAL, 5, "[A-[]]"},
  {REREX_UNORDERED_RANGE, 4, "[z-a]"},
};

static void
test_status(void)
{
  assert(!strcmp(rerex_strerror((RerexStatus)INT32_MAX), "Unknown error"));
}

static void
test_syntax(void)
{
  const size_t n_tests = sizeof(syntax_tests) / sizeof(*syntax_tests);

  for (size_t i = 0; i < n_tests; ++i) {
    const char* const regexp = syntax_tests[i].pattern;
    const size_t      offset = syntax_tests[i].offset;
    const RerexStatus status = syntax_tests[i].status;

    RerexPattern*     pattern = NULL;
    size_t            end     = 0;
    const RerexStatus st      = rerex_compile(regexp, &end, &pattern);

    assert(st == status);
    assert(!pattern);
    assert(strcmp(rerex_strerror(st), rerex_strerror(REREX_SUCCESS)));
    assert(end == offset);
  }
}

int
main(void)
{
  test_status();
  test_syntax();
  return 0;
}
