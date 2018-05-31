add_library(Plugin.AMP MODULE
    AmpArithmeticNodes.cpp
    AmpMorphologyNodes.cpp
    main.cpp
    Prerequisites.h
)
target_link_libraries(Plugin.AMP PRIVATE
    Mouve.Logic
)
target_include_directories(Plugin.AMP PRIVATE ${mouve_SOURCE_DIR})

install(TARGETS Plugin.AMP
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
    ARCHIVE DESTINATION lib/plugins
)
