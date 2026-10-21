#pragma once

class QGuiApplication;

namespace neovlc::app {

// Compose application services and QML, then enter the Qt event loop.
int run(QGuiApplication &application);

} // namespace neovlc::app
