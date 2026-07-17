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

    void clear()
    {
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
    QString currentUserId();
    QString steamVersion();
    QVariantMap libraryFolders();

    int installedAppCount();
    double appPlaytimeMinutes(const QString &appid);
    // returns {unlocked, total}, {-1, -1} when not (yet) known
    QPair<int, int> achievements(const QString &appid);

    QVariantMap appManifest(const QString &appid);
    QString appName(const QString &appid);
    QString appDir(const QString &appid);

    QVariantMap loadVDF(const QString &path);
    QString parseVDF(const QByteArray &data, QVariantMap &output);

    const App &runningApp() const { return m_runningApp; }

  public slots:
    void watchConsoleLog(bool start = false);
    void getAppDetails(QString appid, bool emit_start = false, bool emit_stop = false);
    void fetchAchievements(const QString &appid);

  signals:
    void appStarted(steam::App details);
    void appStopped(steam::App details);
    void appDetails(steam::App details);
    void achievementsUpdated(QString appid);

  private slots:
    void appDetailsReply(QString appid, bool emit_start = false, bool emit_stop = false);
    void achievementsReply(QString appid);

  private:
    QNetworkAccessManager *m_netman = nullptr;
    QTimer *m_consoleTimer = nullptr;
    QFile *m_consoleLog = nullptr;
    QMap<QString, App> m_appCache;
    QMap<QString, QPair<int, int>> m_achievementCache;

    App m_runningApp;

    // walks nested QVariantMaps by key, matching keys case-insensitively
    // since valve's VDF files are inconsistent about casing
    static QVariant vdfGet(const QVariantMap &map, const QStringList &path);
};

}; /* namespace steam */

#endif /* STEAM_HPP */
