mouve_add_plugin(features2d-keypoints
    dllmain.cpp
    nodes/ShiTomasi.cpp
)
mouve_source_group(features2d-keypoints)
target_link_libraries(features2d-keypoints PRIVATE opencv_imgproc)
