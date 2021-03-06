add_library(Mouve.Logic SHARED # TEMPORARY
    Precomp.h
    Precomp.cpp
    Node.cpp
    Node.h
    NodeException.h
    NodeFactory.h
    NodeFlowData.cpp
    NodeFlowData.h
    NodeLink.cpp
    NodeLink.h
    NodeModule.h
    NodePlugin.cpp
    NodePlugin.h
    NodeProperty.cpp
    NodeProperty.h
    NodeResolver.cpp
    NodeResolver.h
    NodeSystem.cpp
    NodeSystem.h
    NodeTree.cpp
    NodeTree.h
    NodeTreeSerializer.cpp
    NodeTreeSerializer.h
    NodeType.cpp
    NodeType.h
    Prerequisites.h
)

target_sources(Mouve.Logic PRIVATE
    Nodes/ArithmeticNodes.cpp
    Nodes/BriskNodes.cpp
    Nodes/CV.cpp
    Nodes/CV.h
    Nodes/ColorConversionNodes.cpp
    Nodes/DrawFeaturesNodes.cpp
    Nodes/FeatureDetectorsNodes.cpp
    Nodes/FeaturesNodes.cpp
    Nodes/FiltersNodes.cpp
    Nodes/FreakNodes.cpp
    Nodes/HistogramNodes.cpp
    Nodes/HomographyNodes.cpp
    Nodes/KeypointsNodes.cpp
    Nodes/MatcherNodes.cpp
    Nodes/MorphologyNodes.cpp
    Nodes/MosaicingNodes.cpp
    Nodes/OrbNodes.cpp
    Nodes/SegmentationNodes.cpp
    Nodes/SiftNodes.cpp
    Nodes/SinkNodes.cpp
    Nodes/SourceNodes.cpp
    Nodes/SurfNodes.cpp
    Nodes/TransformationNodes.cpp
    Nodes/VideoSegmentationNodes.cpp
    Nodes/kSurfNodes.cpp
    Nodes/ksurf.cpp
    Nodes/ksurf.h
)

target_sources(Mouve.Logic PRIVATE
    OpenCL/DeviceArray.cpp
    OpenCL/DeviceArray.h
    OpenCL/GpuActivityLogger.cpp
    OpenCL/GpuActivityLogger.h
    OpenCL/GpuApproxGaussianBlurNode.cpp
    OpenCL/GpuBasicNodes.cpp
    OpenCL/GpuBruteForceMacherNode.cpp
    OpenCL/GpuColorConversionNodes.cpp
    OpenCL/GpuException.h
    OpenCL/GpuHoughLinesNode.cpp
    OpenCL/GpuKernelLibrary.cpp
    OpenCL/GpuKernelLibrary.h
    OpenCL/GpuMixtureOfGaussiansNode.cpp
    OpenCL/GpuMorphologyOperatorNode.cpp
    OpenCL/GpuNode.h
    OpenCL/GpuNodeModule.cpp
    OpenCL/GpuNodeModule.h
    OpenCL/GpuSurfNode.cpp
    OpenCL/IGpuNodeModule.h
)

target_sources(Mouve.Logic PRIVATE
    Jai/IJaiNodeModule.h
    Jai/JaiCameraNode.cpp
    Jai/JaiException.h
    Jai/JaiNodeModule.cpp
    Jai/JaiNodeModule.h
)

get_target_property(sources_list Mouve.Logic SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${sources_list})

target_include_directories(Mouve.Logic PRIVATE ${mouve_SOURCE_DIR})
target_link_libraries(Mouve.Logic PUBLIC Mouve.Kommon)
target_compile_definitions(Mouve.Logic PRIVATE LOGIC_LIB) # Building DLL
target_compile_features(Mouve.Logic PUBLIC cxx_std_11)

target_link_libraries(Mouve.Logic PUBLIC
    Boost::boost
    Boost::disable_autolinking
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_video
    opencv_features2d
    opencv_nonfree
    opencv_flann
    opencv_calib3d
    Qt5::Core
)

set_target_properties(Mouve.Logic
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin    
)

if(MOUVE_USE_PRECOMPILED_HEADERS)
    set_target_properties(Mouve.Logic
        PROPERTIES
            COTIRE_CXX_PREFIX_HEADER_INIT Precomp.h)
    set_directory_properties(PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    cotire(Mouve.Logic)
endif()

if(TARGET TBB::tbb)
    message(STATUS "Intel TBB support: YES")
    target_link_libraries(Mouve.Logic PUBLIC TBB::tbb)
    target_compile_definitions(Mouve.Logic PRIVATE HAVE_TBB)
else()
    message(STATUS "Intel TBB support: NO")
endif()

if(TARGET clw::clw)
    message(STATUS "OpenCL support: YES")
    target_link_libraries(Mouve.Logic PUBLIC clw::clw)
    target_compile_definitions(Mouve.Logic PUBLIC HAVE_OPENCL)

    mouve_add_kernels(Mouve.Logic KERNELS
        OpenCL/kernels/approx_gaussian.cl
        OpenCL/kernels/bayer.cl
        OpenCL/kernels/bfmatcher.cl
        OpenCL/kernels/bfmatcher_macros.cl
        OpenCL/kernels/color.cl
        OpenCL/kernels/fill.cl
        OpenCL/kernels/hough.cl
        OpenCL/kernels/integral.cl
        OpenCL/kernels/mog.cl
        OpenCL/kernels/morphOp.cl
        OpenCL/kernels/surf.cl
    )
else()
    message(STATUS "OpenCL support: NO")
endif()

if(TARGET clw::clw AND TARGET AMDT::ActivityLogger)
    target_link_libraries(Mouve.Logic PRIVATE AMDT::ActivityLogger)
    target_compile_definitions(Mouve.Logic PRIVATE HAVE_AMDT_ACTIVITY_LOGGER)
endif()

if(TARGET JAI::JAI)
    message(STATUS "JAI support: YES")
    target_link_libraries(Mouve.Logic PRIVATE JAI::JAI)
    target_compile_definitions(Mouve.Logic PUBLIC HAVE_JAI)
else()
    message(STATUS "JAI support: NO")
endif()

install(TARGETS Mouve.Logic
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
