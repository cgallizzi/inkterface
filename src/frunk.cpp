#include "frunk.hpp"

#include <algorithm>
#include <chrono>
#include <iterator>

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QSysInfo>
#include <QUuid>

#include "mango.hpp"

#define SERVICE_UUID QUuid{"95c7b479-8e84-4ce7-a121-faf74bf48c84"}
#define TOPLINE_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871900"}
#define MIDLINE_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871901"}
#define BOTLINE_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871902"}
#define KEYVAL_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871903"}
#define VECTOR_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a871904"}
#define FLUSH_UUID QUuid{"d6f4c07e-4a21-4c69-bd15-43a38a8719FF"}

#define RSSI_LIMIT -80
#define RECON_INTERVAL 2000
#define COLLECT_INTERVAL 5000
#define SEND_INTERVAL 30000

using namespace Qt::Literals::StringLiterals;

inline int64_t NOW_MS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

Frunk::Frunk(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
    , m_steam(new steam::Steam(this))
    , m_reconTimer(new QTimer(this))
    , m_updateTimer(new QTimer(this))
    , m_sendTimer(new QTimer(this))
{
    collectSystemState();

    m_reconTimer->setSingleShot(false);
    m_reconTimer->setInterval(RECON_INTERVAL);
    connect(m_reconTimer, &QTimer::timeout, this, &Frunk::onReconCheck);

    m_updateTimer->setSingleShot(false);
    m_updateTimer->setInterval(COLLECT_INTERVAL);
    connect(m_updateTimer, &QTimer::timeout, this, &Frunk::collectSystemState);
    m_updateTimer->start();

    m_sendTimer->setSingleShot(false);
    m_sendTimer->setInterval(SEND_INTERVAL);
    connect(m_sendTimer, &QTimer::timeout, this, &Frunk::sendSystemState);
    m_sendTimer->start();

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this,
            &Frunk::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &Frunk::onDiscoveryEnded);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
            &Frunk::onDiscoveryError);
    qDebug() << "Starting discovery!";
    startDiscovery();

    connect(m_steam, &steam::Steam::appStarted, this, &Frunk::onAppStarted);
    connect(m_steam, &steam::Steam::appStopped, this, &Frunk::onAppStopped);
    m_steam->watchConsoleLog(true);
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
    mango::stop_logging(); // no harm in making sure we've stopped the logging session
    QTimer::singleShot(1000, this, [&]() { QCoreApplication::quit(); });
}

void Frunk::onDiscoveryEnded()
{
    bool noController =
        !m_controller || m_controller->state() == QLowEnergyController::UnconnectedState;
    QBluetoothDeviceInfo nearest;
    for (const auto &info : m_discoveryAgent->discoveredDevices()) {
        if (!info.isValid() || info.isCached() || !info.name().startsWith(u"MANGO"_s)) {
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

void Frunk::writePoints(const uint8_t &index, const Points &points)
{
    auto c = m_service->characteristic(VECTOR_UUID);
    if (c.isValid()) {
        QByteArray ba;
        QDataStream s(&ba, QDataStream::WriteOnly);
        s.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));

        s << index;
        if (qFuzzyCompare(points.xMax + 1.0, points.xMin + 1.0) ||
            qFuzzyCompare(points.yMax + 1.0, points.yMin + 1.0)) {
            // handle potential divide by zero errors, if line is flat, draw a flat line
            s << uint8_t(4);
            s << uint16_t(0);
            s << uint16_t(65535 / 2);
            s << uint16_t(65535);
            s << uint16_t(65535 / 2);
        } else {
            // usually we just scale the x/y axis to the full range of a uint16
            s << uint8_t(points.points.size() * 2);
            double x, y;
            for (const auto &point : points.points) {
                // we pack these into uint16_t to ease the unpack on the esp32
                x = 65535.0 * ((point.x - points.xMin) / (points.xMax - points.xMin));
                y = 65535.0 * ((point.y - points.yMin) / (points.yMax - points.yMin));
                s << uint16_t(x);
                s << uint16_t(y);
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

void Frunk::onAppStarted(steam::App details)
{
    qDebug() << "App started:" << details.appid << "," << details.name;
    state.app.appid = details.appid;
    // TODO: add full utf-8 support to the e-ink font
    state.app.name.clear();
    for (QChar c : details.name) {
        if (c.unicode() <= 127) {
            state.app.name.append(c);
        }
    }
    state.clearPoints(3);
    state.clearPoints(4);
    state.clearPoints(5);
    state.dirty = true;
    collectSystemState();
    m_sendTimer->start(250);
}

void Frunk::onAppStopped(steam::App details)
{
    qDebug() << "App stopped:" << details.appid << "," << details.name;
    state.app.appid.clear();
    state.app.name.clear();
    state.setKeyVal(6, "", "");
    state.setKeyVal(7, "", "");
    state.setKeyVal(8, "", "");
    state.clearPoints(3);
    state.clearPoints(4);
    state.clearPoints(5);
    state.dirty = true;
    mango::stop_logging(); // no harm in making sure we've stopped the logging session
    collectSystemState();
    m_sendTimer->start(250);
}

void Frunk::startDiscovery()
{
    if (m_stopping)
        return;
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(10000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void Frunk::injestMangoLogs()
{
    auto d = QDir::home();
    auto entries = d.entryList({{"mangoapp_*.csv"}});
    std::sort(entries.begin(), entries.end());
    qDebug() << "injesting mango logs:";
    for (auto entry : entries) {
        // ignore the summary logs, we are generating time series data
        if (!entry.contains("_summary.csv")) {
            qDebug() << "> " << entry;
            injestMangoLog(d.absoluteFilePath(entry));
        }
        d.remove(entry);
    }
}

void Frunk::injestMangoLog(QString path)
{
    QFile f{path};
    if (!f.open(QFile::ReadOnly)) {
        qWarning() << "failed to read mango log:" << path;
        return;
    }
    state.clearPoints(3);
    state.clearPoints(4);
    state.clearPoints(5);
    state.setKeyVal(6, "CPU", u"--%"_s);
    state.setKeyVal(6, "GPU", u"--%"_s);
    state.setKeyVal(6, "FPS", u"--"_s);
    bool in_data = false;
    int64_t last_elapsed = 0, current_elapsed = 0;
    double fps = 0, cpu_load = 0, gpu_load = 0;
    int sample_count = 0;
    QByteArray line;
    QList<QByteArray> fields;
    while (!f.atEnd()) {
        line = f.readLine().trimmed();
        // skip lines until we see the desired header
        if (!in_data && line.startsWith("fps,frametime,cpu_load,")) {
            in_data = true;
            continue;
        } else if (!in_data) {
            continue;
        }
        // this is a very simple csv, all numeric fields, no risk of stray commas
        fields = line.split(',');
        current_elapsed = fields.last().toLongLong() / 1000 / 1000; // to ms
        fps += fields.at(0).toDouble();
        cpu_load += fields.at(2).toDouble();
        gpu_load += fields.at(4).toDouble();
        ++sample_count;
        // ignore points that are too close together, we have limited resolution
        // on the e-ink display so it's pointless to have really high freq data
        if ((current_elapsed - last_elapsed) < 500) {
            continue;
        }
        fps /= sample_count;
        cpu_load /= sample_count;
        gpu_load /= sample_count;
        state.setKeyVal(6, "CPU", u"%1%"_s.arg(QString::number(cpu_load, 'f', 1)));
        state.appendPoint(3, current_elapsed, cpu_load);
        state.setKeyVal(7, "GPU", u"%1%"_s.arg(QString::number(gpu_load, 'f', 1)));
        state.appendPoint(4, current_elapsed, gpu_load);
        state.setKeyVal(8, "FPS", u"%1"_s.arg(QString::number(fps, 'f', 1)));
        state.appendPoint(5, current_elapsed, fps);
        last_elapsed = current_elapsed;
        fps = 0;
        cpu_load = 0;
        gpu_load = 0;
        sample_count = 0;
    }
    f.close();
}

void Frunk::collectSystemState()
{
    QString val;
    QFile f;
    QByteArray ba;
    QDir d;

    auto user = m_steam->currentUser();
    state.topLine = QSysInfo::machineHostName();
    state.midLine = user.isEmpty() ? u"No user signed in."_s : u"%1 is signed in."_s.arg(user);
    if (!state.app.appid.isEmpty()) {
        state.botLine = u"Playing %1"_s.arg(state.app.name);
        // we might want to stop the display to avoid the huge "logging ended"
        // banner, and we stop logging because mangohud will fill up memory with
        // some buffers used for generating the summary output if left alive forever
        qDebug() << "stopping ui and mango log session";
        mango::set_display(false);
        mango::stop_logging();
        injestMangoLogs();
        qDebug() << "restarting ui and mango log session";
        mango::set_display(true);
        mango::start_logging(state.app.appid.toLatin1());
    } else {
        state.botLine = "";
    }

    val = QSysInfo::productVersion();
    state.setKeyVal(0, "OS", val);

    val = "N/A";
    f.setFileName("/sys/class/dmi/id/bios_version");
    if (f.exists() && f.open(QFile::ReadOnly)) {
        ba = f.readAll();
        f.close();
        val = QString::fromUtf8(ba).trimmed();
    }
    state.setKeyVal(1, "BIOS", val);

    val = "N/A";
    d.setPath("/home/deck/.steam/steam/package");
    for (auto entry : d.entryList({{"*.manifest"}})) {
        f.setFileName(d.absoluteFilePath(entry));
        if (f.exists() && f.open(QFile::ReadOnly)) {
            while (true) {
                ba = f.readLine();
                if (ba.contains("version")) {
                    break;
                }
            }
            f.close();
            ba = ba.trimmed().last(14);
            ba.replace('"', ' ');
            val = QString::fromUtf8(ba).trimmed();
        }
        break;
    }
    state.setKeyVal(2, "STEAM", val);

    double x = NOW_MS();
    double y = 0;

    // cpu Tctl, seems nice and accurate
    y = readHwmonNode("k10temp", "temp1_input");
    state.setKeyVal(3, "CPU", u"%1 C"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(0, x, y);

    // gpu junction
    y = readHwmonNode("amdgpu", "temp2_input");
    state.setKeyVal(4, "GPU", u"%1 C"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(1, x, y);

    y = readHwmonNode("steamdeck_hwmon", "fan1_input", 1.0);
    state.setKeyVal(5, "FAN", u"%1 RPM"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(2, x, y);
}

QString Frunk::findHwmonNode(const QString &name)
{
    static QMap<QString, QString> nodes;
    if (nodes.contains(name)) {
        return nodes[name];
    }

    QFile f;
    QString data;
    QDir d{"/sys/class/hwmon"};
    for (auto entry : d.entryList({{"hwmon*"}})) {
        f.setFileName(d.absoluteFilePath(entry) + "/name");
        if (f.exists() && f.open(QFile::ReadOnly)) {
            data = QString::fromUtf8(f.readAll()).trimmed();
            f.close();
            nodes[data] = d.absoluteFilePath(entry);
        }
    }

    return nodes.value(name);
}

double Frunk::readHwmonNode(const QString &name, const QString &field, const double &scale)
{
    auto path = findHwmonNode(name);
    if (path.isEmpty()) {
        return 0.0;
    }
    path += "/" + field;

    QString data;
    QFile f{path};
    if (f.exists() && f.open(QFile::ReadOnly)) {
        data = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
    } else {
        return 0.0;
    }

    return data.toDouble() / scale;
}

void Frunk::sendSystemState()
{
    if (!m_controller || m_controller->state() == QLowEnergyController::UnconnectedState) {
        return;
    }
    if (!state.dirty) {
        return;
    }

    writeLine(TOPLINE_UUID, state.topLine);
    writeLine(MIDLINE_UUID, state.midLine);
    writeLine(BOTLINE_UUID, state.botLine);
    for (auto item = state.keyvals.begin(); item != state.keyvals.end(); ++item) {
        auto index = std::distance(state.keyvals.begin(), item);
        if (item->key.isEmpty()) {
            writeKeyVal(index, "", "");
        } else {
            writeKeyVal(index, item->key, item->val);
        }
    }
    for (auto item = state.sparks.begin(); item != state.sparks.end(); ++item) {
        auto index = std::distance(state.sparks.begin(), item);
        writePoints(index, *item);
    }

    flushDisplay();
    m_sendTimer->setInterval(SEND_INTERVAL);
    m_sendTimer->start();
}
