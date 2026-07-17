#include "steam.hpp"

#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QXmlStreamReader>

using namespace Qt::Literals::StringLiterals;

namespace steam
{

Steam::Steam(QObject *parent)
    : QObject(parent)
    , m_netman(new QNetworkAccessManager(this))
{
}

QString Steam::steamDir()
{
    QString result;
#if defined(Q_OS_MACOS)
    result = QDir::homePath() + u"/Library/Application Support/Steam"_s;
#elif defined(Q_OS_WIN)
    QSettings reg(u"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam"_s,
                  QSettings::Registry64Format);
    result = reg.value(u"InstallPath"_s).toString();
#else
    result = QDir::homePath() + u"/.steam/steam"_s;
#endif
    return result;
}

QString Steam::currentUser(bool account_name)
{
    QString result;
    QVariantMap loginusers = loadVDF(steamDir() + "/config/loginusers.vdf");
    const auto users = loginusers.value("users").toMap();
    for (auto &user : users) {
        if (user.toMap().value("MostRecent").value<QByteArray>() == "1"_ba) {
            result = user.toMap().value(account_name ? "AccountName" : "PersonaName").toString();
            break;
        }
    }
    return result;
}

QString Steam::currentUserId()
{
    QString result;
    QVariantMap loginusers = loadVDF(steamDir() + "/config/loginusers.vdf");
    const auto users = loginusers.value("users").toMap();
    for (auto it = users.cbegin(), end = users.cend(); it != end; ++it) {
        if (it.value().toMap().value("MostRecent").value<QByteArray>() == "1"_ba) {
            result = it.key();
            break;
        }
    }
    return result;
}

QVariant Steam::vdfGet(const QVariantMap &map, const QStringList &path)
{
    QVariant node = map;
    for (const auto &key : path) {
        const auto sub = node.toMap();
        auto it = sub.cbegin();
        const auto end = sub.cend();
        for (; it != end; ++it) {
            if (it.key().compare(key, Qt::CaseInsensitive) == 0) {
                break;
            }
        }
        if (it == end) {
            return {};
        }
        node = it.value();
    }
    return node;
}

double Steam::appPlaytimeMinutes(const QString &appid)
{
    const QString steam64 = currentUserId();
    if (steam64.isEmpty() || appid.isEmpty()) {
        return -1;
    }
    // account id is the low 32 bits of the steam64 id
    const quint32 accountid = quint32(steam64.toULongLong() & 0xFFFFFFFFULL);
    const QString path =
        u"%1/userdata/%2/config/localconfig.vdf"_s.arg(steamDir(), QString::number(accountid));
    const QVariantMap config = loadVDF(path);
    const QVariant playtime =
        vdfGet(config, {u"UserLocalConfigStore"_s, u"Software"_s, u"Valve"_s, u"Steam"_s,
                        u"apps"_s, appid, u"Playtime"_s});
    if (!playtime.isValid()) {
        return -1;
    }
    return playtime.toDouble();
}

QPair<int, int> Steam::achievements(const QString &appid)
{
    return m_achievementCache.value(appid, {-1, -1});
}

void Steam::fetchAchievements(const QString &appid)
{
    const QString steam64 = currentUserId();
    if (steam64.isEmpty() || appid.isEmpty()) {
        return;
    }
    // the community endpoint needs no API key, it just requires the user's
    // profile (and game details) to be public; when it isn't we cache a miss
    // and the readout stays at "--"
    auto req = QNetworkRequest(u"https://steamcommunity.com/profiles/%1/stats/%2/achievements/"
                               u"?xml=1"_s.arg(steam64, appid));
    auto reply = m_netman->get(req);
    QTimer::singleShot(10000, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, [this, appid]() { achievementsReply(appid); });
}

void Steam::achievementsReply(QString appid)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    reply->deleteLater();
    if (!reply->isOpen() || reply->error() != QNetworkReply::NoError) {
        qWarning() << "Getting achievements failed:" << reply->errorString();
        return;
    }
    int unlocked = 0;
    int total = 0;
    QXmlStreamReader xml(reply->readAll());
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement) {
            continue;
        }
        if (xml.name() == u"achievement"_s) {
            total += 1;
            if (xml.attributes().value(u"closed"_s) == u"1"_s) {
                unlocked += 1;
            }
        }
    }
    if (total == 0) {
        qDebug() << "no achievements visible for" << appid << "(none, or private profile)";
        return;
    }
    qDebug() << "achievements for" << appid << ":" << unlocked << "/" << total;
    m_achievementCache[appid] = {unlocked, total};
    emit achievementsUpdated(appid);
}

QString Steam::steamVersion()
{
    QDir d(steamDir() + "/package");
    QFile f;
    QByteArray ba;
    const auto entries = d.entryList({{"*.manifest"}});
    for (auto &entry : entries) {
        f.setFileName(d.absoluteFilePath(entry));
        if (f.exists() && f.open(QFile::ReadOnly)) {
            while (true) {
                ba = f.readLine();
                if (ba.contains("version")) {
                    break;
                }
            }
            f.close();
            ba = ba.trimmed().last(14);
            ba.replace('"', ' ');
            return QString::fromUtf8(ba).trimmed();
        }
        break;
    }
    return {};
}

QVariantMap Steam::appManifest(const QString &appid)
{
    QVariantMap result;
    const QVariantMap libFolders = libraryFolders();
    for (const auto &folder : libFolders) {
        const auto &path = folder.toMap().value(u"path"_s);
        const auto &apps = folder.toMap().value(u"apps"_s);
        if (apps.toMap().contains(appid)) {
            result = loadVDF(u"%1/steamapps/appmanifest_%2.acf"_s.arg(path.toString(), appid))
                         .value(u"AppState"_s)
                         .toMap();
            // NOTE: we inject this as it is useful context when interacting
            //       with an app
            result[u"librarydir"_s] = path;
            break;
        }
    }
    return result;
}

int Steam::installedAppCount()
{
    int result = 0;
    const auto libFolders = libraryFolders();
    for (const auto &folder : libFolders) {
        const auto &libPath = folder.toMap().value(u"path"_s).toString();
        auto libDir = QDir(u"%1/steamapps"_s.arg(libPath));
        auto manifests = libDir.entryList({u"appmanifest_*.acf"_s});
        for (auto it = manifests.cbegin(), end = manifests.cend(); it != end; ++it) {
            const auto manifestPath = u"%1/steamapps/%2"_s.arg(libPath, *it);
            if (!QFile::exists(manifestPath)) {
                continue;
            }
            auto manifest = loadVDF(manifestPath).value(u"AppState"_s).toMap();
            if (manifest.value(u"InstalledDepots"_s).toMap().isEmpty()) {
                continue;
            }
            result += 1;
        }
    }
    return result;
}

QString Steam::appName(const QString &appid)
{
    QVariantMap manifest = appManifest(appid);
    return manifest.value("name").toString();
}

QString Steam::appDir(const QString &appid)
{
    QString result;
    QVariantMap manifest = appManifest(appid);
    const auto libDir = manifest.value(u"librarydir"_s).toString();
    const auto installDir = manifest.value(u"installdir"_s).toString();
    if (!installDir.isEmpty()) {
        result = u"%1/steamapps/common/%2"_s.arg(libDir, installDir);
    }
    return result;
}

QVariantMap Steam::libraryFolders()
{
    QVariantMap result = loadVDF(u"%1/config/libraryfolders.vdf"_s.arg(steamDir()));
    return result.value(u"libraryfolders"_s).toMap();
}

QVariantMap Steam::loadVDF(const QString &path)
{
    QVariantMap result;
    QFile f(path);
    if (!f.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open:" << f.fileName();
        return result;
    }
    const QString error = parseVDF(f.readAll(), result);
    if (!error.isEmpty()) {
        qWarning() << "Failed to parse:" << f.fileName() << error;
    }
    f.close();
    return result;
}

QString Steam::parseVDF(const QByteArray &data, QVariantMap &output)
{
    QByteArray token;
    QList<QVariantMap> stack;
    QList<QByteArray> tokens;
    bool inEscape = false;
    bool inToken = false;
    bool haveKey = false;
    for (qsizetype i = 0; i < data.size(); ++i) {
        const auto c = data[i];
        if (inEscape) {
            token.append(c);
            inEscape = false;
            continue;
        } else if (c == '\\') {
            inEscape = true;
            continue;
        }
        if (inToken && c != '"') {
            token.append(c);
            continue;
        }
        switch (c) {
        // handle start/end of token (key or value)
        case '"': {
            if (inToken && haveKey) {
                if (stack.isEmpty()) {
                    return u"No stack frames to insert kv pair at %1"_s.arg(i);
                }
                stack.last().insert(tokens.takeLast(), token);
                token.clear();
                haveKey = false;
            } else if (inToken) {
                tokens.append(token);
                token.clear();
                haveKey = true;
            }
            inToken = !inToken;
            break;
        }
        case '{': {
            if (!haveKey) {
                return u"Missing key for scope starting at %1"_s.arg(i);
            }
            stack.append(QVariantMap());
            haveKey = false;
            break;
        }
        case '}': {
            if (tokens.isEmpty()) {
                return u"Missing key for scope ending at %1"_s.arg(i);
            }
            if (stack.isEmpty()) {
                return u"Missing stack frame for scope ending at %1"_s.arg(i);
            }
            if (stack.count() > 1) {
                QVariantMap frame = stack.takeLast();
                stack.last().insert(tokens.takeLast(), frame);
            } else {
                output.insert(tokens.takeLast(), stack.takeLast());
            }
            break;
        }
        default: {
            break;
        }
        }
    }
    if (!tokens.isEmpty()) {
        return u"Leftover tokens at EOF: %1"_s.arg(tokens.count());
    }
    if (!stack.isEmpty()) {
        return u"Leftover stack at EOF: %1"_s.arg(stack.count());
    }
    return {};
}

void Steam::watchConsoleLog(bool start)
{
    if (!start && (m_consoleLog == nullptr || !m_consoleLog->isOpen())) {
        if (m_consoleTimer) {
            m_consoleTimer->stop();
        }
        return;
    } else if (m_consoleLog == nullptr || !m_consoleLog->isOpen()) {
        if (m_consoleLog) {
            m_consoleLog->deleteLater();
        }
        m_consoleLog = new QFile(this);
        m_consoleLog->setFileName(steamDir() + "/logs/console_log.txt");
        if (!m_consoleLog->open(QFile::ReadOnly)) {
            qDebug() << "Failed to open console_log.txt!";
            return;
        }
        if (!m_consoleTimer) {
            m_consoleTimer = new QTimer(this);
            m_consoleTimer->setSingleShot(false);
            m_consoleTimer->setInterval(1000);
            connect(m_consoleTimer, &QTimer::timeout, this, [&]() { watchConsoleLog(); });
        }
        m_consoleTimer->start();
    }

    QString lastAppId;
    bool appChanged = false;
    while (!m_consoleLog->atEnd()) {
        auto line = m_consoleLog->readLine();
        if (line.contains("] Game process added")) {
            line.slice(line.indexOf(": AppID ") + 8);
            line.truncate(line.indexOf(" "));
            qDebug() << "saw start for:" << line;
            if (line != m_runningApp.appid) {
                lastAppId = line;
                m_runningApp.clear();
                m_runningApp.appid = line;
                appChanged = true;
            }
        } else if (line.contains("] Game process removed")) {
            line.slice(line.indexOf(": AppID ") + 8);
            line.truncate(line.indexOf(" "));
            if (line == m_runningApp.appid) {
                qDebug() << "saw stop for:" << line;
                lastAppId = line;
                m_runningApp.clear();
                appChanged = true;
            }
        }
    }
    if (appChanged) {
        getAppDetails(lastAppId, !m_runningApp.appid.isEmpty(),
                      m_runningApp.appid.isEmpty() && !lastAppId.isEmpty());
    }

    // "[2025-10-06 12:19:18] CAPIJobRequestUserStats - Server response failed 2\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to RunningInstallScript with \"\"\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to SynchronizingCloud with \"\"\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to SynchronizingStats with \"\"\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to ShowInterstitials with \"\"\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp waiting for user response to ShowInterstitials \"\"\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp continues with user response \"ShowInterstitials\"\r\n"
    // "[2025-10-06 12:19:35] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to SynchronizingControllerConfig with \"\"\r\n"
    // "[2025-10-06 12:19:36] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to SiteLicenseSeatCheckout with \"\"\r\n"
    // "[2025-10-06 12:19:36] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to WaitForStreamingEncoderConfig with \"\"\r\n"
    // "[2025-10-06 12:19:36] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to CreatingProcess with \"\"\r\n"
    // "[2025-10-06 12:19:36] GameAction [AppID 736260, ActionID 1] : LaunchApp waiting for user response to CreatingProcess \"\"\r\n"
    // "[2025-10-06 12:19:37] GameAction [AppID 736260, ActionID 1] : LaunchApp continues with user response \"CreatingProcess\"\r\n"
    // "[2025-10-06 12:19:37] Game process added : AppID 736260 \"'/Users/nakyle/Library/Application Support/Steam/steamapps/common/Baba Is You/Baba Is You.app/Contents/MacOS/Chowdren'\", ProcID 14392, IP 0.0.0.0:0\r\n"
    // "[2025-10-06 12:19:37] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to WaitingGameWindow with \"\"\r\n"
    // "[2025-10-06 12:19:37] GameAction [AppID 736260, ActionID 1] : LaunchApp changed task to Completed with \"\"\r\n"
    // "[2025-10-06 12:19:37] Game process updated : AppID 736260 \"'/Users/nakyle/Library/Application Support/Steam/steamapps/common/Baba Is You/Baba Is You.app/Contents/MacOS/Chowdren'\", ProcID 14392, IP 0.0.0.0:0\r\n"
    // "[2025-10-06 12:19:40] GameOverlay: started '/Users/nakyle/Library/Application Support/Steam/Steam.AppBundle/Steam/Contents/MacOS/gameoverlayui' (pid 14406) for game process 14392\r\n"
    // "[2025-10-06 12:19:42] CAPIJobRequestUserStats - Server response failed 2\r\n"
    // "[2025-10-06 12:19:57] Game process removed: AppID 736260 \"'/Users/nakyle/Library/Application Support/Steam/steamapps/common/Baba Is You/Baba Is You.app/Contents/MacOS/Chowdren'\", ProcID 14392 \r\n"
    /*
	 * open {steam dir}/logs/console_log.txt, seek to it's end
	 * call {steam exe} steam:+apps_running
	 * read back from console log
	 * 	looking for lines like so:
	 * 	[2025-10-03 01:22:09] ExecCommandLine: "'~/.steam/root/ubuntu12_32/steam' '-steamdeck' 'steam:+apps_running'"
	 *      [2025-10-03 01:22:09]  - AppID 431730, GameID 431730, ProcessID 625807, IP:Port 0.0.0.0:0
         *         commandline "/home/deck/.local/share/Steam/ubuntu12_32/steam-launch-wrapper -- /home/deck/.local/share/Steam/ubuntu12_32/reaper SteamLaunch AppId=431730 -- '/home/deck/.local/share/Steam/steamapps/common/SteamLinuxRuntime_sniper'/_v2-entry-point --verb=waitforexitandrun -- '/home/deck/.local/share/Steam/steamapps/common/Proton 9.0 (Beta)'/proton waitforexitandrun  '/home/deck/.local/share/Steam/steamapps/common/Aseprite/Aseprite.exe'"
         *         extra info ""
         *         associated PIDs (  625624,625732,625807, )
         *         dwLastIsRunningCheck 269542573
         *      [2025-10-03 01:22:09]  - AppID 413150, GameID 413150, ProcessID 630431, IP:Port 0.0.0.0:0
         *         commandline "/home/deck/.local/share/Steam/ubuntu12_32/steam-launch-wrapper -- /home/deck/.local/share/Steam/ubuntu12_32/reaper SteamLaunch AppId=413150 -- '/home/deck/.local/share/Steam/steamapps/common/SteamLinuxRuntime_sniper'/_v2-entry-point --verb=waitforexitandrun -- '/home/deck/.local/share/Steam/steamapps/common/Proton - Experimental'/proton waitforexitandrun  '/home/deck/.local/share/Steam/steamapps/common/Stardew Valley/Stardew Valley.exe'"
         *         extra info ""
         *         associated PIDs (  630220,630221,630354,630356,630362,630365,630374,630379,630386,630413,630431, )
         *         dwLastIsRunningCheck 269542524
	 * as we see each app info section, add it to a list of running apps.
	 * there is no indicator that the list is complete other than just no more output to console.
	 * we should start a timer and bump it each time we see a new entry, then when it expires we
	 * emit the list of running apps.
	 */
}

void Steam::getAppDetails(QString appid, bool emit_start, bool emit_stop)
{
    // try and quickly load app details from manifest before falling back to
    // loading them from the steam api
    if (!m_appCache.contains(appid)) {
        auto name = appName(appid);
        if (!name.isEmpty()) {
            qDebug() << "loaded appid" << appid << "name from manifest:" << name;
            m_appCache[appid] = {
                .appid = appid,
                .name = name,
            };
        }
    }

    if (m_appCache.contains(appid)) {
        if (emit_start) {
            m_runningApp.appid = appid;
            m_runningApp.name = m_appCache[appid].name;
            emit appStarted(m_appCache[appid]);
        } else if (emit_stop) {
            m_runningApp.clear();
            emit appStopped(m_appCache[appid]);
        } else {
            emit appDetails(m_appCache[appid]);
        }
        return;
    }

    auto req =
        QNetworkRequest(u"https://store.steampowered.com/api/appdetails/?appids=%1"_s.arg(appid));
    auto reply = m_netman->get(req);
    QTimer::singleShot(5000, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this,
            [&, appid, emit_start, emit_stop]() { appDetailsReply(appid, emit_start, emit_stop); });
}

void Steam::appDetailsReply(QString appid, bool emit_start, bool emit_stop)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    if (!reply->isOpen() || reply->error() != QNetworkReply::NoError) {
        QString message = reply->errorString();
        qWarning() << "Getting app details failed:" << message;
        return;
    }
    QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
    response = response.value(appid).toObject();
    if (!response.value("success").toBool()) {
        qWarning() << "Steam rejected app details query:" << response;
        return;
    }

    m_appCache[appid] = {
        .appid = appid,
        .name = response.value("data").toObject().value("name").toString(),
    };
    if (emit_start) {
        m_runningApp.appid = appid;
        m_runningApp.name = m_appCache[appid].name;
        emit appStarted(m_appCache[appid]);
    } else if (emit_stop) {
        m_runningApp.clear();
        emit appStopped(m_appCache[appid]);
    } else {
        emit appDetails(m_appCache[appid]);
    }
}

}; /* namespace steam */
