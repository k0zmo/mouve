add_library(Plugin.Template MODULE
    main.cpp
    TemplateNode.cpp
)
target_link_libraries(Plugin.Template PRIVATE
    Mouve.Logic
)
target_include_directories(Plugin.Template PRIVATE ${mouve_SOURCE_DIR})

install(TARGETS Plugin.Template
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
    ARCHIVE DESTINATION lib/plugins
)
