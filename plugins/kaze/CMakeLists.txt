mouve_add_plugin(kaze
    lib/fed.cpp
    lib/fed.h
    lib/nldiffusion_functions.cpp
    lib/nldiffusion_functions.h
    lib/utils.cpp
    lib/utils.h
    lib/KAZE.cpp
    lib/KAZE.h
    lib/KAZEConfig.h
    lib/AKAZE.cpp
    lib/AKAZE.h
    lib/AKAZEConfig.h
    KazeFilterNode.cpp
)
target_link_libraries(kaze PRIVATE
    opencv_core
    opencv_imgproc
    opencv_features2d
    opencv_calib3d
)
target_include_directories(kaze PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

find_package(OpenMP QUIET)
if(TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(kaze PRIVATE OpenMP::OpenMP_CXX)
endif()
