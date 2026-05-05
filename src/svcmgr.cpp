#include "svcmgr.hpp"

#include <cstdlib>
#include <filesystem>

#include <QDebug>
#include <QDir>

using namespace Qt::Literals::StringLiterals;

#define CHECK_INTERVAL 5000
#define SVC_NAME "mango-frunk"
#define SVC_FILE "mango-frunk.service"

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

    auto svcDef = SERVICE_TEMPLATE.arg(SVC_NAME).arg(exePath).arg(u" --headless"_s);
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
    if (std::system("systemctl --user daemon-reload") != 0) {
        qWarning() << "Failed to reload user service definitions!";
    }
    if (std::system("systemctl --user enable --now " SVC_FILE) != 0) {
        qWarning() << "Failed enable service!";
    }
    check();
}

void SvcMgr::uninstallService()
{
    auto dir = QDir::home();
    dir.cd(u".config/systemd/user"_s);
    if (!dir.exists(SVC_FILE)) {
        qInfo() << "Service not installed!";
        return;
    }
    if (std::system("systemctl --user disable --now " SVC_FILE) != 0) {
        qWarning() << "Failed to disable service!";
    }
    dir.remove(SVC_FILE);

    if (std::system("systemctl --user daemon-reload")) {
        qWarning() << "Failed to reload user service definitions!";
    }
    check();
}

void SvcMgr::startService()
{
    if (std::system("systemctl --user start " SVC_FILE) != 0) {
        qWarning() << "Failed start service!";
    }
    check();
}

void SvcMgr::stopService()
{
    if (std::system("systemctl --user stop " SVC_FILE) != 0) {
        qWarning() << "Failed stop service!";
    }
    check();
}

void SvcMgr::check()
{
    int exitCode = std::system("systemctl --no-pager --user is-active " SVC_NAME);
    // service not found
    if (exitCode == 4) {
        m_installed = false;
        m_running = false;
    }
    // service not running
    else if (exitCode == 3) {
        m_installed = true;
        m_running = false;
    }
    // unknown state
    else if (exitCode != 0) {
        m_installed = false;
        m_running = false;
    }
    // all good! installed and running!
    else {
        m_installed = true;
        m_running = true;
    }
    qInfo() << "Service exit code:" << exitCode << "installed:" << m_installed << ", running:" << m_running;
    emit stateChanged();
}
