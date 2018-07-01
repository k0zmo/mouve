mouve_check_dependencies(opencv_features2d opencv_flann)
mouve_add_plugin(features2d-matchers
    dllmain.cpp
    nodes/DrawMatches.cpp
    nodes/Matchers.cpp
    impl/Drawing.h
    impl/Drawing.cpp
    impl/Matchers.h
    impl/Matchers.cpp
)
mouve_source_group(features2d-matchers)
target_link_libraries(features2d-matchers PRIVATE opencv_features2d opencv_flann)
target_include_directories(features2d-matchers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
