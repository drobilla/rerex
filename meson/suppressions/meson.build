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

if get_option('strict')
  if cc.get_id() == 'clang' and host_machine.system() == 'darwin'
    c_warnings += [
      '-Wno-poison-system-directories',
    ]
  elif cc.get_id() == 'gcc'
    c_warnings += [
      '-Wno-switch-default',
    ]
  elif cc.get_id() == 'msvc'
    c_warnings += [
      '/wd4711', # function selected for automatic inline expansion
      '/wd4800', # implicit conversion to bool
      '/wd5045', # will insert Spectre mitigation
    ]
  endif
endif

if cc.get_id() == 'msvc'
  c_warnings += [
    '/wd4706', # assignment within conditional expression
  ]
endif

add_project_arguments(cc.get_supported_arguments(c_warnings),
                      language: ['c'])
