mouve_check_dependencies(opencv_calib3d)
mouve_add_plugin(features2d-homography
    dllmain.cpp
    nodes/Homography.cpp
)
mouve_source_group(features2d-homography)
target_link_libraries(features2d-homography PRIVATE opencv_calib3d)
target_include_directories(features2d-homography PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
