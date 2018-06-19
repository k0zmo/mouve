mouve_add_plugin(kuwahara-opencl
    dllmain.cpp
    nodes/GpuKuwaharaFilter.cpp
    nodes/GpuAnisotropicKuwaharaFilter.cpp
    nodes/GpuGeneralizedKuwaharaFilter.cpp
    impl/kuwahara.cpp
    impl/kuwahara.h
)
mouve_source_group(kuwahara-opencl)
mouve_add_kernels(kuwahara-opencl KERNELS kernels/kuwahara.cl)
target_include_directories(kuwahara-opencl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kuwahara-opencl PRIVATE opencv_imgproc)
