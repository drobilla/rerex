Contributing to Rerex
=====================

Feel free to tinker with Rerex, I tried to make the code as easy to follow as I
could while preserving the minimalist spirit of it.  Implementing character
sets and escapes and so on have made things a little bit hairier than a
textbook example, but hopefully it is still easy to digest.

If you want to make a contribution, feel free to send patches, but note that I
would like to preserve the cleanliness avoid bloating things too much, so I may
not accept patches for features that are too complicated.  Bug fixes or other
improvements to the code are, of course, most welcome.

That said, this is, of course, free software, so you're free to take it and do
as you please.  I'd be happy to chat about it, these are just guidelines for
what I feel would be appropriate for inclusion in the project.

Happy hacking.

Tooling
-------

All of the code is machine formatted, the style is defined by the included
`clang-format` configuration.  You can use `ninja clang-format` to format
everything.

Configuring with `-Dstrict=true` will enable the ultra-strict warnings that are
run on CI (nearly all of them).  Configuring with `-Dwerror=true` will make
them errors.

A `clang-tidy` configuration is included for more intensive static checking,
you can run all the checks with `ninja clang-tidy`.
