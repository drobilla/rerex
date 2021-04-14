Rerex
=====

Rerex (REally REgular eXpressions) is a simple and efficient NFA-based regular
expression implementation in C99.

Rerex parses a regular expression into an NFA, then simulates execution using
Thompson's algorithm, so matching is linear in the size of the input.  For
details on the theory behind this approach, see [Programming Techniques:
Regular expression search algorithm], the classic paper by Ken Thompson.
[Regular Expression Matching Can Be Simple And Fast] is also a good read, which
compares this approach with those used in most modern regular expression
implementations.  The matching algorithm in Rerex is effectively the same as
the one described there, though with a slightly different implementation.
Parsing and construction are completely different, using a recursive-descent
parser which constructs the automata on the fly.

This is a minimalist implementation with a limited feature set which is mainly
suitable for basic applications like checking markup or programming language
syntax.  It does, however, strive to be clear and high-quality code which is a
bit more fully-featured than a simple example, and suitable for real-world use.
For example, character set expressions are supported, and meaningful errors are
reported with the offset of the problematic character.

The generated automata are not always optimal, though there are a few simple
optimizations that avoid generating redundant states.  Though there is room for
improvement there (at the cost of additional complexity), matching performs
relatively well.

Implementation
--------------

The implementation is clean and warning-free C99, tested and known to work with
at least clang, GCC, and MSVC on POSIX, MacOS, and Windows.  It weighs in at
under 600 [SLOC][], which can be compiled to about 5k on x86.

Additionally, a simple test suite is included which covers 100% of the code.
It is run continuously on all of the above mentioned platforms on x64, as well
as in Linux on 32 and 64 bit ARM via emulation.

Supported Grammar
-----------------

Rerex supports a basic subset of classic regular expression syntax that is
supported by almost all implementations.  The grammar is written here in [XML
EBNF notation][] with terminals in uppercase:

    DOT       ::= '.'
    OPERATOR  ::= '*' | '+' | '?'
    SPECIAL   ::= DOT | OPERATOR | '(' | ')' | '[' | ']' | '^' | '{' | '|' | '}'
    ESCAPABLE ::= SPECIAL | '-'
    ESCAPE    ::= '\' SPECIAL
    CHAR      ::= ESCAPE | [#x20-#x7E] - SPECIAL
    ELEMENT   ::= ([#x20-#x7E] - ']') | ('\' ']')
    Range     ::= ELEMENT | ELEMENT '-' ELEMENT
    Set       ::= Range | Range Set
    Atom      ::= CHAR | DOT | '(' Expr ')' | '[' Set ']'
    Factor    ::= Atom | Atom OPERATOR
    Term      ::= Factor | Factor Term
    Expr      ::= Term | Term '|' Expr

Notable Limitations
-------------------

Rerex is not meant to replace fully-featured regular expression libraries, so
the feature set is somewhat limited.  The most glaring omissions include:

  - Only supports printable ASCII input
  - Only supports anchored matching (no back references or group extraction)
  - Only reads from a null-terminated string (not, for example, files)
  - No support for counted replication with `{}`

Should I Use This?
------------------

If you just want to understand how regular expressions work, and have some
clean code that's easy to understand and tinker with, sure.

If you need a minimal regular expression matching implementation in portable C
for basic ASCII tasks, maybe.

If you need a fully-featured regular expression implementation for
international text with group extraction and other more advanced features,
probably not.

In any case, if you do, letting me know or linking to this page would be
appreciated.

Dependencies
------------

None, except the C standard library.

Building
--------

Rerex consists only of a single header and configuration-free source file, so
building it with any compiler manually should be straightforward.  It should,
in theory, work just about anywhere.

A [Meson][] build definition is included which can be used to do a proper
system installation with a `pkg-config` file, generate IDE projects, run the
tests, and so on.  For example, the library and tests can be built and run like
so:

    meson setup -Dtests=true build
    cd build
    ninja test

See the [Meson documentation][] for more details on using Meson.

Usage
-----

Rerex has a small API based around two objects: a _pattern_, which is an
immutable compiled representation of a regular expression, and a _matcher_
which can be used to match strings against a pattern.  See [the
header](include/rerex/rerex.h) for details, there is no external documentation
aside from this README.

This simple example function uses everything in the API to provide a high-level
string-based match function:

```c
bool matches(const char* regexp, const char* string)
{
    RerexPattern* pattern = NULL;
    size_t        end     = 0;
    RerexStatus   st      = rerex_compile(regexp, &end, &pattern);

    if (st) {
        fprintf(stderr, "(string):%zu: error: %s\n", end, rerex_strerror(st));
        return false;
    }

    RerexMatcher* matcher = rerex_new_matcher(pattern);
    bool          matches = rerex_match(matcher, string);

    rerex_free_matcher(matcher);
    rerex_free_pattern(pattern);

    return matches;
}
```

 -- David Robillard <d@drobilla.net>

[XML EBNF notation]: http://www.w3.org/TR/REC-xml/#sec-notation

[SLOC]: https://en.wikipedia.org/wiki/Source_lines_of_code

[Meson]: https://mesonbuild.com/

[Meson documentation]: https://mesonbuild.com/Quick-guide.html

[Programming Techniques: Regular expression search algorithm]: https://dl.acm.org/doi/10.1145/363347.363387

[Regular Expression Matching Can Be Simple And Fast]: https://swtch.com/~rsc/regexp/regexp1.html
