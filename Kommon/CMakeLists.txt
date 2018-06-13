add_library(Mouve.Kommon
    EnumFlags.h
    Hash.cpp
    Hash.h
    TypeTraits.h
    json11.cpp
    json11.hpp
)

get_target_property(sources_list Mouve.Kommon SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${sources_list})
target_compile_features(Mouve.Kommon PUBLIC cxx_std_11)

install(TARGETS Mouve.Kommon 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
