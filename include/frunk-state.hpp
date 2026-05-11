#ifndef FRUNK_STATE_HPP
#define FRUNK_STATE_HPP

#include <algorithm>
#include <deque>
#include <functional>
#include <limits>
#include <vector>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <QTimer>

#include "steam.hpp"
#include "sysstats.hpp"

#define FRUNK_KEYVAL_COUNT 9
#define FRUNK_SPARK_COUNT 6

using namespace Qt::Literals::StringLiterals;
using DblGetter = std::function<double()>;
using StrGetter = std::function<QString()>;
using Formatter = std::function<QString(double)>;

class FrunkCollector : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString frunkName READ frunkName CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool hasDbl READ hasStr CONSTANT)
    Q_PROPERTY(bool hasStr READ hasStr CONSTANT)

  public:
    explicit FrunkCollector(const QString &disp, const QString &frunk, const QString &desc,
                            DblGetter dg = nullptr, StrGetter sg = nullptr, Formatter fmt = nullptr,
                            QObject *parent = nullptr)
        : QObject(parent)
        , m_displayName(disp)
        , m_frunkName(frunk)
        , m_description(desc)
        , m_dblGetter(dg)
        , m_strGetter(sg)
        , m_formatter(fmt)
    {
        if (hasDbl() && m_formatter == nullptr) {
            m_formatter = [](double value) -> QString { return QString::number(value, 'f', 0); };
        }
    }

    const QString &displayName() const { return m_displayName; }
    const QString &frunkName() const { return m_frunkName; }
    const QString &description() const { return m_description; }
    bool hasStr() const { return m_strGetter != nullptr; }
    bool hasDbl() const { return m_dblGetter != nullptr; }
    double getDbl(bool *ok = nullptr)
    {
        bool status = false;
        double result = 0;
        if (m_dblGetter) {
            status = true;
            result = m_dblGetter();
        }
        if (ok != nullptr) {
            *ok = status;
        }
        return result;
    }
    QString getStr(bool *ok = nullptr)
    {
        bool status = false;
        QString result;
        if (m_strGetter) {
            status = true;
            result = m_strGetter();
        } else if (m_dblGetter && m_formatter) {
            status = true;
            result = m_formatter(m_dblGetter());
        }
        if (ok != nullptr) {
            *ok = status;
        }
        return result;
    }

  private:
    QString m_displayName;
    QString m_frunkName;
    QString m_description;
    DblGetter m_dblGetter;
    StrGetter m_strGetter;
    Formatter m_formatter;
};

class FrunkKV : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString key READ key NOTIFY dataChanged)
    Q_PROPERTY(QString val READ val NOTIFY dataChanged)

  public:
    explicit FrunkKV(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    const QString &key() const { return m_key; }
    const QString &val() const { return m_val; }

    bool setKV(const QString &k, const QString &v)
    {
        bool changed = k != m_key || v != m_val;
        m_key = k;
        m_val = v;
        if (changed)
            emit dataChanged();
        return changed;
    }

  signals:
    void dataChanged();

  private:
    QString m_key;
    QString m_val;
};

class FrunkVec : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<QPointF> points READ points NOTIFY dataChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY dataChanged)
    Q_PROPERTY(double xMin READ xMin NOTIFY dataChanged)
    Q_PROPERTY(double yMax READ yMax NOTIFY dataChanged)
    Q_PROPERTY(double yMin READ yMin NOTIFY dataChanged)

  public:
    explicit FrunkVec(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    const QList<QPointF> &points() const { return m_points; }
    const double &xMax() const { return m_xMax; }
    const double &xMin() const { return m_xMin; }
    const double &yMax() const { return m_yMax; }
    const double &yMin() const { return m_yMin; }

    void append(const double &x, const double &y)
    {
        m_points.emplace_back(x, y);
        while (m_points.size() > m_size) {
            m_points.pop_front();
        }
        // slow, recalc max/min, but we keep so few points it's not critical
        m_xMax = std::numeric_limits<double>::min();
        m_xMin = std::numeric_limits<double>::max();
        m_yMax = std::numeric_limits<double>::min();
        m_yMin = std::numeric_limits<double>::max();
        for (const auto &point : m_points) {
            m_xMax = std::max(m_xMax, point.x());
            m_xMin = std::min(m_xMin, point.x());
            m_yMax = std::max(m_yMax, point.y());
            m_yMin = std::min(m_yMin, point.y());
        }
        emit dataChanged();
    }
    void clear()
    {
        m_points.clear();
        m_xMax = std::numeric_limits<double>::min();
        m_xMin = std::numeric_limits<double>::max();
        m_yMax = std::numeric_limits<double>::min();
        m_yMin = std::numeric_limits<double>::max();
        emit dataChanged();
    }

  signals:
    void dataChanged();

  private:
    int m_size = 20;
    QList<QPointF> m_points;
    double m_xMax = std::numeric_limits<double>::min();
    double m_xMin = std::numeric_limits<double>::max();
    double m_yMax = std::numeric_limits<double>::min();
    double m_yMin = std::numeric_limits<double>::max();
};

class FrunkState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool dirty READ dirty NOTIFY dataChanged)
    Q_PROPERTY(QString topLine READ topLine NOTIFY dataChanged)
    Q_PROPERTY(QString midLine READ midLine NOTIFY dataChanged)
    Q_PROPERTY(QString botLine READ botLine NOTIFY dataChanged)
    Q_PROPERTY(QList<FrunkKV *> keyvals READ keyvals NOTIFY keyvalsChanged)
    Q_PROPERTY(QList<FrunkVec *> sparks READ sparks NOTIFY sparksChanged)
    Q_PROPERTY(QList<FrunkCollector *> collectors READ collectors NOTIFY collectorsChanged)

  public:
    explicit FrunkState(QObject *parent = nullptr)
        : QObject(parent)
        , m_steam(new steam::Steam(this))
        , m_stats(new SysStats(this))
        , m_statsTimer(new QTimer(this))
    {
        registerCollector(u"OS Version"_s, u"OS"_s, u"Host operating system version."_s,
                          std::bind(&SysStats::getOSVersion, m_stats));
        registerCollector(u"BIOS Version"_s, u"BIOS"_s, u"Host BIOS version."_s,
                          std::bind(&SysStats::getBIOSVersion, m_stats));
        registerCollector(u"Steam Version"_s, u"STEAM"_s, u"Host Steam version."_s,
                          std::bind(&steam::Steam::steamVersion, m_steam));

        registerCollector(u"App Count"_s, u"APPS"_s, u"Number of apps installed on the host."_s,
                          std::bind(&steam::Steam::installedAppCount, m_steam));

        registerCollector(u"Fan RPM"_s, u"FAN"_s, u"Host machine fan RPM."_s,
                          std::bind(&SysStats::getFanRPM, m_stats),
                          [](double v) { return u"%1 RPM"_s.arg(QString::number(v, 'f', 0)); });
        registerCollector(u"GPU Clock"_s, u"GPU SCLK"_s, u"GPU core clock speed in GHz."_s,
                          std::bind(&SysStats::getGPUSCLK, m_stats),
                          [](double v) { return u"%1 GHz"_s.arg(QString::number(v, 'f', 1)); });
        registerCollector(u"GPU Memory Clock"_s, u"GPU MCLK"_s, u"GPU memory clock speed in MHz."_s,
                          std::bind(&SysStats::getGPUSCLK, m_stats),
                          [](double v) { return u"%1 MHz"_s.arg(QString::number(v, 'f', 1)); });
        registerCollector(u"GPU Voltage"_s, u"GPU"_s, u"GPU core rail voltage."_s,
                          std::bind(&SysStats::getGPUV, m_stats),
                          [](double v) { return u"%1 V"_s.arg(QString::number(v, 'f', 3)); });
        registerCollector(u"GPU Power"_s, u"GPU"_s, u"GPU power consumption in Watts."_s,
                          std::bind(&SysStats::getGPUW, m_stats),
                          [](double v) { return u"%1 W"_s.arg(QString::number(v, 'f', 1)); });
        registerCollector(u"GPU Temperature"_s, u"GPU"_s, u"GPU junction temperature in C."_s,
                          std::bind(&SysStats::getGPUTemp, m_stats),
                          [](double v) { return u"%1 C"_s.arg(QString::number(v, 'f', 1)); });
        registerCollector(u"GPU Memory Temperature"_s, u"GPU MEM"_s,
                          u"GPU memory temperature in C."_s,
                          std::bind(&SysStats::getGPUMemTemp, m_stats),
                          [](double v) { return u"%1 C"_s.arg(QString::number(v, 'f', 1)); });
        registerCollector(u"GPU Memory"_s, u"GPU MEM"_s, u"GPU memory usage percent."_s,
                          std::bind(&SysStats::getGPUMemPerc, m_stats),
                          [](double v) { return u"%1%"_s.arg(QString::number(v, 'f', 0)); });
        registerCollector(u"GPU"_s, u"GPU"_s, u"GPU usage percent."_s,
                          std::bind(&SysStats::getGPUPerc, m_stats),
                          [](double v) { return u"%1%"_s.arg(QString::number(v, 'f', 0)); });

        registerCollector(u"SSD Temperature"_s, u"SSD"_s, u"SSD temperature in C."_s,
                          std::bind(&SysStats::getSSDTemp, m_stats),
                          [](double v) { return u"%1 C"_s.arg(QString::number(v, 'f', 1)); });

        registerCollector(u"CPU Temperature"_s, u"CPU"_s, u"CPU temperature in C."_s,
                          std::bind(&SysStats::getCPUTemp, m_stats),
                          [](double v) { return u"%1 C"_s.arg(QString::number(v, 'f', 1)); });
        registerCollector(u"CPU"_s, u"CPU"_s, u"CPU usage percent."_s,
                          std::bind(&SysStats::getCPUPerc, m_stats),
                          [](double v) { return u"%1%"_s.arg(QString::number(v, 'f', 0)); });
        registerCollector(u"CPU Frequency"_s, u"CPU"_s, u"Average CPU core frequency in GHz."_s,
                          std::bind(&SysStats::getCPUCLK, m_stats),
                          [](double v) { return u"%1 GHz"_s.arg(QString::number(v, 'f', 1)); });

        registerCollector(u"Memory"_s, u"MEM"_s, u"System memory usage percent."_s,
                          std::bind(&SysStats::getRAMPerc, m_stats),
                          [](double v) { return u"%1%"_s.arg(QString::number(v, 'f', 0)); });

        registerCollector(u"Uptime"_s, u"UPTIME"_s, u"System uptime in seconds."_s,
                          std::bind(&SysStats::getUptime, m_stats),
                          [](double v) { return u"%1 S"_s.arg(QString::number(v, 'f', 0)); });
    }

    bool dirty() const { return m_dirty; }
    const QString &topLine() const { return m_topLine; }
    const QString &midLine() const { return m_midLine; }
    const QString &botLine() const { return m_botLine; }
    const QList<FrunkKV *> keyvals() const { return m_keyvals; }
    const QList<FrunkVec *> sparks() const { return m_sparks; }
    const QList<FrunkCollector *> collectors() const { return m_collectors; }

    void registerCollector(const QString &displayName, const QString &frunkName,
                           const QString &description, DblGetter getter, Formatter fmt = nullptr)
    {
        m_collectors.append(
            new FrunkCollector(displayName, frunkName, description, getter, nullptr, fmt, this));
        emit collectorsChanged();
    }
    void registerCollector(const QString &displayName, const QString &frunkName,
                           const QString &description, StrGetter getter)
    {
        m_collectors.append(new FrunkCollector(displayName, frunkName, description, nullptr, getter,
                                               nullptr, this));
        emit collectorsChanged();
    }

    void setKV(const int &index, const QString &key, const QString &val)
    {
        if (m_keyvals.size() <= index)
            return;
        m_dirty = m_keyvals[index]->setKV(key, val);
        if (m_dirty)
            emit dataChanged();
    }
    void appendPoint(const int &index, const double &x, const double &y)
    {
        if (m_sparks.size() <= index)
            return;
        m_sparks[index]->append(x, y);
        m_dirty = true;
        emit dataChanged();
    }
    void clearPoints(const int &index)
    {
        if (m_sparks.size() <= index)
            return;
        m_sparks[index]->clear();
        m_dirty = true;
        emit dataChanged();
    }
    void reset()
    {
        m_topLine = u""_s;
        m_midLine = u""_s;
        m_botLine = u""_s;
        for (auto kv : m_keyvals) {
            kv->setKV(u""_s, u""_s);
        }
        for (auto spark : m_sparks) {
            spark->clear();
        }
        m_dirty = false;
        emit dataChanged();
    }

    const QStringList ScalarSources;
    const QStringList VectorSources;

  signals:
    void dataChanged();
    void keyvalsChanged();
    void sparksChanged();
    void collectorsChanged();

  public slots:
    void stop() {}

  private slots:
    void onAppStarted([[maybe_unused]] steam::App details) {}
    void onAppStopped([[maybe_unused]] steam::App details) {}

    void collectState() {}

  private:
    steam::Steam *m_steam = nullptr;
    SysStats *m_stats = nullptr;
    QTimer *m_statsTimer = nullptr;

    steam::App m_runningApp;

    bool m_dirty = false;
    QString m_topLine;
    QString m_midLine;
    QString m_botLine;
    QList<FrunkKV *> m_keyvals;
    QList<FrunkVec *> m_sparks;

    QList<FrunkCollector *> m_collectors;
};

#endif /* FRUNK_STATE_HPP */
