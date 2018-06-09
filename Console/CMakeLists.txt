add_executable(Mouve.Console main.cpp)

target_link_libraries(Mouve.Console Mouve.Logic)
target_include_directories(Mouve.Console PRIVATE ${mouve_SOURCE_DIR})

install(TARGETS Mouve.Console
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
