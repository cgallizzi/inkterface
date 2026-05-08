#include "frunk.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iterator>

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QSettings>
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
#define STATS_INTERVAL 2000
#define MANGO_INTERVAL 15000
#define SEND_INTERVAL 30000

using namespace Qt::Literals::StringLiterals;

inline int64_t NOW_MS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

Frunk::Frunk(const QString &name, QObject *parent)
    : QObject(parent)
    , ScalarSources({
          u"OS"_s,
          u"BIOS"_s,
          u"STEAM"_s,
          // # of installed games
          // most recent achievment maybe
      })
    , VectorSources({
          u"FAN RPM"_s,   // SysStats::getFanRPM()
          u"GPU SCLK"_s,  // SysStats::getGPUSCLK()
          u"GPU MCLK"_s,  // SysStats::getGPUMCLK()
          u"GPU V"_s,     // SysStats::getGPUV()
          u"GPU W"_s,     // SysStats::getGPUW()
          u"GPU TEMP"_s,  // SysStats::getGPUTemp()
          u"GPU MEM"_s,   // SysStats::getGPUMemTemp()
          u"GPU %"_s,     // SysStats::getGPUPerc()
          u"GPU MEM %"_s, // SysStats::getGPUMemPerc()
          u"SSD TEMP"_s,  // SysStats::getSSDTemp()
          u"CPU TEMP"_s,  // SysStats::getCPUTemp()
          u"CPU %"_s,     // SysStats::getCPUPerc()
          u"MEM %"_s,     // SysStats::getRAMPerc()
          u"UPTIME"_s,    // SysStats::getUptime()
      })
    /* other ideas for things to display:
     *  active download progress as discrete bar
     *  quote of the day
     *  gabe/gnomekyle faces
     *  steampal-chan face
     *  clock (analog and digital, updates every minute)
     *  portal sentry eye
     *  companion cube
     *  literal easter egg
     *  weather display (need to pickup location or allow setting location)
     *  top style readout (likely top n-processes aggregated over like 30+ seconds)
     *  show connected controllers, just steam controller initially, other HID devices
     *      could be identified later or added by community
     */
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
    , m_steam(new steam::Steam(this))
    , m_desiredName(name)
    , m_stats(new SysStats(this))
    , m_reconTimer(new QTimer(this))
    , m_statsTimer(new QTimer(this))
    , m_mangoTimer(new QTimer(this))
    , m_sendTimer(new QTimer(this))
{
    collectSystemState();

    QSettings settings;
    auto frunkName = settings.value(u"frunkName"_s).toString();
    if (m_desiredName.isEmpty() && !frunkName.isEmpty()) {
        qInfo() << "using" << frunkName << "from settings as desired name";
        m_desiredName = frunkName;
    }

    m_reconTimer->setSingleShot(false);
    m_reconTimer->setInterval(RECON_INTERVAL);
    connect(m_reconTimer, &QTimer::timeout, this, &Frunk::onReconCheck);

    m_statsTimer->setSingleShot(false);
    m_statsTimer->setInterval(STATS_INTERVAL);
    connect(m_statsTimer, &QTimer::timeout, this, &Frunk::collectSystemState);
    m_statsTimer->start();

    m_mangoTimer->setSingleShot(false);
    m_mangoTimer->setInterval(MANGO_INTERVAL);
    connect(m_mangoTimer, &QTimer::timeout, this, &Frunk::collectMangoData);
    // m_mangoTimer->start();

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
        if (!info.isValid() || info.isCached() || !info.name().startsWith(u"FRUNK-"_s)) {
            continue;
        }
        qDebug() << "Discovered: " << info.name() << ", RSSI: " << info.rssi();
        if (!m_desiredName.isEmpty()) {
            if (m_desiredName == info.name()) {
                nearest = info;
                break;
            } else {
                continue;
            }
        }
        if (!nearest.isValid()) {
            nearest = info;
        } else if (info.rssi() < nearest.rssi()) {
            nearest = info;
        }
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
        s.setFloatingPointPrecision(QDataStream::SinglePrecision);

        s << index;
        if (qFuzzyCompare(points.xMax + 1.0, points.xMin + 1.0) ||
            qFuzzyCompare(points.yMax + 1.0, points.yMin + 1.0)) {
            // handle potential divide by zero errors, if line is flat, draw a flat line
            s << uint8_t(4);
            s << float(points.yMin);
            s << float(points.yMax);
            s << uint8_t(0);
            s << uint8_t(255 / 2);
            s << uint8_t(255);
            s << uint8_t(255 / 2);
        } else {
            // usually we just scale the x/y axis to the full range of a uint16
            s << uint8_t(points.points.size() * 2);
            s << float(points.yMin);
            s << float(points.yMax);
            double x, y;
            for (const auto &point : points.points) {
                // we pack these into uint16_t to ease the unpack on the esp32
                x = 255.0 * ((point.x - points.xMin) / (points.xMax - points.xMin));
                y = 255.0 * ((point.y - points.yMin) / (points.yMax - points.yMin));
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
    state.dirty = true;
    m_statsTimer->start(100);
    // m_mangoTimer->start(100);
    m_sendTimer->start(500);
}

void Frunk::onAppStopped(steam::App details)
{
    qDebug() << "App stopped:" << details.appid << "," << details.name;
    state.app.appid.clear();
    state.app.name.clear();
    state.dirty = true;
    mango::stop_logging(); // no harm in making sure we've stopped the logging session
    m_statsTimer->start(100);
    m_sendTimer->start(500);
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
    QFile f;
    QByteArray ba;
    QDir d;

    state.topLine = m_stats->getHostName();
    state.midLine = m_steam->currentUser();
    ;
    if (state.app.name.isEmpty() && !state.botLine.isEmpty()) {
        state.botLine = "";
    } else if (!state.app.name.isEmpty()) {
        state.botLine = state.app.name;
    }

    state.setKeyVal(0, "OS", m_stats->getOSVersion());
    state.setKeyVal(1, "BIOS", m_stats->getBIOSVersion());
    state.setKeyVal(2, "STEAM", m_steam->steamVersion());

    double x = NOW_MS();
    double y = 0;

    y = m_stats->getCPUTemp();
    state.setKeyVal(3, "CPU", u"%1 C"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(0, x, y);

    y = m_stats->getGPUTemp();
    state.setKeyVal(4, "GPU", u"%1 C"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(1, x, y);

    y = m_stats->getFanRPM();
    state.setKeyVal(5, "FAN", u"%1 RPM"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(2, x, y);

    y = m_stats->getCPUPerc();
    state.setKeyVal(6, "CPU", u"%1%"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(3, x, y);

    y = m_stats->getGPUPerc();
    state.setKeyVal(7, "GPU", u"%1%"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(4, x, y);

    y = m_stats->getRAMPerc();
    state.setKeyVal(8, "MEM", u"%1%"_s.arg(QString::number(y, 'f', 0)));
    state.appendPoint(5, x, y);

    m_statsTimer->setInterval(STATS_INTERVAL);
    m_statsTimer->start();
}

void Frunk::collectMangoData()
{
    if (state.app.appid.isEmpty()) {
        return;
    }
    mango::set_display(false);
    mango::stop_logging();
    injestMangoLogs();
    mango::set_display(true);
    mango::start_logging(state.app.appid.toLatin1());

    m_mangoTimer->setInterval(MANGO_INTERVAL);
    // m_mangoTimer->start();
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
