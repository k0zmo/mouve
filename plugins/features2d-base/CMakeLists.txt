mouve_add_plugin(features2d-base
    dllmain.cpp
    nodes/DrawFeatures.cpp
    nodes/Hough.cpp
    nodes/Canny.cpp
    impl/Drawing.h
)
mouve_source_group(features2d-base)
target_link_libraries(features2d-base PRIVATE opencv_imgproc)
target_include_directories(features2d-base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
