add_library(Plugin.Kuwahara MODULE
    cvu.cpp
    cvu.h
    GpuKuwaharaFilterNode.cpp
    KuwaharaFilterNode.cpp
    main.cpp
)
target_link_libraries(Plugin.Kuwahara PRIVATE
    Mouve.Logic
)
target_include_directories(Plugin.Kuwahara PRIVATE ${mouve_SOURCE_DIR})

install(TARGETS Plugin.Kuwahara
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
    ARCHIVE DESTINATION lib/plugins
)

if(TARGET clw::clw)
    install(DIRECTORY kernels DESTINATION bin FILES_MATCHING PATTERN *.cl)
endif()
