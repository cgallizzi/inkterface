#ifndef FRUNK_HPP
#define FRUNK_HPP

#include <QLowEnergyController>
#include <QObject>
#include <QTimer>

#include "frunk-finder.hpp"
#include "frunk-state.hpp"

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
    }

  public slots:
    void stop();

  private slots:
    void connCheck();
    void sendState();

    void clearConnection();
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerServicesDiscovered();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);

  private:
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;
    QBluetoothDeviceInfo m_device;

    FrunkFinder *m_ffinder = nullptr;
    FrunkState *m_fstate = nullptr;

    QTimer *m_connTimer = nullptr;
    QTimer *m_sendTimer = nullptr;
    bool m_connecting = false;
    bool m_stopping = false;

    void writeLine(const QUuid &uuid, const QString &value);
    void writeKeyVal(const uint8_t &index, const QString &key, const QString &value);
    void writePoints(const uint8_t &index, const FrunkField *field);
    void flushDisplay();
};

#endif /* FRUNK_HPP */
