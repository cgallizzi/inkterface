#ifndef SVCMGR_HPP
#define SVCMGR_HPP

#include <QObject>

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
};

#endif /* SVCMGR_HPP */

