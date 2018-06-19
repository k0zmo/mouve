mouve_add_plugin(brisk
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
mouve_source_group(brisk)
target_link_libraries(brisk PRIVATE opencv_features2d)
target_include_directories(brisk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-mssse3 ssse3_supported)
if(ssse3_supported)
    target_compile_options(brisk PRIVATE -mssse3)
endif()
