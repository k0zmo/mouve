mouve_add_plugin(features2d-ksurf-opencl
    dllmain.cpp
    nodes/GpuSurf.cpp
)
mouve_source_group(features2d-ksurf-opencl)
mouve_add_kernels(features2d-ksurf-opencl KERNELS
    kernels/fill.cl
    kernels/integral.cl
    kernels/surf.cl
)
target_include_directories(features2d-ksurf-opencl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(features2d-ksurf-opencl PRIVATE
    opencv_imgproc
    opencv_features2d
    clw::clw)
