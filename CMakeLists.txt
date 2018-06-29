cmake_minimum_required(VERSION 3.9)
project(mouve LANGUAGES CXX VERSION 1.0)

# Set CMake modules path
list(APPEND CMAKE_MODULE_PATH "${mouve_SOURCE_DIR}/cmake")
include(mouve)

option(MOUVE_USE_PRECOMPILED_HEADERS "Use precompiled headers." ON)
option(MOUVE_WITH_GUI_EDITOR "Build Qt5-based GUI editor" ON)
option(MOUVE_WITH_JAI "Build with JAI cameras support" OFF)
option(MOUVE_WITH_VISUAL_LEAK_DETECTOR "Use Visual Leak Detector in debug build." OFF)

if(MOUVE_USE_PRECOMPILED_HEADERS)
    include(cotire)
endif()

# Show folders in MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost 1.61.0 REQUIRED
    COMPONENTS
        filesystem
        system
)
find_package(OpenCV 2.4 REQUIRED
    COMPONENTS
        opencv_core
        opencv_imgproc
        opencv_highgui
        opencv_features2d
        opencv_flann
        opencv_calib3d
    OPTIONAL_COMPONENTS
        opencv_video
        opencv_nonfree
)
find_package(TBB QUIET)

if(MOUVE_WITH_GUI_EDITOR)
    find_package(Qt5 REQUIRED
        COMPONENTS
            Core
            Gui
            Widgets
            OpenGL
        )
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
endif()

if(MOUVE_WITH_JAI)
    set(JAI_DYNAMIC TRUE)
    find_package(JAI REQUIRED)
endif()

if(MOUVE_WITH_VISUAL_LEAK_DETECTOR)
    find_package(VLD REQUIRED)
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

foreach(vendor clw;fmt;kl)
    add_subdirectory(vendor/${vendor} EXCLUDE_FROM_ALL)
    set_target_properties(${vendor} PROPERTIES FOLDER vendor)
endforeach()
set_target_properties(json11 PROPERTIES FOLDER vendor)

add_subdirectory(Logic)
add_subdirectory(Console)
add_subdirectory(plugins)

if(MOUVE_WITH_GUI_EDITOR)
    add_subdirectory(UI)
    set_directory_properties(PROPERTIES VS_STARTUP_PROJECT Mouve.UI)
endif()
