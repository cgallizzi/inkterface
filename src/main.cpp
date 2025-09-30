#include <iostream>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <QTimer>

#include "config.h"
#include "frunk.hpp"

#define EXIT_SUCCESS 0

using namespace Qt::Literals::StringLiterals;

int runHeadless(int argc, char *argv[], [[maybe_unused]] const QString &name)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(PROJECT_DISPLAY_NAME);
    app.setOrganizationName(ORG_NAME);
    app.setOrganizationDomain(ORG_DOMAIN);
    app.setApplicationVersion(PROJECT_VER);

    Frunk frunk(&app);

    int exitCode = EXIT_SUCCESS;

    // QTimer::singleShot(5000, &app, SLOT(quit()));

    app.exec();
    return exitCode;
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules("qt.bluetooth* = false");
    qDebug() << PROJECT_DISPLAY_NAME << "version" << PROJECT_VER;

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(u"mango-frunk data feed generator."_s);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption nameOption{u"name"_s, u"Explicit mango frunk name to connect to."_s,
                                  u"name"_s};
    parser.addOption(nameOption);

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

    return runHeadless(argc, argv, name);
}
