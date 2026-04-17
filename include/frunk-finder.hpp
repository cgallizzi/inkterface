#ifndef FRUNK_FINDER_HPP
#define FRUNK_FINDER_HPP

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>

class FrunkInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(qint16 rssi READ rssi CONSTANT)

  public:
    explicit FrunkInfo(const QBluetoothDeviceInfo &info, QObject *parent = nullptr);

    QString name() { return m_info.name(); }
    qint16 rssi() { return m_info.rssi(); }

  private:
    QBluetoothDeviceInfo m_info;
};

class FrunkFinder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<FrunkInfo *> frunks MEMBER m_frunks NOTIFY frunksChanged)

  signals:
    void frunksChanged();

  public:
    explicit FrunkFinder(QObject *parent = nullptr);
    ~FrunkFinder()
    {
        if (m_discoveryAgent) {
            m_discoveryAgent->stop();
        }
    }

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QList<FrunkInfo *> m_frunks;

    void startDiscovery();
};

#endif /* FRUNK_FINDER_HPP */
