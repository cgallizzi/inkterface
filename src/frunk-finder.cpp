#include "frunk-finder.hpp"

#include <QDebug>
#include <algorithm>

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
    QSettings settings;
    auto frunkName = settings.value(u"frunkName"_s).toString();
    auto rssi = settings.value(u"lastRssi"_s).toInt();
    auto ifaceVersion = settings.value(u"lastIfaceVersion"_s).toString().toLatin1();
    if (!frunkName.isEmpty()) {
        QBluetoothDeviceInfo info;
        info.setName(frunkName);
        info.setRssi(rssi);
        info.setManufacturerData(0x055d, ifaceVersion);
        m_placeholderFrunk = new FrunkInfo(info, this);
    }

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

    QSettings settings;
    auto frunkName = settings.value(u"frunkName"_s).toString();
    for (const auto &info : m_discoveryAgent->discoveredDevices()) {
        if (!info.isValid() || info.isCached() || !info.name().startsWith(u"FRUNK-"_s)) {
            continue;
        }
        qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
        auto frunk = new FrunkInfo(info, this);
        m_frunks.append(frunk);

        if (frunk->name() == frunkName) {
            settings.setValue(u"lastRssi"_s, frunk->rssi());
            settings.setValue(u"lastIfaceVersion"_s, frunk->ifaceVersion());
        }
    }
    std::stable_sort(m_frunks.begin(), m_frunks.end(), [](const FrunkInfo *a, const FrunkInfo *b) {
        if (a->supported() != b->supported()) {
            return a->supported();
        }
        if (a->rssi() != b->rssi()) {
            return a->rssi() > b->rssi();
        }
        return a->name() < b->name();
    });
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
