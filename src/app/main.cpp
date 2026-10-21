#include "Application.h"

#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("NeoVLC"));
    QCoreApplication::setOrganizationName(QStringLiteral("Kiisanz"));

    return neovlc::app::run(application);
}
