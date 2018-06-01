function(mouve_add_plugin _name)
    add_library(${_name} MODULE ${ARGN})
    target_link_libraries(${_name} PRIVATE Mouve.Logic)
    target_include_directories(${_name} PRIVATE ${mouve_SOURCE_DIR})

    if(MSVC) # We should really check for multi-config generator
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${config} configUpper)
            set_target_properties(${_name}
                PROPERTIES 
                    LIBRARY_OUTPUT_DIRECTORY_${configUpper} ${CMAKE_BINARY_DIR}/bin/${config}/plugins
            )
        endforeach()
    else()
        set_target_properties(${_name}
            PROPERTIES 
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins

        )
    endif()

    install(TARGETS ${_name}
        RUNTIME DESTINATION bin/plugins
        LIBRARY DESTINATION bin/plugins
        ARCHIVE DESTINATION lib/plugins
    )
endfunction()
