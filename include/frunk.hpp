#ifndef FRUNK_HPP
#define FRUNK_HPP

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <QTimer>

class Frunk : public QObject
{
    Q_OBJECT

  public:
    explicit Frunk(QObject *parent = nullptr);

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerSvcDiscovered(const QBluetoothUuid &service);
    void onControllerError(QLowEnergyController::Error error);
    void onControllerRssi(int16_t rssi);
    void onReconCheck();

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QTimer *m_reconTimer = nullptr;

    void startDiscovery();
};

#endif /* FRUNK_HPP */
