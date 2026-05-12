#include "frunk-finder.hpp"

#include <QDebug>
#include <algorithm>

using namespace Qt::Literals::StringLiterals;

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

    updatePlaceholder();
    startDiscovery();
}

void FrunkFinder::onDiscoveryEnded()
{
    if (m_stopping)
        return;

    for (auto frunk : std::as_const(m_frunks)) {
        frunk->deleteLater();
    }
    m_frunks.clear();
    m_frunkFound = false;

    updatePlaceholder();
    auto frunkName = m_placeholderFrunk->name();
    const auto devices = m_discoveryAgent->discoveredDevices();
    for (const auto &info : devices) {
        if (!info.isValid() || info.isCached() || !info.name().startsWith(u"FRUNK-"_s)) {
            continue;
        }
        qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
        auto frunk = new FrunkInfo(info, this);
        m_frunks.append(frunk);

        if (!frunkName.isEmpty() && frunk->name() == frunkName) {
            m_frunkFound = true;
            QSettings settings;
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
    startDiscovery();
    emit frunksChanged();
}

void FrunkFinder::updatePlaceholder()
{
    if (m_placeholderFrunk) {
        m_placeholderFrunk->deleteLater();
        m_placeholderFrunk = nullptr;
    }
    QSettings settings;
    auto frunkName = settings.value(u"frunkName"_s).toString();
    auto rssi = settings.value(u"lastRssi"_s).toInt();
    auto ifaceVersion = settings.value(u"lastIfaceVersion"_s).toString().toLatin1();
    QBluetoothDeviceInfo info;
    if (!frunkName.isEmpty()) {
        info.setName(frunkName);
        info.setRssi(rssi);
        info.setManufacturerData(0x055d, ifaceVersion);
    }
    m_placeholderFrunk = new FrunkInfo(info, this);
}
