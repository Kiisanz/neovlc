#include "Application.h"

#include <vlc/VlcContext.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

namespace neovlc::app {

int run(QGuiApplication &application)
{
    // Keep libVLC alive until all objects owned by the QML engine are destroyed.
    backend::VlcContext vlc;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vlcBackend"), &vlc);
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
