# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

project('rerex', ['c'],
        version: '0.0.1',
        license: 'ISC',
        meson_version: '>= 0.53.0',
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

# Set global warning flags
subdir('meson/warnings')
subdir('meson/suppressions')

###########
# Library #
###########

include_dirs = include_directories(['include'])
c_headers = files('include/rerex/rerex.h')
sources = files('src/rerex.c')

# Set appropriate arguments for building against the library type
subdir('meson/library')
if get_option('default_library') == 'static'
  add_project_arguments(['-DREREX_STATIC'], language: ['c'])
endif

# Build shared and/or static library
librerex = library(
  meson.project_name() + library_suffix,
  sources,
  version: meson.project_version(),
  include_directories: include_dirs,
  c_args: c_warnings + ['-DREREX_INTERNAL'],
  gnu_symbol_visibility: 'hidden',
  install: true)

# Generage pkg-config file for external dependants
pkg.generate(
  librerex,
  name: 'Rerex',
  filebase: versioned_name,
  subdirs: [versioned_name],
  version: meson.project_version(),
  description: 'A simple and efficient regular expression implementation')

# Declare dependency for internal meson dependants
rerex_dep = declare_dependency(
  link_with: librerex,
  include_directories: include_dirs)

# Install header to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'rerex')

#########
# Tests #
#########

# Build and run tests
if get_option('tests')
  subdir('test')
endif

# Display configuration summary
if not meson.is_subproject()
  summary('Tests', get_option('tests'), bool_yn: true)
  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
