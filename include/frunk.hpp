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
    ~Frunk()
    {
        if (m_controller) {
            m_controller->disconnectFromDevice();
        }
        if (m_discoveryAgent) {
            m_discoveryAgent->stop();
        }
    }

  public slots:
    void stop();

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerServicesDiscovered();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceError(QLowEnergyService::ServiceError error);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onReconCheck();

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;

    QTimer *m_reconTimer = nullptr;
    bool m_stopping = false;

    void startDiscovery();
    void writeLine(const QUuid &uuid, const std::string &value);
    void writeKeyVal(const uint16_t &index, const std::string &key, const std::string &value);
    void flushDisplay();
};

#endif /* FRUNK_HPP */
