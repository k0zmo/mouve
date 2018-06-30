mouve_check_dependencies(clw::clw opencv_imgproc opencv_features2d)
mouve_add_plugin(features2d-ksurf-opencl
    dllmain.cpp
    nodes/GpuSURF.cpp
)
mouve_source_group(features2d-ksurf-opencl)
mouve_add_kernels(features2d-ksurf-opencl KERNELS
    kernels/fill.cl
    kernels/integral.cl
    kernels/surf.cl
)
target_link_libraries(features2d-ksurf-opencl PRIVATE
    opencv_imgproc
    opencv_features2d
)
