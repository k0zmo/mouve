find_path(VLD_ROOT
    NAMES
        include/vld.h
    PATHS
        ENV VLDROOT
        "$ENV{PROGRAMFILES}/Visual Leak Detector"
        "$ENV{PROGRAMFILES\(X86\)}/Visual Leak Detector"
    DOC "VLD root directory"
)

find_path(VLD_INCLUDE_DIR
    vld.h
    HINTS ${VLD_ROOT}
    PATH_SUFFIXES include
    DOC "VLD include directory"
)
mark_as_advanced(VLD_INCLUDE_DIR)

set(possible_suffixes lib)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND possible_suffixes lib/Win32)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND possible_suffixes lib/Win64)
endif()

find_library(VLD_LIBRARY
    NAMES vld
    HINTS ${VLD_ROOT}
    PATH_SUFFIXES ${possible_suffixes}
    DOC "VLD library"
)
mark_as_advanced(VLD_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VLD 
    REQUIRED_VARS 
        VLD_INCLUDE_DIR
        VLD_LIBRARY
)

if(VLD_FOUND AND NOT TARGET VLD::VLD)
    add_library(VLD::VLD UNKNOWN IMPORTED)
    set_target_properties(VLD::VLD PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${VLD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${VLD_INCLUDE_DIR}"
    )
endif()
