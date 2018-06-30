mouve_check_dependencies(opencv_imgproc opencv_features2d)
mouve_add_plugin(features2d-ksurf
    dllmain.cpp
    nodes/kSURF.cpp
    impl/ksurf.cpp
    impl/ksurf.h
)
mouve_source_group(features2d-ksurf)
target_link_libraries(features2d-ksurf PRIVATE opencv_imgproc opencv_features2d)
target_include_directories(features2d-ksurf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET TBB::tbb)
    # How to "get it" more easily from Mouve.Logic
    target_link_libraries(features2d-ksurf PRIVATE TBB::tbb)
    target_compile_definitions(features2d-ksurf PRIVATE HAVE_TBB)
endif()
