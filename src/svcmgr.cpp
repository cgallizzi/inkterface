#include "svcmgr.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#endif

using namespace Qt::Literals::StringLiterals;

#define CHECK_INTERVAL 5000
#define SVC_NAME "mango-frunk"
#define SVC_FILE "mango-frunk.service"
#define SYSTEMCTL u"/usr/bin/systemctl"_s

static const QString SERVICE_TEMPLATE{u"[Unit]\n"_s
                                      u"Description=%1\n"_s
                                      u"After=network.target\n"_s
                                      u"\n"_s
                                      u"[Service]\n"_s
                                      u"Type=simple\n"_s
                                      u"ExecStart=%2%3\n"_s
                                      u"Restart=on-failure\n"_s
                                      u"\n"_s
                                      u"[Install]\n"_s
                                      u"WantedBy=default.target\n"_s};

SvcMgr::SvcMgr(QObject *parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
{
    check();
    m_checkTimer->setSingleShot(false);
    m_checkTimer->setInterval(CHECK_INTERVAL);
    connect(m_checkTimer, &QTimer::timeout, this, &SvcMgr::check);
    m_checkTimer->start();
}

void SvcMgr::installService()
{
#if defined(Q_OS_LINUX)
    auto exePath = QString(std::getenv("APPIMAGE"));
    if (exePath.isEmpty()) {
        try {
            exePath = QString(std::filesystem::canonical("/proc/self/exe").c_str());
        } catch (const std::exception &ex) {
            exePath = u""_s;
        }
    }
    if (exePath.isEmpty()) {
        qWarning() << "Can only install service on linux hosts!";
        return;
    }
    qInfo() << "using" << exePath << "for service";

    auto svcDef = SERVICE_TEMPLATE.arg(SVC_NAME, exePath, u" --headless"_s);
    auto dir = QDir::home();
    if (!dir.mkpath(u".config/systemd/user"_s)) {
        qWarning() << "Failed to create systemd user config directory!";
        return;
    }
    dir.cd(u".config/systemd/user"_s);
    if (dir.exists(SVC_FILE)) {
        qWarning() << "Service already installed, uninstall first!";
        return;
    }
    QFile f{dir.filePath(SVC_FILE)};
    if (!f.open(QFile::WriteOnly)) {
        qWarning() << "Failed to create systemd user service definition!";
        return;
    }
    f.write(svcDef.toLatin1());
    f.close();
    qDebug() << "wrote svc def to" << f.fileName() << ", def:" << svcDef;

    // Create a short-lived private connection to the session bus
    auto connName = u"frunkmanager-%1"_s.arg(QCoreApplication::applicationPid());
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connName);
    if (!bus.isConnected()) {
        qWarning() << "Failed to connect to session bus";
        return;
    }

    {
        QDBusInterface systemd("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager", bus);
        if (!systemd.isValid()) {
            qWarning() << "Failed to create systemd interface";
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        // systemctl --user daemon-reload
        QDBusReply<void> reloadReply = systemd.call("Reload");
        if (!reloadReply.isValid()) {
            qWarning() << "Reload failed:" << reloadReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        // systemctl --user enable something.service
        QString unit = SVC_FILE;
        QList<QString> files{unit};
        QDBusReply<void> enableReply = systemd.call("EnableUnitFiles", files,
                                                    false, // runtime
                                                    true   // force
        );
        if (!enableReply.isValid()) {
            qWarning() << "Enable failed:" << enableReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        // systemctl --user start something.service
        QDBusReply<QDBusObjectPath> startReply = systemd.call("StartUnit", unit, "replace");
        if (!startReply.isValid()) {
            qWarning() << "Start failed:" << startReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        qDebug() << "Started job:" << startReply.value().path();
    }
    // Explicitly destroy the temporary connection
    QDBusConnection::disconnectFromBus(connName);
#endif
}

void SvcMgr::uninstallService()
{
#if defined(Q_OS_LINUX)
    QString unit = SVC_FILE;
    QList<QString> files{unit};
    auto connName = u"frunkmanager-%1"_s.arg(QCoreApplication::applicationPid());
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connName);
    if (!bus.isConnected()) {
        qWarning() << "Failed to connect to session bus";
        check();
        return;
    }
    {
        QDBusInterface systemd("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager", bus);
        if (!systemd.isValid()) {
            qWarning() << "Failed to create systemd interface";
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        // systemctl --user stop something.service
        QDBusReply<QDBusObjectPath> stopReply = systemd.call("StopUnit", unit, "replace");
        if (!stopReply.isValid()) {
            qWarning() << "Stop failed:" << stopReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        // systemctl --user disable something.service
        QDBusReply<void> disableReply = systemd.call("DisableUnitFiles", files, false);
        if (!disableReply.isValid()) {
            qWarning() << "Disable failed:" << disableReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        auto dir = QDir::home();
        dir.cd(u".config/systemd/user"_s);
        if (dir.exists(SVC_FILE)) {
            dir.remove(SVC_FILE);
        }

        // systemctl --user daemon-reload
        QDBusReply<void> reloadReply = systemd.call("Reload");
        if (!reloadReply.isValid()) {
            qWarning() << "Reload failed:" << reloadReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }
    }
    QDBusConnection::disconnectFromBus(connName);
    check();
#endif
}

void SvcMgr::startService()
{
#if defined(Q_OS_LINUX)
    QString unit = SVC_FILE;
    auto connName = u"frunkmanager-%1"_s.arg(QCoreApplication::applicationPid());
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connName);
    if (!bus.isConnected()) {
        qWarning() << "Failed to connect to session bus";
        check();
        return;
    }
    {
        QDBusInterface systemd("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager", bus);
        if (!systemd.isValid()) {
            qWarning() << "Failed to create systemd interface";
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        // systemctl --user enable something.service
        QString unit = SVC_FILE;
        QList<QString> files{unit};
        QDBusReply<void> enableReply = systemd.call("EnableUnitFiles", files,
                                                    false, // runtime
                                                    true   // force
        );
        if (!enableReply.isValid()) {
            qWarning() << "Enable failed:" << enableReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        // systemctl --user start something.service
        QDBusReply<QDBusObjectPath> startReply = systemd.call("StartUnit", unit, "replace");
        if (!startReply.isValid()) {
            qWarning() << "Start failed:" << startReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        qDebug() << "Started job:" << startReply.value().path();
    }
    QDBusConnection::disconnectFromBus(connName);
    check();
#endif
}

void SvcMgr::stopService()
{
#if defined(Q_OS_LINUX)
    QString unit = SVC_FILE;
    auto connName = u"frunkmanager-%1"_s.arg(QCoreApplication::applicationPid());
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connName);
    if (!bus.isConnected()) {
        qWarning() << "Failed to connect to session bus";
        check();
        return;
    }
    {
        QDBusInterface systemd("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager", bus);
        if (!systemd.isValid()) {
            qWarning() << "Failed to create systemd interface";
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        // systemctl --user stop something.service
        QDBusReply<QDBusObjectPath> reply = systemd.call("StopUnit", unit, "replace");
        if (!reply.isValid()) {
            qWarning() << "Stop failed:" << reply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            check();
            return;
        }

        qDebug() << "Stopped job:" << reply.value().path();
    }
    QDBusConnection::disconnectFromBus(connName);
    check();
#endif
}

void SvcMgr::check()
{
#if defined(Q_OS_LINUX)
    auto connName = u"frunkmanager-%1"_s.arg(QCoreApplication::applicationPid());
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connName);
    if (!bus.isConnected()) {
        qWarning() << "Failed to connect to session bus";
        return;
    }

    QString unit = SVC_FILE;
    QString state;
    {
        QDBusInterface manager("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager", bus);
        if (!manager.isValid()) {
            qWarning() << "Failed to create manager interface";
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        // Get the object path for the unit
        QDBusReply<QDBusObjectPath> unitReply = manager.call("GetUnit", unit);
        if (!unitReply.isValid()) {
            qWarning() << "GetUnit failed:" << unitReply.error().message();
            m_installed = false;
            m_running = false;
            QDBusConnection::disconnectFromBus(connName);
            emit stateChanged();
            return;
        }
        m_installed = true;

        // Interface for the specific unit
        QDBusInterface unitIface("org.freedesktop.systemd1", unitReply.value().path(),
                                 "org.freedesktop.DBus.Properties", bus);
        if (!unitIface.isValid()) {
            qWarning() << "Failed to create unit interface";
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        // Read ActiveState property
        QDBusReply<QVariant> propReply =
            unitIface.call("Get", "org.freedesktop.systemd1.Unit", "ActiveState");
        if (!propReply.isValid()) {
            qWarning() << "Property read failed:" << propReply.error().message();
            QDBusConnection::disconnectFromBus(connName);
            return;
        }

        state = propReply.value().toString();
    }
    qInfo() << "Read state:" << state;
    m_running = state == u"active"_s || state == u"activating"_s;

    // if we have a service file that also is "installed"
    auto dir = QDir::home();
    dir.cd(u".config/systemd/user"_s);
    m_installed = dir.exists(SVC_FILE);

    qInfo() << "Service installed:" << m_installed << ", running:" << m_running;
    emit stateChanged();
#endif
}
