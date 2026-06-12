#include "panel-finder.hpp"

#include <QDebug>
#include <algorithm>

using namespace Qt::Literals::StringLiterals;

PanelFinder::PanelFinder(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this,
            &PanelFinder::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &PanelFinder::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            &PanelFinder::onDiscoveryError);

    updatePlaceholder();
    startDiscovery();
}

void PanelFinder::onDiscoveryEnded()
{
    if (m_stopping)
        return;

    for (auto panel : std::as_const(m_panels)) {
        panel->deleteLater();
    }
    m_panels.clear();
    m_panelFound = false;

    updatePlaceholder();
    auto panelName = m_placeholderPanel->name();
    const auto devices = m_discoveryAgent->discoveredDevices();
    for (const auto &info : devices) {
        if (!info.isValid() || info.isCached() || !info.name().startsWith(u"INKTF-"_s)) {
            continue;
        }
        qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
        auto panel = new PanelInfo(info, this);
        m_panels.append(panel);

        if (!panelName.isEmpty() && panel->name() == panelName) {
            m_panelFound = true;
            QSettings settings;
            settings.setValue(u"lastRssi"_s, panel->rssi());
            settings.setValue(u"lastIfaceVersion"_s, panel->ifaceVersion());
        }
    }
    std::stable_sort(m_panels.begin(), m_panels.end(), [](const PanelInfo *a, const PanelInfo *b) {
        if (a->supported() != b->supported()) {
            return a->supported();
        }
        if (a->rssi() != b->rssi()) {
            return a->rssi() > b->rssi();
        }
        return a->name() < b->name();
    });
    startDiscovery();
    emit panelsChanged();
}

void PanelFinder::updatePlaceholder()
{
    if (m_placeholderPanel) {
        m_placeholderPanel->deleteLater();
        m_placeholderPanel = nullptr;
    }
    QSettings settings;
    auto panelName = settings.value(u"panelName"_s).toString();
    auto rssi = settings.value(u"lastRssi"_s).toInt();
    auto ifaceVersion = settings.value(u"lastIfaceVersion"_s).toString().toLatin1();
    QBluetoothDeviceInfo info;
    if (!panelName.isEmpty()) {
        info.setName(panelName);
        info.setRssi(rssi);
        info.setManufacturerData(0x055d, ifaceVersion);
    }
    m_placeholderPanel = new PanelInfo(info, this);
}
