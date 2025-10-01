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

    struct KeyVal {
        QString key;
        QString val;
    };
    typedef std::vector<KeyVal> KeyVals;

    struct SystemState {
        QString topLine;
        QString midLine;
        QString botLine;

        KeyVals keyvals{9};

        void reset()
        {
            topLine = "";
            midLine = "";
            botLine = "";
            keyvals.clear();
            keyvals.resize(9);
        }
    } state;

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

    void onUpdate();

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;

    QTimer *m_reconTimer = nullptr;
    QTimer *m_updateTimer = nullptr;
    bool m_stopping = false;

    void startDiscovery();
    void writeLine(const QUuid &uuid, const QString &value);
    void writeKeyVal(const uint16_t &index, const QString &key, const QString &value);
    void flushDisplay();

    void collectSystemState();
    void sendSystemState();
};

#endif /* FRUNK_HPP */
