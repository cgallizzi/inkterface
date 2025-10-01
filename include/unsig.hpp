#ifndef UNSIG_HPP
#define UNSIG_HPP

#include <QList>
#include <QObject>
#include <QSocketNotifier>

#ifndef Q_OS_WIN
#include <sys/signal.h>
#else
/* this class is effectively disabled on windows, just placeholders to simplify
 * "using" the class elsewhere
 */
#define SIGINT 0
#define SIGTERM 0
#endif

/* Handler for turning Unix signals into Qt signals that can be acted on.
 * Based on https://doc.qt.io/qt-6/unix-signals.html
 */
class UnSig : public QObject
{
    Q_OBJECT

  public:
    explicit UnSig(QObject *parent);

    void catchSignal(int signal);
    static void _signalHandler(int signal);

  signals:
    void unixSignal(int signal);

  private slots:
    void onSignal();

  private:
    static int m_sockpair[2];
    QSocketNotifier *m_notifier;
    QList<int> m_signals;
};

#endif /* UNSIG_HPP */
