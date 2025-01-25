# - Find libseccomp library and headers
# Once done, this will define the following variables:
#  SECcomp_FOUND        - TRUE if both the library and header were found
#  SECcomp_INCLUDE_DIR  - Path to the seccomp.h header
#  SECcomp_LIBRARY      - Path to the seccomp library (libseccomp.so or equivalent)

find_path(Seccomp_INCLUDE_DIR
    NAMES seccomp.h
    HINTS /usr/include /usr/local/include /usr/include/linux
)

find_library(Seccomp_LIBRARY
    NAMES libseccomp libseccomp.so
    HINTS /usr/lib /usr/local/lib /lib /lib64 /usr/lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Seccomp REQUIRED_VARS Seccomp_INCLUDE_DIR Seccomp_LIBRARY)

if (Seccomp_FOUND)
    set(Seccomp_LIBRARIES ${Seccomp_LIBRARY})
    set(Seccomp_INCLUDE_DIRS ${Seccomp_INCLUDE_DIR})
endif ()
