find_path(AMDTActivityLogger_ROOT
    NAMES
        include/CXLActivityLogger.h
    PATHS
        "$ENV{PROGRAMFILES\(X86\)}/CodeXL/SDK/AMDTActivityLogger"
        "$ENV{PROGRAMFILES}/CodeXL/SDK/AMDTActivityLogger"
    DOC "AMD Developer Tools Activity Logger root directory"
)

find_path(AMDTActivityLogger_INCLUDE_DIR
    NAMES CXLActivityLogger.h
    HINTS ${AMDTActivityLogger_ROOT}
    PATH_SUFFIXES include
    DOC "AMD Developer Tools Activity Logger include directory"
)
mark_as_advanced(AMDTActivityLogger_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AMDTActivityLogger
    REQUIRED_VARS
        AMDTActivityLogger_ROOT
        AMDTActivityLogger_INCLUDE_DIR
)

if(AMDTActivityLogger_FOUND AND NOT TARGET AMDT::ActivityLogger)
    add_library(AMDT::ActivityLogger INTERFACE IMPORTED)
    set_target_properties(AMDT::ActivityLogger PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${AMDTActivityLogger_INCLUDE_DIR}
    )
endif()
