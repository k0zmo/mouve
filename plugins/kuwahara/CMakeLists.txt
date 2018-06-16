mouve_add_plugin(kuwahara
    cvu.cpp
    cvu.h
    GpuKuwaharaFilterNode.cpp
    KuwaharaFilterNode.cpp
    main.cpp
)
target_link_libraries(kuwahara PRIVATE opencv_imgproc)

if(TARGET clw::clw)
    mouve_add_kernels(kuwahara KERNELS kernels/kuwahara.cl)
endif()
