mouve_add_plugin(kaze
    lib/fed.cpp
    lib/fed.h
    lib/KAZE.cpp
    lib/KAZE.h
    lib/KAZEConfig.h
    lib/nldiffusion_functions.cpp
    lib/nldiffusion_functions.h
    lib/utils.cpp
    lib/utils.h
    KazeFilterNode.cpp
)
target_link_libraries(kaze PRIVATE
    opencv_core
    opencv_imgproc
    opencv_features2d
    opencv_calib3d
)

find_package(OpenMP QUIET)
if(TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(kaze PRIVATE OpenMP::OpenMP_CXX)
endif()
