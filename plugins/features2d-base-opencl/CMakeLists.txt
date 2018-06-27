mouve_add_plugin(features2d-base-opencl
    dllmain.cpp
    nodes/GpuHoughLines.cpp
)
mouve_source_group(features2d-base-opencl)
mouve_add_kernels(features2d-base-opencl KERNELS
    kernels/fill_buffer.cl
    kernels/hough.cl
)
