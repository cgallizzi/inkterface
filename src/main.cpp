#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <thread>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QTimer>

#include "config.h"
#include "frunk.hpp"
#include "unsig.hpp"

#define EXIT_SUCCESS 0

using namespace Qt::Literals::StringLiterals;

static const QString SERVICE_TEMPLATE{u"[Unit]\n"_s
                                      u"Description=mango-frunk\n"_s
                                      u"After=network.target\n"_s
                                      u"\n"_s
                                      u"[Service]\n"_s
                                      u"Type=simple\n"_s
                                      u"ExecStart=%1%2\n"_s
                                      u"Restart=on-failure\n"_s
                                      u"\n"_s
                                      u"[Install]\n"_s
                                      u"WantedBy=default.target\n"_s};

QString getExecutablePath()
{
    auto appImage = std::getenv("APPIMAGE");
    if (appImage) {
        return {appImage};
    }
    try {
        return {std::filesystem::canonical("/proc/self/exe").c_str()};
    } catch (const std::exception &ex) {
        return {""};
    }
}

int runHeadless(int argc, char *argv[], [[maybe_unused]] const QString &name)
{
    QCoreApplication app(argc, argv);
    UnSig unsig(&app);
    unsig.catchSignal(SIGINT);
    unsig.catchSignal(SIGTERM);

    app.setApplicationName(PROJECT_DISPLAY_NAME);
    app.setOrganizationName(ORG_NAME);
    app.setOrganizationDomain(ORG_DOMAIN);
    app.setApplicationVersion(PROJECT_GITREV);

    Frunk frunk(&app);

    int exitCode = EXIT_SUCCESS;

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &frunk, &Frunk::stop);
    QObject::connect(&unsig, &UnSig::unixSignal, &frunk, &Frunk::stop);
    exitCode = app.exec();
    return exitCode;
}

int installService([[maybe_unused]] int argc, [[maybe_unused]] char *argv[],
                   [[maybe_unused]] const QString &name)
{
    auto exePath = getExecutablePath();
    if (exePath.isEmpty()) {
        qWarning() << "Can only install service on linux hosts!";
        return 1;
    }
    auto svcDef =
        SERVICE_TEMPLATE.arg(exePath).arg(name.isEmpty() ? u""_s : u" --name %1"_s.arg(name));
    auto dir = QDir::home();
    if (!dir.mkpath(u".config/systemd/user"_s)) {
        qWarning() << "Failed to create systemd user config directory!";
        return 1;
    }
    dir.cd(u".config/systemd/user"_s);
    if (dir.exists(u"mango-frunk.service"_s)) {
        qDebug() << "Uninstalling old service definition...";
        (void)!system("systemctl --user stop mango-frunk.service");
        (void)!system("systemctl --user disable mango-frunk.service");
        dir.remove(u"mango-frunk.service"_s);
    }
    QFile f{dir.filePath(u"mango-frunk.service"_s)};
    if (!f.open(QFile::WriteOnly)) {
        qWarning() << "Failed to create systemd user service definition!";
        return 1;
    }
    f.write(svcDef.toLatin1());
    f.close();
    (void)!system("systemctl --user daemon-reload");
    (void)!system("systemctl --user enable mango-frunk.service");
    (void)!system("systemctl --user start mango-frunk.service");
    return 0;
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules("qt.bluetooth* = false");
    qDebug() << PROJECT_DISPLAY_NAME << "version" << PROJECT_GITREV;

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(u"mango-frunk data feed generator."_s);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption nameOption{u"name"_s, u"Explicit mango frunk name to connect to."_s,
                                  u"name"_s};
    parser.addOption(nameOption);

    QCommandLineOption installOption{u"install"_s, u"Install as a systemd service (LINUX ONLY)."_s};
    parser.addOption(installOption);

    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args << QString(argv[i]);
    }
    parser.process(args);

    QString name;
    if (parser.isSet(nameOption)) {
        name = parser.value(nameOption);
        qDebug() << "Using provided name:" << name;
    }

    if (parser.isSet(installOption)) {
        return installService(argc, argv, name);
    } else {
        return runHeadless(argc, argv, name);
    }
}
