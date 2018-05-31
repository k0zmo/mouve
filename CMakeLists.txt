cmake_minimum_required(VERSION 3.9)
project(mouve LANGUAGES CXX VERSION 1.0)

# Set CMake modules path
list(APPEND CMAKE_MODULE_PATH "${mouve_SOURCE_DIR}/cmake")
include(mouve)

option(MOUVE_USE_PRECOMPILED_HEADERS "Use precompiled headers." ON)
option(MOUVE_USE_VISUAL_LEAK_DETECTOR "Use Visual Leak Detector in debug build." OFF)

if(MOUVE_USE_PRECOMPILED_HEADERS)
    include(cotire)
endif()

# Show folders in MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost 1.55.0 REQUIRED COMPONENTS)
find_package(OpenCV 2.4 REQUIRED
    COMPONENTS
        opencv_core
        opencv_imgproc
        opencv_highgui
        opencv_video
        opencv_features2d
        opencv_nonfree
        opencv_flann
        opencv_calib3d
)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB)

set(JAI_DYNAMIC TRUE)
find_package(JAI)

if(MOUVE_USE_VISUAL_LEAK_DETECTOR)
    find_package(VLD)
endif()

find_package(AMDTActivityLogger)

# OpenCV devs acknowledge only Debug and Release configurations
# We need to manually map the other two to Release.
# Otherwise CMake will try to link with Debug binaries when building
# with MinSizeRel and RelWithDebInfo config causing ABI mismatch with MSVCRT
set_target_properties(${OpenCV_LIBS} PROPERTIES
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
)

add_subdirectory(external/clw)

add_subdirectory(Kommon)
add_subdirectory(Logic)
add_subdirectory(Console)
add_subdirectory(UI)
add_subdirectory(Plugin.AMP)
add_subdirectory(Plugin.BRISK)
add_subdirectory(Plugin.Kuwahara)
add_subdirectory(Plugin.ShiTomasi)
add_subdirectory(Plugin.Template)
