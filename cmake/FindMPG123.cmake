find_package(PkgConfig)
pkg_check_modules(PC_MPG123 QUIET libmpg123)

find_path(MPG123_INCLUDE_DIR mpg123.h
          HINTS ${PC_MPG123_INCLUDEDIR} ${PC_MPG123_INCLUDE_DIRS})
find_library(MPG123_LIBRARY NAMES mpg123 mpg123-0
             HINTS ${PC_MPG123_LIBDIR} ${PC_MPG123_LIBRARY_DIRS})

set(MPG123_LIBRARIES ${MPG123_LIBRARY})
set(MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR})

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(MPG123 DEFAULT_MSG MPG123_LIBRARY MPG123_INCLUDE_DIR)
mark_as_advanced(MPG123_INCLUDE_DIR MPG123_LIBRARY)
