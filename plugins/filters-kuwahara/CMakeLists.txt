mouve_add_plugin(filters-kuwahara
    dllmain.cpp
    nodes/AnisotropicKuwaharaFilter.cpp
    nodes/GeneralizedKuwaharaFilter.cpp
    nodes/KuwaharaFilter.cpp
    impl/kuwahara.cpp
    impl/kuwahara.h
)
mouve_source_group(filters-kuwahara)
target_link_libraries(filters-kuwahara PRIVATE opencv_imgproc)
target_include_directories(filters-kuwahara PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
