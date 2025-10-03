#ifndef STEAM_HPP
#define STEAM_HPP

#include <QObject>

namespace steam {

struct RunningApp {
	int appid;
	int gameid;
	int processid;
	QString commandline;
	QString extrainfo;
	QList<int> associatedpids;
};

class Steam : public QObject {
	Q_OBJECT

	public:
		explicit Steam(QObject* parent=nullptr);

	public slots:
		void getRunningApps();

	signals:
		void runningApps(bool success, QList<RunningApp> apps);
};

}; /* namespace steam */

#endif /* STEAM_HPP */
