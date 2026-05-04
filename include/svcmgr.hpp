#ifndef SVCMGR_HPP
#define SVCMGR_HPP

#include <QObject>
#include <QProcess>
#include <QTimer>

class SvcMgr : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isInstalled READ isInstalled NOTIFY stateChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY stateChanged)

  public:
    explicit SvcMgr(QObject *parent = nullptr);
    bool isInstalled() const { return m_installed; }
    bool isRunning() const { return m_running; }

  signals:
    void stateChanged();

  public slots:
    void installService();
    void uninstallService();
    void startService();
    void stopService();
    void check();

  private:
    QTimer *m_checkTimer = nullptr;

    bool m_installed = false;
    bool m_running = false;
};

#endif /* SVCMGR_HPP */
