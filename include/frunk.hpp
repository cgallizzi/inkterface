#ifndef FRUNK_HPP
#define FRUNK_HPP

#include <algorithm>
#include <deque>
#include <limits>
#include <vector>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <QTimer>

#include "steam.hpp"

class Frunk : public QObject
{
    Q_OBJECT

  public:
    explicit Frunk(QObject *parent = nullptr);
    ~Frunk()
    {
        if (m_controller) {
            m_controller->disconnectFromDevice();
        }
        if (m_discoveryAgent) {
            m_discoveryAgent->stop();
        }
    }

    struct Point {
        double x;
        double y;

        Point()
            : x(0)
            , y(0)
        {
        }
        Point(double _x, double _y)
            : x(_x)
            , y(_y)
        {
        }
    };

    struct Points {
        std::deque<Point> points;
        size_t size;
        double xMax;
        double xMin;
        double yMax;
        double yMin;

        Points()
            : size(20)
            // WARN: our max size is 32 points to keep within a single moderate
            //       MTU (128 bytes), can be adjusted with a FW change if necessary
            , xMax(std::numeric_limits<double>::min())
            , xMin(std::numeric_limits<double>::max())
            , yMax(std::numeric_limits<double>::min())
            , yMin(std::numeric_limits<double>::max())
        {
        }

        void append(const double &x, const double &y)
        {
            points.emplace_back(x, y);
            while (points.size() > size) {
                points.pop_front();
            }
            // slow, recalc max/min, but we will only ever keep ~10 elements
            // so no big impact
            xMax = std::numeric_limits<double>::min();
            xMin = std::numeric_limits<double>::max();
            yMax = std::numeric_limits<double>::min();
            yMin = std::numeric_limits<double>::max();
            for (const auto &point : points) {
                xMax = std::max(xMax, point.x);
                xMin = std::min(xMin, point.x);
                yMax = std::max(yMax, point.y);
                yMin = std::min(yMin, point.y);
            }
        }

        void clear() { points.clear(); }
    };

    struct KeyVal {
        QString key;
        QString val;
    };
    typedef std::vector<KeyVal> KeyVals;

    struct SystemState {
        bool dirty = false;
        QString topLine;
        QString midLine;
        QString botLine;

        KeyVals keyvals{9};
        std::vector<Points> sparks{6};

        steam::App app;

        void setKeyVal(const uint8_t &index, const QString &key, const QString &val)
        {
            if (keyvals[index].key != key) {
                keyvals[index].key = key;
                dirty = true;
            }
            if (keyvals[index].val != val) {
                keyvals[index].val = val;
                dirty = true;
            }
        }

        void appendPoint(const uint8_t &index, const double &x, const double &y)
        {
            sparks[index].append(x, y);
            dirty = true;
        }

        void clearPoints(const uint8_t &index)
        {
            sparks[index].clear();
            dirty = true;
        }

        void reset()
        {
            dirty = false;
            topLine = "";
            midLine = "";
            botLine = "";
            keyvals.clear();
            keyvals.resize(9);
        }
    } state;

  public slots:
    void stop();

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerServicesDiscovered();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceError(QLowEnergyService::ServiceError error);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onReconCheck();

    void onAppStarted(steam::App details);
    void onAppStopped(steam::App details);

    void collectSystemState();
    void collectMangoData();
    void sendSystemState();

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;
    steam::Steam *m_steam = nullptr;

    QTimer *m_reconTimer = nullptr;
    QTimer *m_statsTimer = nullptr;
    QTimer *m_mangoTimer = nullptr;
    QTimer *m_sendTimer = nullptr;
    bool m_stopping = false;

    void startDiscovery();
    void writeLine(const QUuid &uuid, const QString &value);
    void writeKeyVal(const uint8_t &index, const QString &key, const QString &value);
    void writePoints(const uint8_t &index, const Points &points);
    void flushDisplay();

    QString findHwmonNode(const QString &name);
    double readHwmonNode(const QString &name, const QString &field, const double &scale = 1000.0);
    void injestMangoLog(QString path);
    void injestMangoLogs();
};

#endif /* FRUNK_HPP */
