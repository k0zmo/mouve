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
    Nodes/MosaicingNodes.cpp
    Nodes/OrbNodes.cpp
    Nodes/SegmentationNodes.cpp
    Nodes/SiftNodes.cpp
    Nodes/SinkNodes.cpp
    Nodes/SourceNodes.cpp
    Nodes/SurfNodes.cpp
    Nodes/TransformationNodes.cpp
    Nodes/kSurfNodes.cpp
    Nodes/ksurf.cpp
    Nodes/ksurf.h
)

target_sources(Mouve.Logic PRIVATE
    OpenCL/AmdtActivityLogger.cpp
    OpenCL/AmdtActivityLogger.h
    OpenCL/DeviceArray.cpp
    OpenCL/DeviceArray.h
    OpenCL/GpuActivityLogger.cpp
    OpenCL/GpuActivityLogger.h
    OpenCL/GpuApproxGaussianBlurNode.cpp
    OpenCL/GpuBasicNodes.cpp
    OpenCL/GpuBruteForceMacherNode.cpp
    OpenCL/GpuColorConversionNodes.cpp
    OpenCL/GpuException.cpp
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

# We're going to export symbols explicitly each one by one
include(GenerateExportHeader)
generate_export_header(Mouve.Logic BASE_NAME Mouve)
target_sources(Mouve.Logic PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/mouve_export.h)
set_target_properties(Mouve.Logic PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(MSVC)
    # Disable warning: 'X' needs to have dll-interface to be used by clients of class
    target_compile_options(Mouve.Logic PUBLIC /wd4251 /wd4275)
endif()

target_include_directories(Mouve.Logic 
    PRIVATE ${mouve_SOURCE_DIR} 
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(Mouve.Logic PRIVATE ${CMAKE_DL_LIBS})
target_compile_features(Mouve.Logic PUBLIC cxx_std_14)

target_link_libraries(Mouve.Logic 
    PUBLIC
        Boost::boost
        opencv_core
        fmt::fmt
        kl::kl
    PRIVATE
        Boost::disable_autolinking
        Boost::filesystem
        opencv_imgproc
        opencv_highgui
        opencv_features2d
        opencv_nonfree
        opencv_flann
        opencv_calib3d
)

set_target_properties(Mouve.Logic
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(TARGET TBB::tbb)
    message(STATUS "Intel TBB support: YES")
    target_link_libraries(Mouve.Logic PRIVATE TBB::tbb)
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

if(MOUVE_USE_PRECOMPILED_HEADERS)
    set_target_properties(Mouve.Logic
        PROPERTIES
            COTIRE_CXX_PREFIX_HEADER_INIT Precomp.h)
    set_directory_properties(PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    cotire(Mouve.Logic)
endif()

install(TARGETS Mouve.Logic
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
