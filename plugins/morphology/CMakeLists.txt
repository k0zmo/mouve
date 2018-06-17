mouve_add_plugin(morphology dllmain.cpp
    Utils.cpp
    Utils.h
    Nodes/HitMissOperator.cpp
    Nodes/MorphologyOperator.cpp
    Nodes/StructuringElement.cpp
    Nodes/GpuMorphologyOperator.cpp
)
target_link_libraries(morphology PRIVATE opencv_imgproc)
target_include_directories(morphology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
mouve_source_group(morphology)

if(TARGET clw::clw)
    mouve_add_kernels(morphology KERNELS kernels/morphOp.cl)
endif()
