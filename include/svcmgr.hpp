#ifndef SVCMGR_HPP
#define SVCMGR_HPP

#include <QObject>
#include <QProcess>
#include <QTimer>

class SvcMgr : public QObject
{
    Q_OBJECT

  public:
    explicit SvcMgr(QObject *parent = nullptr);

  public slots:
    void installService();
    void uninstallService();
    void startService();
    void stopService();
    void check();

  private slots:
    void onCheckErrorOccurred(QProcess::ProcessError error);
    void onCheckFinished(int exitCode, QProcess::ExitStatus exitStatus = NormalExit);
    void onCheckReadyReadStandardError();
    void onCheckReadyReadStandardOutput();
    void onCheckStarted();
    void onCheckStateChanged(QProcess::ProcessState newState);

  private:
    QTimer *m_checkTimer = nullptr;
    QProcess *m_checkProc = nullptr;
};

#endif /* SVCMGR_HPP */
