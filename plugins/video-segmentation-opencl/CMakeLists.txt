mouve_check_dependencies(clw::clw)
mouve_add_plugin(video-segmentation-opencl
    dllmain.cpp
    nodes/GpuMOG.cpp
)
mouve_source_group(video-segmentation-opencl)
mouve_add_kernels(video-segmentation-opencl KERNELS kernels/mog.cl)
