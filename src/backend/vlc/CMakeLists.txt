find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBVLC REQUIRED IMPORTED_TARGET libvlc>=3.0)

add_library(neovlc_backend_vlc STATIC
    VlcContext.cpp
    VlcContext.h
    VlcMediaPlayer.cpp
    VlcMediaPlayer.h
    VlcVideoOutput.cpp
    VlcVideoOutput.h
)
add_library(NeoVLC::Vlc ALIAS neovlc_backend_vlc)

target_include_directories(neovlc_backend_vlc
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(neovlc_backend_vlc
    PUBLIC Qt6::Core Qt6::Gui
    PRIVATE PkgConfig::LIBVLC
)
