#include "steam.hpp"

#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>

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
        const auto &libPath = folder.toMap().value(u"path"_s);
        const auto &apps = folder.toMap().value(u"apps"_s).toMap();
        for (auto it = apps.cbegin(), end = apps.cend(); it != end; ++it) {
            const auto manifestPath =
                u"%1/steamapps/appmanifest_%2.acf"_s.arg(libPath.toString(), it.key());
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
        m_consoleLog->seek(m_consoleLog->size());
        if (!m_consoleTimer) {
            m_consoleTimer = new QTimer(this);
            m_consoleTimer->setSingleShot(false);
            m_consoleTimer->setInterval(250);
            connect(m_consoleTimer, &QTimer::timeout, this, [&]() { watchConsoleLog(); });
        }
        m_consoleTimer->start();
    }

    while (!m_consoleLog->atEnd()) {
        auto line = m_consoleLog->readLine();
        if (line.contains("] Game process added")) {
            line.slice(line.indexOf(": AppID ") + 8);
            line.truncate(line.indexOf(" "));
            getAppDetails(line, true, false);
        } else if (line.contains("] Game process removed")) {
            line.slice(line.indexOf(": AppID ") + 8);
            line.truncate(line.indexOf(" "));
            getAppDetails(line, false, true);
        }
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
