mouve_add_plugin(Plugin.ShiTomasi
    ShiTomasi.cpp
)
target_link_libraries(Plugin.ShiTomasi PRIVATE opencv_imgproc)
