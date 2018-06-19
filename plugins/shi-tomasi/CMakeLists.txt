mouve_add_plugin(shi-tomasi
    dllmain.cpp
    nodes/ShiTomasi.cpp
)
mouve_source_group(shi-tomasi)
target_link_libraries(shi-tomasi PRIVATE opencv_imgproc)
