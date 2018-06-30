mouve_check_dependencies(clw::clw)
mouve_add_plugin(filters-base-opencl
    dllmain.cpp
    nodes/GpuApproxGaussianBlur.cpp
)
mouve_source_group(filters-base-opencl)
mouve_add_kernels(filters-base-opencl KERNELS kernels/approx_gaussian.cl)
