mouve_check_dependencies(opencv_imgproc)
mouve_add_plugin(filters-base
    dllmain.cpp
    nodes/Custom.cpp
    nodes/HighPass.cpp
    nodes/LowPass.cpp
    impl/Predefined.h
    impl/Predefined.cpp
)
mouve_source_group(filters-base)
target_link_libraries(filters-base PRIVATE opencv_imgproc)
target_include_directories(filters-base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
