#ifndef FRUNK_HPP
#define FRUNK_HPP

#include <QBluetoothDeviceDiscoveryAgent>
#include <QObject>

class Frunk : public QObject
{
    Q_OBJECT

  public:
    explicit Frunk(QObject *parent = nullptr);

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onDiscoveryDiscovered(const QBluetoothDeviceInfo &info);
    void onDiscoveryUpdated(const QBluetoothDeviceInfo &info,
                            QBluetoothDeviceInfo::Fields updatedFields);

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
};

#endif /* FRUNK_HPP */
