#include "steam.hpp"

#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QNetworkReply>

#if defined(Q_OS_MACOS)
const static QString STEAM_PFX = QDir::homePath() + "/Library/Application Support/Steam";
#elif defined(Q_OS_WIN)
const static QString STEAM_PFX = "C:/Steam";
#else
const static QString STEAM_PFX = QDir::homePath() + "/.steam/steam";
#endif

using namespace Qt::Literals::StringLiterals;

namespace steam
{

Steam::Steam(QObject *parent)
    : QObject(parent)
    , m_netman(new QNetworkAccessManager(this))
{
}

QString Steam::currentUser(bool account_name)
{
    QByteArray field = account_name ? "AccountName" : "PersonaName";
    QString s;
    QByteArray ba;
    QFile f{STEAM_PFX + "/config/loginusers.vdf"};
    if (f.exists() && f.open(QFile::ReadOnly)) {
        while (true) {
            ba = f.readLine();
            if (ba.contains(field)) {
                break;
            }
        }
        f.close();
        s = QString::fromUtf8(ba).trimmed();
        s = s.sliced(s.indexOf(field) + 12).trimmed();
        auto len = s.length();
        s = s.mid(1, len - 2);
    }
    if (s.contains("hwtestalert")) {
        s = currentUser(true);
    }
    return s;
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
        m_consoleLog->setFileName(STEAM_PFX + "/logs/console_log.txt");
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
    if (m_appCache.contains(appid)) {
        if (emit_start) {
            emit appStarted(m_appCache[appid]);
        } else if (emit_stop) {
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
        emit appStarted(m_appCache[appid]);
    } else if (emit_stop) {
        emit appStopped(m_appCache[appid]);
    } else {
        emit appDetails(m_appCache[appid]);
    }
}

}; /* namespace steam */
