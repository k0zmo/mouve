mouve_add_plugin(morphology
    dllmain.cpp
    nodes/HitMissOperator.cpp
    nodes/MorphologyOperator.cpp
    nodes/StructuringElement.cpp
    impl/Utils.cpp
    impl/Utils.h
)
target_link_libraries(morphology PRIVATE opencv_imgproc)
target_include_directories(morphology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
mouve_source_group(morphology)
