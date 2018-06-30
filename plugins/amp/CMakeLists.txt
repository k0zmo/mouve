if(NOT MSVC)
    # AMP only supported with Visual C++
    return()
endif()

mouve_add_plugin(amp
    dllmain.cpp
    nodes/AmpArithmetic.cpp
    nodes/AmpMorphology.cpp
)
mouve_source_group(amp)