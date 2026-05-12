#include "frunk.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QObject>
#include <QSettings>
#include <QUuid>

#define SERVICE_UUID                                                                               \
    QUuid { "95c7b479-8e84-4ce7-a121-faf74bf48c84" }
#define TOPLINE_UUID                                                                               \
    QUuid { "d6f4c07e-4a21-4c69-bd15-43a38a871900" }
#define MIDLINE_UUID                                                                               \
    QUuid { "d6f4c07e-4a21-4c69-bd15-43a38a871901" }
#define BOTLINE_UUID                                                                               \
    QUuid { "d6f4c07e-4a21-4c69-bd15-43a38a871902" }
#define KEYVAL_UUID                                                                                \
    QUuid { "d6f4c07e-4a21-4c69-bd15-43a38a871903" }
#define VECTOR_UUID                                                                                \
    QUuid { "d6f4c07e-4a21-4c69-bd15-43a38a871904" }
#define FLUSH_UUID                                                                                 \
    QUuid { "d6f4c07e-4a21-4c69-bd15-43a38a8719FF" }

#define CONN_INTERVAL 2000
#define SEND_INTERVAL 30000

using namespace Qt::Literals::StringLiterals;

Frunk::Frunk(QObject *parent)
    : QObject(parent)
    , m_ffinder(new FrunkFinder(this))
    , m_fstate(new FrunkState(this))
    , m_connTimer(new QTimer(this))
    , m_sendTimer(new QTimer(this))
{
    connect(m_ffinder, &FrunkFinder::frunksChanged, this, &Frunk::connCheck);

    m_connTimer->setSingleShot(false);
    m_connTimer->setInterval(CONN_INTERVAL);
    connect(m_connTimer, &QTimer::timeout, this, &Frunk::connCheck);
    m_connTimer->start();

    m_sendTimer->setSingleShot(false);
    m_sendTimer->setInterval(SEND_INTERVAL);
    connect(m_sendTimer, &QTimer::timeout, this, &Frunk::sendState);
    m_sendTimer->start();
}

void Frunk::stop()
{
    m_stopping = true;
    if (m_controller) {
        m_controller->disconnectFromDevice();
    }
    QTimer::singleShot(1000, this, [&]() { QCoreApplication::quit(); });
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
    qDebug() << "connected, discovering services...";
    m_connecting = false;
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
    connect(m_service, &QLowEnergyService::stateChanged, this, &Frunk::onServiceStateChanged);
    m_service->discoverDetails();
}

void Frunk::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qDebug() << "New service state: " << state;
    if (state != QLowEnergyService::RemoteServiceDiscovered) {
        return;
    }
    qDebug() << "Found " << m_service->characteristics().count() << "characteristics!";

    m_sendTimer->start(250);
}

void Frunk::writeLine(const QUuid &uuid, const QString &value)
{
    auto c = m_service->characteristic(uuid);
    if (c.isValid()) {
        m_service->writeCharacteristic(c, value.left(42).toStdString().c_str());
    }
}

void Frunk::writeKeyVal(const uint8_t &index, const QString &key, const QString &value)
{
    auto c = m_service->characteristic(KEYVAL_UUID);
    if (c.isValid()) {
        QByteArray ba;
        QDataStream s(&ba, QDataStream::WriteOnly);
        s.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));

        s << index;
        s.writeRawData(key.left(31).toStdString().c_str(), 32);
        s.writeRawData(value.left(31).toStdString().c_str(), 32);
        m_service->writeCharacteristic(c, ba);
    }
}

void Frunk::writePoints(const uint8_t &index, const FrunkField *field)
{
    auto c = m_service->characteristic(VECTOR_UUID);
    if (c.isValid()) {
        QByteArray ba;
        QDataStream s(&ba, QDataStream::WriteOnly);
        s.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));
        s.setFloatingPointPrecision(QDataStream::SinglePrecision);

        s << index;
        if (qFuzzyCompare(field->xMax() + 1.0, field->xMin() + 1.0) ||
            qFuzzyCompare(field->yMax() + 1.0, field->yMin() + 1.0)) {
            // handle potential divide by zero errors, if line is flat, draw a flat line
            s << uint8_t(4);
            s << float(field->yMin());
            s << float(field->yMax());
            s << uint8_t(0);
            s << uint8_t(255 / 2);
            s << uint8_t(255);
            s << uint8_t(255 / 2);
        } else {
            // usually we just scale the x/y axis to the full range of a uint16
            s << uint8_t(field->depth() * 2);
            s << float(field->yMin());
            s << float(field->yMax());
            double x, y;
            for (const auto &point : field->points()) {
                // we pack these into uint16_t to ease the unpack on the esp32
                x = 255.0 * ((point.x() - field->xMin()) / (field->xMax() - field->xMin()));
                y = 255.0 * ((point.y() - field->yMin()) / (field->yMax() - field->yMin()));
                s << uint8_t(x);
                s << uint8_t(y);
            }
        }

        m_service->writeCharacteristic(c, ba);
    }
}

void Frunk::flushDisplay()
{
    auto c = m_service->characteristic(FLUSH_UUID);
    if (c.isValid()) {
        m_service->writeCharacteristic(
            c, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss A").toLatin1());
    }
}

void Frunk::connCheck()
{
    if (m_connecting) {
        return;
    }
    auto frunk = m_ffinder->frunk();
    if (!frunk || !frunk->bleInfo().isValid()) {
        if (m_controller) {
            qDebug() << "Removing connection, no frunk configured/found.";
            clearConnection();
        }
        return;
    }
    // TODO: fix this, they don't always match when they DO match...
    if (frunk->bleInfo() != m_device && m_device.isValid()) {
        if (m_controller) {
            qDebug() << "Removing connection, doesn't match configured frunk.";
            clearConnection();
        }
    }
    auto state = m_controller ? m_controller->state() : QLowEnergyController::UnconnectedState;
    qDebug() << "Controller state:" << state;
    bool connected = m_controller && (state == QLowEnergyController::ConnectingState ||
                                      state == QLowEnergyController::ConnectedState ||
                                      state == QLowEnergyController::DiscoveringState ||
                                      state == QLowEnergyController::DiscoveredState);
    if (connected) {
        return;
    }
    m_connecting = true;
    m_device = frunk->bleInfo();
    qDebug() << "Connecting to " << m_device.name();
    m_controller = QLowEnergyController::createCentral(m_device, this);
    connect(m_controller, &QLowEnergyController::stateChanged, this,
            &Frunk::onControllerStateChanged);
    connect(m_controller, &QLowEnergyController::discoveryFinished, this,
            &Frunk::onControllerServicesDiscovered);
    m_controller->connectToDevice();
}

void Frunk::clearConnection()
{
    if (m_service) {
        m_service->disconnect(this);
        m_service->deleteLater();
        m_service = nullptr;
    }
    if (m_controller) {
        m_controller->disconnect(this);
        m_controller->disconnectFromDevice();
        m_controller->deleteLater();
        m_controller = nullptr;
    }
    m_device = QBluetoothDeviceInfo();
}

void Frunk::sendState()
{
    if (!m_controller || m_controller->state() == QLowEnergyController::UnconnectedState) {
        return;
    }
    if (!m_fstate->dirty()) {
        return;
    }

    writeLine(TOPLINE_UUID, m_fstate->topLine());
    writeLine(MIDLINE_UUID, m_fstate->midLine());
    writeLine(BOTLINE_UUID, m_fstate->botLine());
    int fields = 0;
    int sparks = 0;
    for (auto field : m_fstate->fields()) {
        writeKeyVal(fields, field->key(), field->val());
        ++fields;
        if (field->depth() > 0) {
            writePoints(sparks, field);
            ++sparks;
        }
    }
    flushDisplay();

    m_sendTimer->setInterval(SEND_INTERVAL);
    m_sendTimer->start();
}
