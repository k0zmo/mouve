add_library(Plugin.ShiTomasi MODULE ShiTomasi.cpp)
target_link_libraries(Plugin.ShiTomasi PRIVATE Mouve.Logic)
target_include_directories(Plugin.ShiTomasi PRIVATE ${mouve_SOURCE_DIR})

install(TARGETS Plugin.ShiTomasi
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
    ARCHIVE DESTINATION lib/plugins
)
