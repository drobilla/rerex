# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

# Project-specific warning suppressions.
#
# This should be used in conjunction with the generic "warnings" sibling that
# enables all reasonable warnings for the compiler.  It lives here just to keep
# the top-level meson.build more readable.

#####
# C #
#####

if is_variable('cc')
  c_suppressions = []

  if get_option('strict')
    if cc.get_id() == 'gcc'
      c_suppressions += [
        '-Wno-switch-default',
      ]
    elif cc.get_id() == 'msvc'
      c_suppressions += [
        '/wd4711', # function selected for automatic inline expansion
        '/wd4800', # implicit conversion to bool
        '/wd5045', # will insert Spectre mitigation
      ]
    endif
  endif

  if cc.get_id() == 'msvc'
    c_suppressions += [
      '/wd4706', # assignment within conditional expression
    ]
  endif

  c_suppressions = cc.get_supported_arguments(c_suppressions)
endif
