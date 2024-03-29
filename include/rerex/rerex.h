// Copyright 2020-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef REREX_REREX_H
#define REREX_REREX_H

#include <stdbool.h>
#include <stddef.h>

// REREX_API must be used to decorate things in the public API
#ifndef REREX_API
#  if defined(_WIN32) && !defined(REREX_STATIC) && defined(REREX_INTERNAL)
#    define REREX_API __declspec(dllexport)
#  elif defined(_WIN32) && !defined(REREX_STATIC)
#    define REREX_API __declspec(dllimport)
#  elif defined(__GNUC__)
#    define REREX_API __attribute__((visibility("default")))
#  else
#    define REREX_API
#  endif
#endif

// GCC function attributes
#if defined(__GNUC__)
#  define REREX_CONST_API __attribute__((const)) REREX_API
#else
#  define REREX_CONST_API REREX_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Return status code
typedef enum {
  REREX_SUCCESS,
  REREX_EXPECTED_CHAR,
  REREX_EXPECTED_ELEMENT,
  REREX_EXPECTED_RBRACKET,
  REREX_EXPECTED_RPAREN,
  REREX_EXPECTED_SPECIAL,
  REREX_UNEXPECTED_SPECIAL,
  REREX_UNEXPECTED_END,
  REREX_UNORDERED_RANGE,
  REREX_NO_MEMORY,
} RerexStatus;

/// Pattern that represents a compiled valid regular expression
typedef struct RerexPatternImpl RerexPattern;

/// Matcher that can be used to match strings against a pattern
typedef struct RerexMatcherImpl RerexMatcher;

/// Return a human-readable description of `status`
REREX_CONST_API
const char*
rerex_strerror(RerexStatus status);

/**
   Build a regular expression from a pattern string.

   The `pattern` must be a null-terminated string in supported regular
   expression syntax.  Upon return, `end` is set to the offset of the last
   character processed.  On success, `REREX_SUCCESS` is returned, and `out` is
   pointed to a newly allocated pattern which must be freed with
   rerex_free_pattern().  On error, `out` is unchanged, an error code is
   returned, and `end` can be used to determine the position of the error in
   the pattern.
*/
REREX_API
RerexStatus
rerex_compile(const char* pattern, size_t* end, RerexPattern** out);

/**
   Allocate a new matcher for matching against a pattern.

   The returned matcher can be used to match several strings against a single
   pattern using rerex_match().  It is newly allocated and must be freed with
   rerex_free_matcher().
*/
REREX_API
RerexMatcher*
rerex_new_matcher(const RerexPattern* regexp);

/// Return true if `string` matches the pattern of `matcher`
REREX_API
bool
rerex_match(RerexMatcher* matcher, const char* string);

/// Free a matcher allocated with rerex_new_matcher()
REREX_API
void
rerex_free_matcher(RerexMatcher* matcher);

/// Free a pattern allocated with rerex_compile()
REREX_API
void
rerex_free_pattern(RerexPattern* expression);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // REREX_REREX_H
