#include "frunk.hpp"

#include <QDebug>
#include <QObject>

Frunk::Frunk(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this,
            &Frunk::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &Frunk::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            &Frunk::onDiscoveryError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this,
            &Frunk::onDiscoveryDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this,
            &Frunk::onDiscoveryUpdated);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(60000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void Frunk::onDiscoveryEnded() { qDebug() << "Discovery Ended"; }

void Frunk::onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "Discovery Error: " << error;
}

void Frunk::onDiscoveryDiscovered(const QBluetoothDeviceInfo &info)
{
    qDebug() << "Discovery Discovered: " << info.name();
}

void Frunk::onDiscoveryUpdated(const QBluetoothDeviceInfo &info,
                               QBluetoothDeviceInfo::Fields updatedFields)
{
    qDebug() << "Discovery Updated: " << info.name() << ", fields: " << updatedFields;
}
