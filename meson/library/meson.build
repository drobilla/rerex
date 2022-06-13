# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

# General code to determine the library type to build.
#
# Unfortunately, meson's default_library option does not handle real-world
# situations out of the box.  In particular, it is usually necessary to specify
# different flags for static and shared builds of C libraries so that symbols
# can be exported.  To work around this, we do not support default_library=both
# on Windows.  On other platforms with GCC-like compilers, we can support both
# because symbols can safely be exported in the same way (giving them default
# visibility) in both static and shared builds.

# Set library_type to the target_type to use with build_target() for libraries
if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif

  library_type = 'both_libraries'
elif get_option('default_library') == 'shared'
  library_type = 'shared_library'
else
  library_type = 'static_library'
endif

# Set library_suffix to the suffix for libraries
if cc.get_id() == 'msvc'
  # Meson inconsistently adds a version to the name only in this case, so
  # provide no suffix to get a sensible name with a single version suffix
  library_suffix = ''
else
  library_suffix = '-@0@'.format(meson.project_version().split('.')[0])
endif
