# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

if get_option('lint')
  # Check release metadata
  autoship = find_program('autoship', required: false)
  if autoship.found()
    test('autoship', autoship,
         args: ['test', rerex_src_root],
         suite: 'data')
  endif

  # Check licensing metadata
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse,
         args: ['--root', rerex_src_root, 'lint'],
         suite: 'data')
  endif

  # Check code with cppcheck
  if not meson.is_subproject()
    cppcheck = find_program('cppcheck', required: false)
    if cppcheck.found()
      compdb_path = join_paths(rerex_build_root, 'compile_commands.json')
      test('cppcheck', cppcheck,
           args: ['-q',
                  '--error-exitcode=1',
                  '--enable=warning,style,performance,portability',
                  '--project=' + compdb_path],
           suite: 'code')
    endif
  endif
endif

# Run unit tests
foreach name : ['syntax', 'match', 'xsd']
  full_name = 'test_@0@'.format(name)
  source = files('@0@.c'.format(full_name))
  test(full_name,
       executable(full_name,
                  source,
                  c_args: c_suppressions,
                  dependencies: rerex_dep,
                  include_directories: include_dirs),
       suite: 'unit')
endforeach

# Set up warning suppressions for header test
header_suppressions = []
if cc.get_id() in ['clang', 'emscripten']
  if warning_level == 'everything' and not meson.is_cross_build()
    header_suppressions += [
      '-Wno-poison-system-directories',
    ]
  endif
endif

# Test that header has no warnings (ignoring suppressions for stricter dependants)
test('test_headers',
     executable('test_headers',
                files('headers/test_headers.c'),
                c_args: header_suppressions,
                dependencies: rerex_dep,
                include_directories: include_dirs))
