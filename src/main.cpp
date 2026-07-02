#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "appcontroller.h"
#include "bootstrap.h"
#include "thememanager.h"
#include "wingetadapter.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("YAS"));
    app.setApplicationName(QStringLiteral("yas-winget"));
    app.setApplicationDisplayName(QStringLiteral("Yet Another Store for Winget"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    yas::loadBundledFonts();

    WingetAdapter adapter;
    yas::AppController controller(&adapter);
    yas::ThemeManager themeManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("App"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("YasManager"), &themeManager);
    engine.loadFromModule("YasWinget", "Main");
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}
