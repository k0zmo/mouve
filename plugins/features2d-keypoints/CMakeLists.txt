mouve_add_plugin(features2d-keypoints
    dllmain.cpp
    nodes/ShiTomasi.cpp
    nodes/FREAK.cpp
    nodes/ORB.cpp
)
# Prioritize BRISK implementation from the features2d-brisk plug-in over the
# OpenCV built-in BRISK as it uses different agast score which gives worse results
# NOTE: Nodes are not compatible with each others - different properties
if(NOT TARGET features2d-brisk)
    target_sources(features2d-keypoints PRIVATE nodes/BRISK.cpp)
endif()
mouve_source_group(features2d-keypoints)

target_link_libraries(features2d-keypoints PRIVATE
    opencv_imgproc
    opencv_features2d
)
