if(NOT TARGET opencv_video)
    return()
endif()

mouve_add_plugin(video-segmentation dllmain.cpp
    Nodes/AdaptiveMOG.cpp
    Nodes/BackgroundSubtractor.cpp
    Nodes/GMG.cpp
    Nodes/MOG.cpp
)
target_link_libraries(video-segmentation PRIVATE opencv_video)
