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
    void onControllerStateChanged();
    void onControllerSvcChanged();

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController* m_controller = nullptr;
};

#endif /* FRUNK_HPP */
