project('rerex', ['c'],
        version: '0.0.1',
        license: 'ISC',
        meson_version: '>= 0.49.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
          'default_library=shared',
          'warning_level=3',
        ])

major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'rerex' + version_suffix

# Load build tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Set ultra strict warnings for developers, if requested
c_warnings = []
c_suppressions = []
if get_option('strict')
  if not meson.is_subproject()
    subdir('meson')
    add_project_arguments(all_c_warnings, language: ['c'])
  endif

  if cc.get_id() == 'gcc'
    c_suppressions = [
      '-Wno-aggregate-return',
      '-Wno-switch-default',
    ]
  elif cc.get_id() == 'msvc'
    c_suppressions = [
      '/wd4706',  # assignment within conditional expression
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4800',  # implicit conversion to bool
      '/wd5045',  # will insert Spectre mitigation
    ]
  else
    c_suppressions = []
  endif
endif

rerex_c_args = cc.get_supported_arguments(c_suppressions)

if cc.get_id() == 'msvc'
  # Build as C++
  add_project_arguments(['/TP'], language: ['c'])

  # Suppress warnings in system headers
  add_project_arguments(['/experimental:external',
                         '/external:W0',
                         '/external:anglebrackets'],
                        language: ['c'])
endif

# Determine library type and the flags needed to build it
if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif

  library_type = 'both_libraries'
  library_args = ['-DREREX_INTERNAL']
  prog_args = []
elif get_option('default_library') == 'shared'
  library_type = 'shared_library'
  library_args = ['-DREREX_INTERNAL']
  prog_args = []
else
  library_type = 'static_library'
  library_args = ['-DREREX_INTERNAL', '-DREREX_STATIC']
  prog_args = ['-DREREX_STATIC']
endif

# Build shared and/or static library/libraries
librerex = build_target(
  versioned_name,
  files('src/rerex.c'),
  version: meson.project_version(),
  include_directories: include_directories(['include']),
  c_args: rerex_c_args + library_args,
  gnu_symbol_visibility: 'hidden',
  install: true,
  target_type: library_type)

# Generage pkg-config file
pkg.generate(
  librerex,
  name: 'Rerex',
  filebase: versioned_name,
  subdirs: [versioned_name],
  version: meson.project_version(),
  description: 'A simple and efficient regular expression implementation')

# Install header to a versioned include directory
install_headers(files('include/rerex/rerex.h'), subdir: versioned_name / 'rerex')

rerex_dep = declare_dependency(
  link_with: librerex,
  include_directories: include_directories(['include']))

# Build and run tests
if get_option('tests')
  foreach name : ['syntax', 'match', 'xsd']
    full_name = 'test_@0@'.format(name)
    source = files('test/@0@.c'.format(full_name))
    test(full_name,
         executable(full_name,
                    source,
                    c_args: rerex_c_args + prog_args,
                    include_directories: include_directories(['include']),
                    dependencies: rerex_dep))
  endforeach
endif

if not meson.is_subproject() and meson.version().version_compare('>=0.53.0')
  summary('Tests', get_option('tests'), bool_yn: true)

  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
