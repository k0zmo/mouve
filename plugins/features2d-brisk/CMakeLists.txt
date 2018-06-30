mouve_check_dependencies(opencv_features2d)
mouve_add_plugin(features2d-brisk
    dllmain.cpp
    nodes/Brisk.cpp
    nodes/BriskDescriptorExtractor.cpp
    nodes/BriskFeatureDetector.cpp
    impl/brisk.cpp
    impl/brisk.h
    impl/agast/agast5_8_nms.cc
    impl/agast/agast5_8.cc
    impl/agast/agast5_8.h
    impl/agast/agast7_12d_nms.cc
    impl/agast/agast7_12d.cc
    impl/agast/agast7_12d.h
    impl/agast/agast7_12s_nms.cc
    impl/agast/agast7_12s.cc
    impl/agast/agast7_12s.h
    impl/agast/AstDetector.cc
    impl/agast/AstDetector.h
    impl/agast/cvWrapper.h
    impl/agast/nonMaximumSuppression.cc
    impl/agast/oast9_16_nms.cc
    impl/agast/oast9_16.cc
    impl/agast/oast9_16.h
)
mouve_source_group(features2d-brisk)
target_link_libraries(features2d-brisk PRIVATE opencv_features2d)
target_include_directories(features2d-brisk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-mssse3 ssse3_supported)
if(ssse3_supported)
    target_compile_options(features2d-brisk PRIVATE -mssse3)
endif()
