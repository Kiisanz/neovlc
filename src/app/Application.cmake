target_sources(neovlc PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Application.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Application.h"
)

target_link_libraries(neovlc
    PRIVATE NeoVLC::Vlc Qt6::Quick Qt6::QuickControls2
)
