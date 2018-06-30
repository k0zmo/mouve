mouve_check_dependencies(opencv_video)
mouve_add_plugin(video-segmentation
    dllmain.cpp
    nodes/AdaptiveMOG.cpp
    nodes/BackgroundSubtractor.cpp
    nodes/GMG.cpp
    nodes/MOG.cpp
)
target_link_libraries(video-segmentation PRIVATE opencv_video)
mouve_source_group(video-segmentation)
