macro(mouve_bin_directory _config _directory)
    if(MSVC) # We should really check for multi-config generator
        set(${_directory} ${CMAKE_BINARY_DIR}/bin/${_config})
    else()
        set(${_directory} ${CMAKE_BINARY_DIR}/bin)
    endif()
endmacro()

function(mouve_add_plugin _name)
    add_library(${_name} MODULE ${ARGN})
    target_link_libraries(${_name} PRIVATE Mouve.Logic)
    target_include_directories(${_name} PRIVATE ${mouve_SOURCE_DIR})

    if(MSVC) # We should really check for multi-config generator
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${config} configUpper)
            mouve_bin_directory(${config} directory)
            set_target_properties(${_name} PROPERTIES 
                LIBRARY_OUTPUT_DIRECTORY_${configUpper} ${directory}/plugins
            )
        endforeach()
    else()
        mouve_bin_directory(${CMAKE_BUILD_TYPE} directory)
        set_target_properties(${_name} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${directory}/plugins
        )
    endif()

    install(TARGETS ${_name}
        RUNTIME DESTINATION bin/plugins
        LIBRARY DESTINATION bin/plugins
        ARCHIVE DESTINATION lib/plugins
    )
endfunction()
