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

/*
  Tests more realistic patterns and using a matcher multiple times.

  The patterns here were written by myself and may have bugs.  XSD datatypes
  unfortunately don't have canonical patterns, though sometimes they
  infuriatingly say that they are constrained by a pattern... which is not
  given.  XML is terrible, don't use it, but the XSD datatypes are more
  generally useful.  Some patterns here slightly fuzzy (for example, with
  dates), so matching does not necessary mean the value itself is valid.

  These don't have much to do with Rerex itself, but it's something I needed it
  for, and having some more realistic tests is nice.  These also test reusing a
  matcher, unlike the basic match tests.
*/

#undef NDEBUG

#include "rerex/rerex.h"

#include <assert.h>
#include <stddef.h>

static void
test_pattern(const char* const regexp,
             const char* const matching[],
             const char* const nonmatching[])
{
  RerexPattern*     pattern = NULL;
  size_t            end     = 0;
  const RerexStatus st      = rerex_compile(regexp, &end, &pattern);

  assert(!st);

  RerexMatcher* const matcher = rerex_new_matcher(pattern);

  for (const char* const* m = matching; *m; ++m) {
    assert(rerex_match(matcher, *m));
  }

  for (const char* const* n = nonmatching; *n; ++n) {
    assert(!rerex_match(matcher, *n));
  }

  rerex_free_matcher(matcher);
  rerex_free_pattern(pattern);
}

static void
test_base64Binary(void)
{
  static const char* const regexp =
    "(([A-Za-z0-9+/] *[A-Za-z0-9+/] *[A-Za-z0-9+/] *[A-Za-z0-9+/] *)*" //
    "(([A-Za-z0-9+/] *[A-Za-z0-9+/] *[A-Za-z0-9+/] *[A-Za-z0-9+/])|"   //
    "([A-Za-z0-9+/] *[A-Za-z0-9+/] *[AEIMQUYcgkosw048] *=)|"           //
    "([A-Za-z0-9+/] *[AQgw] *= *=)))?";

  static const char* const good[] = {
    "0FB8",
    "0fb8",
    "0 FB8 0F+9",
    "0F+40A8=",
    "0F+40A==",
    "",
    NULL,
  };

  static const char* const bad[] = {
    " 0FB8",
    "0FB8 ",
    " 0FB8 ",
    "FB8",
    "==0F",
    "0F+40A9=",
    "0F+40B==",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_boolean(void)
{
  static const char* const regexp = "(true|false|0|1)";
  static const char* const good[] = {"true", "false", "0", "1", NULL};
  static const char* const bad[]  = {"TRUE", "T", "", NULL};

  test_pattern(regexp, good, bad);
}

static void
test_date(void)
{
  static const char* const regexp = //
    "-?[0-9][0-9][0-9][0-9][0-9]*"  //
    "-(0[1-9]|1[0-2])"              //
    "-(0[1-9]|[12][0-9]|3[01])"     //
    "(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "2004-04-12",
    "-0045-01-01",
    "12004-04-12",
    "2004-04-12-05:00",
    "2004-04-12Z",
    "2001-10-26",
    "2001-10-26+02:00",
    "2001-10-26Z",
    "2001-10-26+00:00",
    "-2001-10-26",
    "-20000-04-01",
    NULL,
  };

  static const char* const bad[] = {
    "99-04-12",
    "2004-4-2",
    "2004/04/02",
    "04-12-2004",
    // "2004-04-31", // Not quite that clever...
    "2001-10",
    "2001-10-32",
    "2001-13-26+02:00",
    "01-10-26",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_decimal(void)
{
  static const char* const regexp =
    "[+-]?(([0-9]+[.]?[0-9]*)|([0-9]*[.]?[0-9]+))";

  static const char* const good[] = {
    "3.0",
    "-3.0",
    "+3.5",
    "3",
    ".3",
    "3.",
    "0",
    "-.3",
    "0003.0",
    "3.0000",
    "-456",
    NULL,
  };

  static const char* const bad[] = {"3,5", ".", "", NULL};

  test_pattern(regexp, good, bad);
}

// Tests both xsd:float and xsd:double, which are lexically identical
static void
test_float(void)
{
  static const char* const regexp = "-?INF|NaN|[+-]?(([0-9]+[.]?[0-9]*)|([0-"
                                    "9]*[.]?[0-9]+))([eE][-+]?[0-9]+)?";

  static const char* const good[] = {
    "-3E2",
    "4268.22752E11",
    "+24.3e-3",
    "12",
    "+3.5",
    "INF",
    "-INF",
    "-0",
    "NaN",
    NULL,
  };

  static const char* const bad[] = {
    "-3E2.4",
    "12E",
    "+INF",
    "NAN",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_gDay(void)
{
  static const char* const regexp =
    "---(0[1-9]|[12][0-9]|3[01])(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "---02",
    "---01",
    "---01Z",
    "---01+02:00",
    "---01-04:00",
    "---15",
    "---31",
    NULL,
  };

  static const char* const bad[] = {
    "02",
    "---2",
    "---32",
    "--30-",
    "---35",
    "---5",
    "15",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_gMonth(void)
{
  static const char* const regexp =
    "--(0[1-9]|1[0-2])(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "--04",
    "--04-05:00",
    "--05",
    "--11Z",
    "--11+02:00",
    "--11-04:00",
    "--02",
    NULL,
  };

  static const char* const bad[] = {
    "2004-04",
    "04",
    "--4",
    "--13",
    "-01-",
    "--13",
    "--1",
    "01",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_gMonthDay(void)
{
  static const char* const regexp = //
    "--(0[1-9]|1[0-2])"             //
    "-(0[1-9]|[12][0-9]|3[01])"     //
    "(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "--04-12",
    "--04-12Z",
    "--05-01",
    "--11-01Z",
    "--11-01+02:00",
    "--11-01-04:00",
    "--11-15",
    "--02-29",
    NULL,
  };

  static const char* const bad[] = {
    "04-12",
    // "--04-31", Not quite that clever...
    "--4-6",
    "-01-30-",
    "--01-35",
    "--1-5",
    "01-15",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_gYear(void)
{
  static const char* const regexp =
    "-?[0-9][0-9][0-9][0-9][0-9]*(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "2004",
    "2004-05:00",
    "12004",
    "0922",
    "-0045",
    "2001+02:00",
    "2001Z",
    "2001+00:00",
    "-2001",
    "-20000",
    NULL,
  };

  static const char* const bad[] = {
    "99",
    "922",
    "01",
    "2001-12",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_gYearMonth(void)
{
  static const char* const regexp = //
    "-?[0-9][0-9][0-9][0-9][0-9]*"  //
    "-(0[1-9]|1[0-2])"              //
    "(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "2001-10",
    "2001-10+02:00",
    "2001-10Z",
    "2001-10+00:00",
    "-2001-10",
    "-20000-04",
    "2004-04-05:00",
    NULL,
  };

  static const char* const bad[] = {
    "2001",
    "2001-13",
    "2001-13-26+02:00",
    "01-10",
    "99-04",
    "2004",
    "2004-4",
    "2004-13",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_hexBinary(void)
{
  static const char* const regexp = "([0-9A-Fa-f][0-9A-Fa-f])*";
  static const char* const good[] = {"0FB8", "0fb8", "", NULL};
  static const char* const bad[]  = {"F", "FB8", NULL};

  test_pattern(regexp, good, bad);
}

static void
test_integer(void)
{
  static const char* const regexp = "[-+]?[0-9]+";
  static const char* const good[] = {"122", "00122", "0", "-3", "+3", NULL};
  static const char* const bad[]  = {"3.", "3.0", "A", "", NULL};

  test_pattern(regexp, good, bad);
}

static void
test_language(void)
{
  static const char* const regexp =                     //
    "[a-zA-Z][a-zA-Z]?[a-zA-Z]?[a-zA-Z]?"               //
    "[a-zA-Z]?[a-zA-Z]?[a-zA-Z]?[a-zA-Z]?"              //
    "(-[a-zA-Z0-9][a-zA-Z0-9]?[a-zA-Z0-9]?[a-zA-Z0-9]?" //
    "[a-zA-Z0-9]?[a-zA-Z0-9]?[a-zA-Z0-9]?[a-zA-Z0-9]?)*";

  static const char* const good[] = {
    "en",
    "en-GB",
    "en-US",
    "fr",
    "fr-FR",
    "fr-CA",
    "de",
    "zh",
    "ja",
    "ko",
    "i-navajo",
    "x-Newspeak",
    "any-value-with-short-parts",
    NULL,
  };

  static const char* const bad[] = {
    "longerThan8",
    "even-longerThan8",
    "longererThan8-first",
    "last-longererThan8",
    "middle-longererThan8-CA",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_nonNegativeInteger(void)
{
  static const char* const regexp = "[+]?[0-9]+";
  static const char* const good[] = {"+3", "122", "0", "0012", "+123", NULL};
  static const char* const bad[]  = {"-3", "3.0", "", NULL};

  test_pattern(regexp, good, bad);
}

static void
test_nonPositiveInteger(void)
{
  static const char* const regexp = "(0|-[0-9]+)";
  static const char* const good[] = {"-3", "-0", "-00122", NULL};
  static const char* const bad[]  = {"122", "+3", "3.", "3.0", "", NULL};

  test_pattern(regexp, good, bad);
}

static void
test_positiveInteger(void)
{
  static const char* const regexp = "[+]?[0-9]*[1-9]+[0-9]*";
  static const char* const good[] = {"122", "+3", "00122", NULL};
  static const char* const bad[]  = {"0", "-3", "3.0", "", NULL};

  test_pattern(regexp, good, bad);
}

static void
test_duration(void)
{
  static const char* const regexp = //
    "-?P"                           //
    "([0-9]+Y)?"                    //
    "([0-9]+M)?"                    //
    "([0-9]+D)?"                    //
    "(T"                            //
    "([0-9]+H)?"                    //
    "([0-9]+M)?"                    //
    "([0-9]+(\\.[0-9]+)?S)?"        //
    ")?";

  static const char* const good[] = {
    "PT1004199059S",
    "PT130S",
    "PT2M10S",
    "P1DT2S",
    "-P1Y",
    "P1Y2M3DT5H20M30.123S",
    NULL,
  };

  static const char* const bad[] = {
    "1Y",
    "P1S",
    "P-1Y",
    "P1M2Y",
    "P1Y-1M",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_datetime(void)
{
  static const char* const regexp = //
    "-?[0-9][0-9][0-9][0-9][0-9]*"  //
    "-(0[1-9]|1[0-2])"              //
    "-(0[1-9]|[12][0-9]|3[01])"     //
    "T"                             //
    "(([0-1][0-9])|(2[0-4])):"      //
    "[0-5][0-9]:"                   //
    "[0-5][0-9](.[0-9]+)?"          //
    "(Z|[-+][0-2][0-9]:[0-5][0-9])?";

  static const char* const good[] = {
    "2001-10-26T21:32:52",
    "2001-10-26T21:32:52+02:00",
    "2001-10-26T19:32:52Z",
    "2001-10-26T19:32:52+00:00",
    "-2001-10-26T21:32:52",
    "2001-10-26T21:32:52.12679",
    NULL,
  };

  static const char* const bad[] = {
    "2001-10-26",
    "2001-10-26T21:32",
    "2001-10-26T25:32:52+02:00",
    "01-10-26T21:32",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

static void
test_time(void)
{
  static const char* const regexp =
    "(([0-1][0-9])|(2[0-4])):[0-5][0-9]:[0-5][0-9](.[0-9]+)?(Z|[-+][0-2][0-"
    "9]:[0-5][0-9])?";

  static const char* const good[] = {
    "13:20:00",
    "13:20:30.5555",
    "13:20:00-05:00",
    "13:20:00Z",
    "00:00:00",
    "24:00:00",
    "21:32:52",
    "21:32:52+02:00",
    "19:32:52Z",
    "19:32:52+00:00",
    "21:32:52.12679",
    NULL,
  };

  static const char* const bad[] = {
    "5:20:00",
    "13:20",
    "13:20.5:00",
    "13:65:00",
    "21:32",
    "25:25:10",
    "-10:00:00",
    "1:20:10",
    "",
    NULL,
  };

  test_pattern(regexp, good, bad);
}

int
main(void)
{
  test_base64Binary();
  test_boolean();
  test_date();
  test_decimal();
  test_float();
  test_gDay();
  test_gMonth();
  test_gMonthDay();
  test_gYear();
  test_gYearMonth();
  test_hexBinary();
  test_integer();
  test_language();
  test_nonNegativeInteger();
  test_nonPositiveInteger();
  test_positiveInteger();
  test_duration();
  test_datetime();
  test_time();

  return 0;
}
