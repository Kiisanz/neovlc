# The application composes the backend and the QML UI.
add_subdirectory(backend/vlc)

qt_add_executable(neovlc MANUAL_FINALIZATION)
include(app/Application.cmake)
include(ui/Ui.cmake)
qt_finalize_executable(neovlc)
