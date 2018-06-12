mouve_add_plugin(Plugin.Kuwahara
    cvu.cpp
    cvu.h
    GpuKuwaharaFilterNode.cpp
    KuwaharaFilterNode.cpp
    main.cpp
)
target_link_libraries(Plugin.Kuwahara PRIVATE opencv_imgproc)

if(TARGET clw::clw)
    mouve_add_kernels(Plugin.Kuwahara KERNELS kernels/kuwahara.cl)
endif()
