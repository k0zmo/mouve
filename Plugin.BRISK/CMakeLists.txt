mouve_add_plugin(Plugin.BRISK
    brisk.cpp
    brisk.h
    BriskNode.cpp
    agast/agast5_8_nms.cc
    agast/agast5_8.cc
    agast/agast5_8.h
    agast/agast7_12d_nms.cc
    agast/agast7_12d.cc
    agast/agast7_12d.h
    agast/agast7_12s_nms.cc
    agast/agast7_12s.cc
    agast/agast7_12s.h
    agast/AstDetector.cc
    agast/AstDetector.h
    agast/cvWrapper.h
    agast/nonMaximumSuppression.cc
    agast/oast9_16_nms.cc
    agast/oast9_16.cc
    agast/oast9_16.h
)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-mssse3 ssse3_supported)
if(ssse3_supported)
    target_compile_options(Plugin.BRISK PRIVATE -mssse3)
endif()
