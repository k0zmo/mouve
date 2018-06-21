mouve_add_plugin(features2d-shi-tomasi
    dllmain.cpp
    nodes/ShiTomasi.cpp
)
mouve_source_group(features2d-shi-tomasi)
target_link_libraries(features2d-shi-tomasi PRIVATE opencv_imgproc)
