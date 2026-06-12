#ifndef PANEL_HPP
#define PANEL_HPP

#include <QLowEnergyController>
#include <QObject>
#include <QTimer>

#include "panel-finder.hpp"
#include "panel-state.hpp"

class Panel : public QObject
{
    Q_OBJECT

  public:
    explicit Panel(QObject *parent = nullptr);
    ~Panel()
    {
        if (m_controller) {
            m_controller->disconnectFromDevice();
        }
    }

    bool isConnected() const
    {
        auto state = m_controller ? m_controller->state() : QLowEnergyController::UnconnectedState;
        return state == QLowEnergyController::ConnectingState ||
               state == QLowEnergyController::ConnectedState ||
               state == QLowEnergyController::DiscoveringState ||
               state == QLowEnergyController::DiscoveredState;
    }

  public slots:
    void stop();

  private slots:
    void connCheck();
    void sendState();

    void clearConnection();
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerServicesDiscovered();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onServiceError(QLowEnergyService::ServiceError error);
    void onServiceCharacteristicWritten(const QLowEnergyCharacteristic &characteristic,
                                        const QByteArray &value);

  private:
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;
    QBluetoothDeviceInfo m_device;

    PanelFinder *m_finder = nullptr;
    PanelState *m_state = nullptr;

    QTimer *m_connTimer = nullptr;
    QTimer *m_sendTimer = nullptr;
    bool m_connecting = false;
    bool m_stopping = false;
    std::chrono::time_point<std::chrono::steady_clock> m_lastComms;

    void writeLine(const QUuid &uuid, const QString &value);
    void writeKeyVal(const uint8_t &index, const QString &key, const QString &value);
    void writePoints(const uint8_t &index, const PanelField *field);
    void flushDisplay();
};

#endif /* PANEL_HPP */
