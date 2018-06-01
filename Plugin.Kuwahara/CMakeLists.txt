mouve_add_plugin(Plugin.Kuwahara
    cvu.cpp
    cvu.h
    GpuKuwaharaFilterNode.cpp
    KuwaharaFilterNode.cpp
    main.cpp
)

if(TARGET clw::clw)
    install(DIRECTORY kernels DESTINATION bin FILES_MATCHING PATTERN *.cl)
endif()
