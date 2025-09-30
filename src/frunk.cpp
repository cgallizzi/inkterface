#include "frunk.hpp"

#include <QDebug>
#include <QObject>

using namespace Qt::Literals::StringLiterals;

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
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(15000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void Frunk::onDiscoveryEnded() {
	qDebug() << "Discovery ended!";
	for (const auto& info : m_discoveryAgent->discoveredDevices()) {
		if (!info.isValid()) {
			continue;
		}
		if (!info.name().startsWith(u"MANGO"_s)) {
			continue;
		}
		   qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
		   m_controller = QLowEnergyController::createCentral(info, this);
		   m_controller.connect(QLowEnergyContoller::stateChanged, this, Frunk::onControllerStateChanged);
		   m_controller.connect(QLowEnergyController::serviceDiscovered, this, Frunk::onControllerSvcDiscovered);
		   m_controller.connect(QLowEnergyController::errorOccurred, this, Frunk::onControllerSvcDiscovered);
		   break;
	}
	qDebug() << "Restarting discovery!";
	if (!m_controller) {
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(15000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
	}
}

void Frunk::onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "Discovery Error: " << error;
}

void Frunk::onControllerStateChanged()
{
}

void Frunk::onControllerSvcChanged()
{
}
