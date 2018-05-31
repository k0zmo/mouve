add_library(Mouve.Kommon
    EnumFlags.h
    Hash.cpp
    Hash.h
    HighResolutionClock.cpp
    HighResolutionClock.h
    MacroUtils.h
    ModulePath.cpp
    ModulePath.h
    NestedException.h
    SharedLibrary.cpp
    SharedLibrary.h
    Singleton.h
    StringUtils.cpp
    StringUtils.h
    TypeTraits.h
    Utils.h
    json11.cpp
    json11.hpp
    konfig.h
)

get_target_property(sources_list Mouve.Kommon SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${sources_list})
target_link_libraries(Mouve.Kommon PRIVATE ${CMAKE_DL_LIBS})

install(TARGETS Mouve.Kommon 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
