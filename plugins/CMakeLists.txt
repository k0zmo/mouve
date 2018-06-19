add_subdirectory(brisk)
add_subdirectory(kuwahara)
add_subdirectory(morphology)
add_subdirectory(shi-tomasi)
add_subdirectory(template)

if(TARGET opencv_video)
    add_subdirectory(video-segmentation)
endif()

if(TARGET clw::clw)
    add_subdirectory(kuwahara-opencl)
    add_subdirectory(morphology-opencl)
    add_subdirectory(video-segmentation-opencl)
endif()

if(MSVC) 
    # AMP only supported with Visual C++
    add_subdirectory(amp)
endif()
