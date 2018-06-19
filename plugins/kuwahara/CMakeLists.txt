mouve_add_plugin(kuwahara
    dllmain.cpp
    nodes/AnisotropicKuwaharaFilter.cpp
    nodes/GeneralizedKuwaharaFilter.cpp
    nodes/KuwaharaFilter.cpp
    impl/kuwahara.cpp
    impl/kuwahara.h
)
mouve_source_group(kuwahara)
target_link_libraries(kuwahara PRIVATE opencv_imgproc)
target_include_directories(kuwahara PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
