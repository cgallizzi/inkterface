#include "frunk-finder.hpp"

#include <QDebug>

#define RSSI_LIMIT -80

using namespace Qt::Literals::StringLiterals;

FrunkInfo::FrunkInfo(const QBluetoothDeviceInfo &info, QObject *parent)
    : QObject(parent)
    , m_info(info)
{
}

FrunkFinder::FrunkFinder(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this,
            &FrunkFinder::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &FrunkFinder::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            &FrunkFinder::onDiscoveryError);
    qDebug() << "Starting discovery!";
    startDiscovery();
}

void FrunkFinder::onDiscoveryEnded()
{
    for (auto frunk : m_frunks) {
        frunk->deleteLater();
    }
    m_frunks.clear();

    for (const auto &info : m_discoveryAgent->discoveredDevices()) {
        if (!info.isValid() || info.isCached() || !info.name().startsWith(u"MANGO"_s)) {
            continue;
        }
        qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
        m_frunks.append(new FrunkInfo(info, this));
    }
    emit frunksChanged();

    startDiscovery();
}

void FrunkFinder::onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "Discovery Error: " << error;
    startDiscovery();
}

void FrunkFinder::startDiscovery()
{
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(10000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
