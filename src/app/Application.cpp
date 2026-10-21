#include "Application.h"

#include <vlc/VlcContext.h>
#include <vlc/VlcMediaPlayer.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

namespace neovlc::app {

int run(QGuiApplication &application)
{
    // Keep libVLC alive until all objects owned by the QML engine are destroyed.
    backend::VlcContext vlc;
    // Declared after the context and before the engine so it is destroyed first.
    backend::VlcMediaPlayer player(vlc);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vlcBackend"), &vlc);
    engine.rootContext()->setContextProperty(QStringLiteral("vlcPlayer"), &player);
    const QUrl mainUrl(QStringLiteral("qrc:/NeoVLC/Main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &application, [mainUrl](QObject *object, const QUrl &url) {
            if (!object && url == mainUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(mainUrl);

    return application.exec();
}

} // namespace neovlc::app
