# Package the UI in the executable so QML resources are always linked in.
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/Main.qml"
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

qt_add_qml_module(neovlc
    URI NeoVLC
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES "${CMAKE_CURRENT_LIST_DIR}/Main.qml"
)
