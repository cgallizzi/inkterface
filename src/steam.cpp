#include "steam.hpp"

namespace steam {

Steam::Steam(QObject* parent)
	: QObject(parent)
{
}

void Steam::getRunningApps()
{
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

}; /* namespace steam */
