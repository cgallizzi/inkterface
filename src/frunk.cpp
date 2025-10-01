#include "frunk.hpp"

#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QObject>
#include <QSysInfo>
#include <QUuid>

#define SERVICE_UUID QUuid("95c7b479-8e84-4ce7-a121-faf74bf48c84")
#define TOPLINE_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871900"}
#define MIDLINE_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871901"}
#define BOTLINE_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871902"}
#define KEYVAL_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871903"}
#define VECTOR_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871904"}
#define FLUSH_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a8719FF"}

#define RSSI_LIMIT -48

using namespace Qt::Literals::StringLiterals;

Frunk::Frunk(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
    , m_reconTimer(new QTimer(this))
{
    m_reconTimer->setSingleShot(false);
    m_reconTimer->setInterval(2000);
    connect(m_reconTimer, &QTimer::timeout, this, &Frunk::onReconCheck);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this,
            &Frunk::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &Frunk::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            &Frunk::onDiscoveryError);
    qDebug() << "Starting discovery!";
    startDiscovery();
}

void Frunk::stop()
{
    m_stopping = true;
    if (m_controller) {
        m_controller->disconnectFromDevice();
    }
    if (m_discoveryAgent) {
        m_discoveryAgent->stop();
    }
    QTimer::singleShot(1000, this, [&]() { QCoreApplication::quit(); });
}

void Frunk::onDiscoveryEnded()
{
    bool noController =
        !m_controller || m_controller->state() == QLowEnergyController::UnconnectedState;
    QBluetoothDeviceInfo nearest;
    for (const auto &info : m_discoveryAgent->discoveredDevices()) {
        if (!info.isValid()) {
            continue;
        }
        if (!info.name().startsWith(u"MANGO"_s)) {
            continue;
        }
        if (!nearest.isValid()) {
            nearest = info;
        } else if (info.rssi() < nearest.rssi()) {
            nearest = info;
        }
        qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
    }
    if (noController && nearest.isValid() && nearest.rssi() > RSSI_LIMIT) {
        qDebug() << "Connecting to " << nearest.name();
        m_controller = QLowEnergyController::createCentral(nearest, this);
        connect(m_controller, &QLowEnergyController::stateChanged, this,
                &Frunk::onControllerStateChanged);
        connect(m_controller, &QLowEnergyController::discoveryFinished, this,
                &Frunk::onControllerServicesDiscovered);
        connect(m_controller, &QLowEnergyController::errorOccurred, this,
                &Frunk::onControllerError);
        m_controller->connectToDevice();
        m_reconTimer->start();
    } else if (noController) {
        startDiscovery();
    }
}

void Frunk::onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "Discovery Error: " << error;
}

void Frunk::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    qDebug() << "New controller state: " << state;
    if (state < QLowEnergyController::ConnectedState) {
        // delete any lingering service object that was probably invalidated by
        // a disconnect
        if (m_service) {
            m_service->deleteLater();
            m_service = nullptr;
        }
        return;
    }
    m_controller->discoverServices();
}

void Frunk::onControllerServicesDiscovered()
{
    if (!m_controller || m_controller->state() == QLowEnergyController::UnconnectedState) {
        return;
    }
    auto services = m_controller->services();
    qDebug() << "Discovered " << services.count() << " services!";
    if (m_service) {
        return;
    }
    m_service = m_controller->createServiceObject(SERVICE_UUID, this);
    if (!m_service) {
        qDebug() << "Failed to create service!";
        m_controller->disconnectFromDevice();
        return;
    }
    connect(m_service, &QLowEnergyService::errorOccurred, this, &Frunk::onServiceError);
    connect(m_service, &QLowEnergyService::stateChanged, this, &Frunk::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicRead, this,
            &Frunk::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged, this,
            &Frunk::onCharacteristicChanged);
    m_service->discoverDetails();
}

void Frunk::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "Controller error: " << error;
}

void Frunk::onServiceError(QLowEnergyService::ServiceError error)
{
    qDebug() << "Service error:" << error;
}

void Frunk::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qDebug() << "New service state: " << state;
    if (state != QLowEnergyService::RemoteServiceDiscovered) {
        return;
    }
    qDebug() << "Found " << m_service->characteristics().count() << "characteristics!";

    writeLine(TOPLINE_UUID, QSysInfo::machineHostName().toStdString());
    writeLine(MIDLINE_UUID, "chipolux is signed in");
    writeLine(BOTLINE_UUID, "Playing Silksong (and dying a lot...)");
    writeKeyVal(0, "OS", QSysInfo::productVersion().toStdString());
    flushDisplay();
}

void Frunk::writeLine(const QUuid &uuid, const std::string &value)
{
    auto c = m_service->characteristic(uuid);
    if (c.isValid()) {
        m_service->writeCharacteristic(c, value.substr(0, 42).c_str());
    }
}

void Frunk::writeKeyVal(const uint16_t &index, const std::string &key, const std::string &value)
{
    auto c = m_service->characteristic(KEYVAL_UUID);
    if (c.isValid()) {
        QByteArray ba;
        QDataStream s(&ba, QDataStream::WriteOnly);
        s.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));

        s << index;
        s.writeRawData(key.substr(0, 31).c_str(), 32);
        s.writeRawData(value.substr(0, 31).c_str(), 32);
        m_service->writeCharacteristic(c, ba);
    }
}

void Frunk::flushDisplay()
{
    auto c = m_service->characteristic(FLUSH_UUID);
    if (c.isValid()) {
        m_service->writeCharacteristic(c, "");
    }
}

void Frunk::onCharacteristicChanged(const QLowEnergyCharacteristic &, const QByteArray &value)
{
    qDebug() << "Characteristc changed: " << value;
}

void Frunk::onReconCheck()
{
    if (!m_controller || m_controller->state() == QLowEnergyController::UnconnectedState) {
        m_reconTimer->stop();
        qDebug() << "Lost connection, restarting discovery!";
        startDiscovery();
        return;
    }
    if (m_controller->state() < QLowEnergyController::ConnectedState) {
        return;
    }
    // NOTE: ->readRssi() doesn't work on linux/bluez, let the MCU kick us
}

void Frunk::startDiscovery()
{
    if (m_stopping)
        return;
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(15000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
