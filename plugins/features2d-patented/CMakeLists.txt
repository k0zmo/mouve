mouve_add_plugin(features2d-patented
    dllmain.cpp
    nodes/SIFT.cpp
    nodes/SURF.cpp
)
mouve_source_group(features2d-patented)
target_link_libraries(features2d-patented PRIVATE opencv_nonfree)
