# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

project('rerex', ['c'],
        version: '0.0.1',
        license: 'ISC',
        meson_version: '>= 0.54.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
        ])

rerex_src_root = meson.current_source_dir()
rerex_build_root = meson.current_build_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'rerex' + version_suffix

#######################
# Compilers and Flags #
#######################

# Required tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Set global warning suppressions
c_suppressions = []
warning_level = get_option('warning_level')
if cc.get_id() == 'clang'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-declaration-after-statement',
    ]

    if not meson.is_cross_build()
      c_suppressions += [
        '-Wno-poison-system-directories',
      ]
    endif
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-switch-default',
    ]
  endif

elif cc.get_id() == 'msvc'
  if warning_level == 'everything'
    c_suppressions += [
      '/wd4711', # function selected for automatic inline expansion
      '/wd4800', # implicit conversion to bool
      '/wd5045', # will insert Spectre mitigation
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '/wd4706', # assignment within conditional expression
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

##########################
# Platform Configuration #
##########################

# Use versioned name everywhere to support parallel major version installations
if host_machine.system() == 'windows'
  if get_option('default_library') == 'both'
    error('default_library=both is not supported on Windows')
  endif
  soversion = ''
else
  soversion = meson.project_version().split('.')[0]
endif

###########
# Library #
###########

include_dirs = include_directories(['include'])
c_headers = files('include/rerex/rerex.h')
sources = files('src/rerex.c')

# Set appropriate arguments for building against the library type
extra_c_args = []
if get_option('default_library') == 'static'
  extra_c_args = ['-DREREX_STATIC']
endif

# Build shared and/or static library
librerex = library(
  versioned_name,
  sources,
  c_args: c_suppressions + extra_c_args + ['-DREREX_INTERNAL'],
  gnu_symbol_visibility: 'hidden',
  include_directories: include_dirs,
  install: true,
  soversion: soversion,
  version: meson.project_version(),
)

# Declare dependency for internal meson dependants
rerex_dep = declare_dependency(
  compile_args: extra_c_args,
  include_directories: include_dirs,
  link_with: librerex,
)

# Generage pkg-config file for external dependants
pkg.generate(
  librerex,
  description: 'A simple and efficient regular expression implementation',
  extra_cflags: extra_c_args,
  filebase: versioned_name,
  name: 'Rerex',
  subdirs: [versioned_name],
  version: meson.project_version(),
)

# Override pkg-config dependency for internal meson dependants
meson.override_dependency(versioned_name, rerex_dep)

# Install header to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'rerex')

#########
# Tests #
#########

# Build and run tests
if not get_option('tests').disabled()
  subdir('test')
endif

# Display configuration summary
if not meson.is_subproject()
  summary('Tests', not get_option('tests').disabled(), bool_yn: true)
  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
