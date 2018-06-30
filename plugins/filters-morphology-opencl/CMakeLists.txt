mouve_check_dependencies(clw::clw)
mouve_add_plugin(filters-morphology-opencl
    dllmain.cpp
    nodes/GpuMorphologyOperator.cpp
)
target_include_directories(filters-morphology-opencl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
mouve_source_group(filters-morphology-opencl)
mouve_add_kernels(filters-morphology-opencl KERNELS kernels/morphOp.cl)
