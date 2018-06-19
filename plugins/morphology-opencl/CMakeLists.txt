mouve_add_plugin(morphology-opencl
    dllmain.cpp
    nodes/GpuMorphologyOperator.cpp
)
target_include_directories(morphology-opencl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
mouve_source_group(morphology-opencl)
mouve_add_kernels(morphology-opencl KERNELS kernels/morphOp.cl)
