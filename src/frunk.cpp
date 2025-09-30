#include "frunk.hpp"

#include <QDebug>
#include <QObject>

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

void Frunk::onDiscoveryEnded()
{
    qDebug() << "Discovery ended!";
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
    if (noController && nearest.isValid()) {
        qDebug() << "Connecting to " << nearest.name();
        m_controller = QLowEnergyController::createCentral(nearest, this);
        connect(m_controller, &QLowEnergyController::stateChanged, this,
                &Frunk::onControllerStateChanged);
        connect(m_controller, &QLowEnergyController::serviceDiscovered, this,
                &Frunk::onControllerSvcDiscovered);
        connect(m_controller, &QLowEnergyController::errorOccurred, this,
                &Frunk::onControllerError);
        connect(m_controller, &QLowEnergyController::rssiRead, this, &Frunk::onControllerRssi);
        m_controller->connectToDevice();
        m_reconTimer->start();
    } else if (noController) {
        qDebug() << "Restarting discovery!";
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
}

void Frunk::onControllerSvcDiscovered(const QBluetoothUuid &service)
{
    qDebug() << "Service: " << service;
}

void Frunk::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "Controller error: " << error;
}

void Frunk::onControllerRssi(int16_t rssi) { qDebug() << "Current RSSI: " << rssi; }

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
    m_controller->readRssi();
}

void Frunk::startDiscovery()
{
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(15000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
