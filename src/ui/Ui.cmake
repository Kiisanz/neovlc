# Package the UI in the executable so QML resources are always linked in.
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/Main.qml"
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

# Generated QML type registration includes module headers by bare name.
target_include_directories(neovlc PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

qt_add_qml_module(neovlc
    URI NeoVLC
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES "${CMAKE_CURRENT_LIST_DIR}/Main.qml"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/VideoItem.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/VideoItem.h"
)
