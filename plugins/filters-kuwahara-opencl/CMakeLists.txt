mouve_add_plugin(filters-kuwahara-opencl
    dllmain.cpp
    nodes/GpuKuwaharaFilter.cpp
    nodes/GpuAnisotropicKuwaharaFilter.cpp
    nodes/GpuGeneralizedKuwaharaFilter.cpp
    impl/kuwahara.cpp
    impl/kuwahara.h
)
mouve_source_group(filters-kuwahara-opencl)
mouve_add_kernels(filters-kuwahara-opencl KERNELS kernels/kuwahara.cl)
target_include_directories(filters-kuwahara-opencl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(filters-kuwahara-opencl PRIVATE opencv_imgproc)
