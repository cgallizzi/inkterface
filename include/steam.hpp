#ifndef STEAM_HPP
#define STEAM_HPP

#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

namespace steam
{

struct App {
    QString appid;
    QString name;
};

class Steam : public QObject
{
    Q_OBJECT

  public:
    explicit Steam(QObject *parent = nullptr);

    QString currentUser();

  public slots:
    void watchConsoleLog(bool start = false);
    void getAppDetails(QString appid, bool emit_start = false, bool emit_stop = false);

  signals:
    void appStarted(App details);
    void appStopped(App details);
    void appDetails(App details);

  private slots:
    void appDetailsReply(QString appid, bool emit_start = false, bool emit_stop = false);

  private:
    QNetworkAccessManager *m_netman = nullptr;
    QTimer *m_consoleTimer = nullptr;
    QFile *m_consoleLog = nullptr;
    QMap<QString, App> m_appCache;
};

}; /* namespace steam */

#endif /* STEAM_HPP */
