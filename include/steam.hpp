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

    void clear() {
        appid.clear();
        name.clear();
    }
};

class Steam : public QObject
{
    Q_OBJECT

  public:
    explicit Steam(QObject *parent = nullptr);

    QString steamDir();
    QString currentUser(bool account_name = false);
    QString steamVersion();
    QVariantMap libraryFolders();

    int installedAppCount();

    QVariantMap appManifest(const QString &appid);
    QString appName(const QString &appid);
    QString appDir(const QString &appid);

    QVariantMap loadVDF(const QString &path);
    QString parseVDF(const QByteArray &data, QVariantMap &output);

    const App& runningApp() const { return m_runningApp; }

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

    App m_runningApp;
};

}; /* namespace steam */

#endif /* STEAM_HPP */
