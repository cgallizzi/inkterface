#include <cstdlib>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>

#include "config.h"
#include "loghandler.hpp"
#include "panel-finder.hpp"
#include "panel-state.hpp"
#include "panel.hpp"
#include "svcmgr.hpp"
#include "unsig.hpp"

#define EXIT_SUCCESS 0

using namespace Qt::Literals::StringLiterals;

bool registerFonts()
{
    bool success = true;
    QString prefix = u":/%1/resources/IBM_Plex_Mono/"_s.arg(QML_URI);
    const auto entries = QDir(prefix).entryList({u"*.ttf"_s});
    for (const QString &f : entries) {
        if (QFontDatabase::addApplicationFont(prefix + f) == -1) {
            success = false;
            break;
        }
    }
    return success;
}

int runUi(int argc, char *argv[])
{
    qDebug() << "Running with UI...";
    QApplication app(argc, argv);
    app.setApplicationName(PROJECT_DISPLAY_NAME);
    app.setOrganizationName(ORG_NAME);
    app.setOrganizationDomain(ORG_DOMAIN);
    app.setApplicationVersion(PROJECT_GITREV);
    app.setWindowIcon(QIcon(u":/%1/resources/icon.png"_s.arg(QML_URI)));
    QSettings settings;
    qDebug() << "Settings stored in:" << settings.fileName();

    QQuickStyle::setStyle("Basic");
    if (!registerFonts()) {
        qWarning() << "Failed to register custom fonts!";
    } else {
        QFont font(u"IBM Plex Mono"_s, -1, QFont::Medium);
        // NOTE: we use pixel size rather than point size here so our layout
        //       elements in QML are consistent across platforms
        font.setPixelSize(24);
        app.setFont(font);
    }

    QQmlApplicationEngine engine(&app);
    QQmlContext *ctx = engine.rootContext();

    PanelFinder *panelFinder = new PanelFinder(&app);
    ctx->setContextProperty("panelFinder", panelFinder);

    SvcMgr *svcMgr = new SvcMgr(&app);
    ctx->setContextProperty("svcMgr", svcMgr);

    PanelState *fs = new PanelState(&app);
    ctx->setContextProperty("panelState", fs);

    const QUrl url(u"qrc:/%1/qml/main.qml"_s.arg(QML_URI));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url, &app](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical("Failed to load any QML objects!");
                app.quit();
            }
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

int runHeadless(int argc, char *argv[])
{
    qDebug() << "Running headless...";
    QCoreApplication app(argc, argv);
    UnSig unsig(&app);
    unsig.catchSignal(SIGINT);
    unsig.catchSignal(SIGTERM);

    app.setApplicationName(PROJECT_DISPLAY_NAME);
    app.setOrganizationName(ORG_NAME);
    app.setOrganizationDomain(ORG_DOMAIN);
    app.setApplicationVersion(PROJECT_GITREV);

    Panel panel(&app);

    int exitCode = EXIT_SUCCESS;

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &panel, &Panel::stop);
    QObject::connect(&unsig, &UnSig::unixSignal, &panel, &Panel::stop);
    exitCode = app.exec();
    return exitCode;
}

int installService([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    SvcMgr svcMgr;
    svcMgr.installService();
    return EXIT_SUCCESS;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    LogHandler::install(PROJECT_NAME ".log", 50);

    QLoggingCategory::setFilterRules("qt.bluetooth* = false");
    qDebug() << PROJECT_DISPLAY_NAME << "version" << PROJECT_GITREV;

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(u"%1 data feed generator."_s.arg(PROJECT_NAME));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption installOption{u"install"_s, u"Install as a systemd service."_s};
    parser.addOption(installOption);

    QCommandLineOption headlessOption{u"headless"_s, u"Run without UI, used for service mode."_s};
    parser.addOption(headlessOption);

    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args << QString(argv[i]);
    }
    parser.process(args);

    if (parser.isSet(installOption)) {
        return installService(argc, argv);
    } else if (parser.isSet(headlessOption)) {
        return runHeadless(argc, argv);
    } else {
        return runUi(argc, argv);
    }
}
