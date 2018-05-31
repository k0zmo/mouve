if(JAI_DYNAMIC)
    find_path(JAI_INCLUDE_DIR
        Jai_Factory_Dynamic.h
        PATHS ENV JAI_SDK_INCLUDE
    )
    mark_as_advanced(JAI_INCLUDE_DIR)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(JAI
        REQUIRED_VARS 
            JAI_INCLUDE_DIR
    )

    if(JAI_FOUND AND NOT TARGET JAI::JAI)
        add_library(JAI::JAI INTERFACE IMPORTED)
        set_target_properties(JAI::JAI PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${JAI_INCLUDE_DIR}"
            INTERFACE_COMPILE_DEFINITIONS "JAI_SDK_DYNAMIC_LOAD=1"
            INTERFACE_LINK_LIBRARIES "$<$<PLATFORM_ID:Windows>:ShLwApi>"
        )
    endif()
else()
    find_path(JAI_INCLUDE_DIR
        jai_factory.h
        PATHS ENV JAI_SDK_INCLUDE
    )
    mark_as_advanced(JAI_INCLUDE_DIR)

    set(jai_sdk_lib_env "JAI_SDK_LIB")
    if(CMAKE_SIZEOF_VOID_P STREQUAL "8")
        string(APPEND jai_sdk_lib_env "_64")
    endif()

    find_library(JAI_LIBRARY 
        Jai_Factory
        PATHS
            ENV ${jai_sdk_lib_env}
    )
    mark_as_advanced(JAI_LIBRARY)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(JAI
        REQUIRED_VARS 
            JAI_INCLUDE_DIR
            JAI_LIBRARY
    )

    if(JAI_FOUND AND NOT TARGET JAI::JAI)
        add_library(JAI::JAI UNKNOWN IMPORTED)
        set_target_properties(JAI::JAI PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${JAI_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${JAI_INCLUDE_DIR}"
        )
    endif()
endif()
