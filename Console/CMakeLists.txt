add_executable(Mouve.Console main.cpp)
target_link_libraries(Mouve.Console PUBLIC
    Mouve.Logic
    opencv_highgui
)
target_include_directories(Mouve.Console PRIVATE ${mouve_SOURCE_DIR})

set_target_properties(Mouve.Console
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS Mouve.Console
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
