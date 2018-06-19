function(mouve_add_kernels _target)
    set(boolean "")
    set(one_value_args SUBDIR)
    set(multi_value_args KERNELS)
    cmake_parse_arguments(options "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Show these kernel files in IDE, under "Kernel Files" group (only in VS)
    target_sources(${_target} PRIVATE ${options_KERNELS})
    source_group("kernels" FILES ${options_KERNELS})

    # Optional subdir inside kernels/
    set(outdir kernels)
    if(options_SUBDIR)
        string(APPEND outdir /${options_SUBDIR})
    endif()

    # Build POST_BUILD command event that copies all kernel files to bin/$<CONFIG>/kernels directory
    set(cmd "")
    foreach(file ${options_KERNELS})
        get_filename_component(fileabs ${file} ABSOLUTE)
        get_filename_component(filename ${file} NAME)
        list(APPEND cmd 
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${fileabs}
                $<TARGET_FILE_DIR:Mouve.Logic>/${outdir}/${filename}
        )
    endforeach()
    add_custom_command(TARGET ${_target} POST_BUILD ${cmd})

    # Also, add these kernel files to INSTALL target
    install(FILES ${options_KERNELS} DESTINATION bin/${outdir})
endfunction()

function(mouve_add_plugin _name)
    add_library(${_name} MODULE ${ARGN})
    target_link_libraries(${_name} PRIVATE Mouve.Logic)
    target_include_directories(${_name} PRIVATE ${mouve_SOURCE_DIR})
    set_target_properties(${_name} PROPERTIES FOLDER plugins)

    if(MSVC) # We should really check for multi-config generator
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${config} configUpper)
            set_target_properties(${_name} PROPERTIES 
                LIBRARY_OUTPUT_DIRECTORY_${configUpper} ${CMAKE_BINARY_DIR}/bin/${config}/plugins
            )
        endforeach()
    else()
        set_target_properties(${_name} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins
        )
    endif()

    install(TARGETS ${_name}
        RUNTIME DESTINATION bin/plugins
        LIBRARY DESTINATION bin/plugins
        ARCHIVE DESTINATION lib/plugins
    )
endfunction()

function(mouve_source_group _target)
    get_target_property(sources_list ${_target} SOURCES)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${sources_list})
endfunction()
