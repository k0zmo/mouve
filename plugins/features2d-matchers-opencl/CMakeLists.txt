mouve_check_dependencies(clw::clw)
mouve_add_plugin(features2d-matchers-opencl
    dllmain.cpp
    nodes/GpuBruteForceMatcher.cpp
)
mouve_source_group(features2d-matchers-opencl)
mouve_add_kernels(features2d-matchers-opencl KERNELS
    kernels/bfmatcher.cl
    kernels/bfmatcher_macros.cl
)
