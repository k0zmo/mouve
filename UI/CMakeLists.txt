add_executable(Mouve.UI WIN32
    Precomp.h
    Precomp.cpp
    Application.cpp
    Controller.cpp
    Controller.h
    Editor.qrc
    GLWidget.cpp
    GLWidget.h
    GpuChoiceDialog.cpp
    GpuChoiceDialog.h
    LogView.cpp
    LogView.h
    Prerequisites.h
    PreviewWidget.cpp
    PreviewWidget.h
    Property.cpp
    Property.h
    PropertyDelegate.cpp
    PropertyDelegate.h
    PropertyManager.cpp
    PropertyManager.h
    PropertyModel.cpp
    PropertyModel.h
    PropertyWidgets.cpp
    PropertyWidgets.h
    Singleton.h
    TreeWorker.cpp
    TreeWorker.h
)

target_sources(Mouve.UI PRIVATE
    NodeEditor/NodeConnectorView.cpp
    NodeEditor/NodeConnectorView.h
    NodeEditor/NodeEditorView.cpp
    NodeEditor/NodeEditorView.h
    NodeEditor/NodeLinkView.cpp
    NodeEditor/NodeLinkView.h
    NodeEditor/NodeSocketView.cpp
    NodeEditor/NodeSocketView.h
    NodeEditor/NodeStyle.cpp
    NodeEditor/NodeStyle.h
    NodeEditor/NodeToolTip.cpp
    NodeEditor/NodeToolTip.h
    NodeEditor/NodeView.cpp
    NodeEditor/NodeView.h
)
mouve_source_group(Mouve.UI)

target_include_directories(Mouve.UI PRIVATE ${mouve_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(Mouve.UI PUBLIC cxx_std_14)

target_link_libraries(Mouve.UI PUBLIC
    Mouve.Logic
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
    OpenGL::GL
    Threads::Threads
)
set_target_properties(Mouve.UI
    PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(TARGET VLD::VLD)
    message(STATUS "Visual Leak Detector supported: YES")
    target_link_libraries(Mouve.UI PRIVATE VLD)
    target_compile_definitions(Mouve.UI PRIVATE $<$<CONFIG:DEBUG>:HAVE_VLD>)
else()
    message(STATUS "Visual Leak Detector supported: NO")
endif()

if(MOUVE_USE_PRECOMPILED_HEADERS)
    set_target_properties(Mouve.UI
        PROPERTIES
            COTIRE_CXX_PREFIX_HEADER_INIT Precomp.h)
    set_directory_properties(PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    cotire(Mouve.UI)
endif()

if(WIN32)
    target_sources(Mouve.UI PRIVATE Mouve.UI.rc)
endif()

install(TARGETS Mouve.UI
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
