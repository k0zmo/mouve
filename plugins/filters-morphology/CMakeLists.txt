mouve_check_dependencies(opencv_imgproc)
mouve_add_plugin(filters-morphology
    dllmain.cpp
    nodes/HitMissOperator.cpp
    nodes/MorphologyOperator.cpp
    nodes/StructuringElement.cpp
    impl/Utils.cpp
    impl/Utils.h
)
target_link_libraries(filters-morphology PRIVATE opencv_imgproc)
target_include_directories(filters-morphology PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
mouve_source_group(filters-morphology)
