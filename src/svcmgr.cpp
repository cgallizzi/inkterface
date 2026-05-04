#include "svcmgr.hpp"

#define CHECK_INTERVAL 5000

SvcMgr::SvcMgr(QObject *parent = nullptr)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
    , m_checkProc(new QProcess(this))
{
    connect(m_checkProc, &QProcess::errorOccurred, this, &SvcMgr::onCheckErrorOccurred);
    connect(m_checkProc, &QProcess::finished, this, &SvcMgr::onCheckFinished);
    connect(m_checkProc, &QProcess::readyReadStandardError, this,
            &SvcMgr::onCheckReadyReadStandardError);
    connect(m_checkProc, &QProcess::readyReadStandardOutput, this,
            &SvcMgr::onCheckReadyReadStandardOutput);
    connect(m_checkProc, &QProcess::started, this, &SvcMgr::onCheckStarted);
    connect(m_checkProc, &QProcess::stateChanged, this, &SvcMgr::onCheckStateChanged);

    check();
    m_checkTimer->setSingleShot(false);
    m_checkTimer->setInterval(CHECK_INTERVAL);
    connect(m_checkTimer, &QTimer::timeout, this, &SvcMgr::check);
    m_checkTimer->start();
}

void SvcMgr::installService() {}

void SvcMgr::uninstallService() {}

void SvcMgr::startService() {}

void SvcMgr::stopService() {}

void SvcMgr::check() {}

void SvcMgr::onCheckErrorOccurred(QProcess::ProcessError error) {}

void SvcMgr::onCheckFinished(int exitCode, QProcess::ExitStatus exitStatus) {}

void SvcMgr::onCheckReadyReadStandardError() {}

void SvcMgr::onCheckReadyReadStandardOutput() {}

void SvcMgr::onCheckStarted() {}

void SvcMgr::onCheckStateChanged(QProcess::ProcessState newState) {}
