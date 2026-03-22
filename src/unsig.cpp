#include "unsig.hpp"

#include <QDebug>

#ifndef Q_OS_WIN
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

UnSig::UnSig(QObject *parent)
    : QObject{parent}
    , m_notifier(nullptr)
    , m_signals()
{
#ifndef Q_OS_WIN
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sockpair)) {
        qFatal("UnSig socketpair error: %s", ::strerror(errno));
    }
    m_notifier = new QSocketNotifier(m_sockpair[1], QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &UnSig::onSignal);
    m_notifier->setEnabled(true);
#endif
}

#ifndef Q_OS_WIN
int UnSig::m_sockpair[2];
#endif

void UnSig::catchSignal(int signal)
{
#ifndef Q_OS_WIN
    if (m_signals.contains(signal)) {
        qWarning() << "Already catching signal, ignoring:" << signal;
        return;
    }
    struct sigaction sigact;
    sigact.sa_handler = UnSig::_signalHandler;
    sigact.sa_flags = 0;
    sigact.sa_flags |= SA_RESTART;
    sigemptyset(&sigact.sa_mask);
    if (sigaction(signal, &sigact, NULL)) {
        qFatal("UnSig sigaction error: %s", ::strerror(errno));
    }
    m_signals.append(signal);
#else
    Q_UNUSED(signal)
#endif
}

void UnSig::_signalHandler(int signal)
{
#ifndef Q_OS_WIN
    (void)!::write(m_sockpair[0], &signal, sizeof(signal));
#else
    Q_UNUSED(signal)
#endif
}

void UnSig::onSignal()
{
#ifndef Q_OS_WIN
    m_notifier->setEnabled(false);
    int signal;
    (void)!::read(m_sockpair[1], &signal, sizeof(signal));
    qDebug() << "UnSig caught signal:" << signal;
    emit unixSignal(signal);
    m_notifier->setEnabled(true);
#endif
}
