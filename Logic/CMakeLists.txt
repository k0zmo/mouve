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

if(MOUVE_USE_PRECOMPILED_HEADERS)
    set_target_properties(Mouve.Logic 
        PROPERTIES
            COTIRE_CXX_PREFIX_HEADER_INIT Precomp.h)
    set_directory_properties(PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    cotire(Mouve.Logic)
endif()

if(TARGET clw::clw)
    target_link_libraries(Mouve.Logic PUBLIC clw::clw)
    target_compile_definitions(Mouve.Logic PUBLIC HAVE_OPENCL)

    install(DIRECTORY OpenCL/kernels DESTINATION bin FILES_MATCHING PATTERN *.cl)
endif()

install(TARGETS Mouve.Logic 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
