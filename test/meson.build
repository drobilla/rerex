# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

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

# Run unit tests
foreach name : ['syntax', 'match', 'xsd']
  full_name = 'test_@0@'.format(name)
  source = files('@0@.c'.format(full_name))
  test(full_name,
       executable(full_name,
                  source,
                  include_directories: include_dirs,
                  dependencies: rerex_dep),
       suite: 'unit')
endforeach
