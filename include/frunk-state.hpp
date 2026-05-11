#ifndef FRUNK_STATE_HPP
#define FRUNK_STATE_HPP

#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>

#include <QObject>
#include <QSettings>
#include <QTimer>

#include "steam.hpp"
#include "sysstats.hpp"

#define FRUNK_POINT_DEPTH 20
#define FRUNK_READOUT_COUNT 3
#define FRUNK_SPARK_COUNT 6
#define FRUNK_FIELD_COUNT (FRUNK_READOUT_COUNT + FRUNK_SPARK_COUNT)

using namespace Qt::Literals::StringLiterals;
using DblGetter = std::function<double()>;
using StrGetter = std::function<QString()>;
using Formatter = std::function<QString(double)>;

inline int64_t NOW_MS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

class FrunkCollector : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString frunkName READ frunkName CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool hasDbl READ hasDbl CONSTANT)
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
    bool hasDbl() const { return m_dblGetter != nullptr; }
    Q_INVOKABLE double getDbl(bool *ok = nullptr)
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
    bool hasStr() const { return m_strGetter != nullptr || m_formatter != nullptr; }
    Q_INVOKABLE QString getStr(bool *ok = nullptr)
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

class FrunkField : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString key READ key NOTIFY readoutChanged)
    Q_PROPERTY(QString val READ val NOTIFY readoutChanged)

    Q_PROPERTY(int depth READ depth NOTIFY pointsChanged)
    Q_PROPERTY(QList<QPointF> points READ points NOTIFY pointsChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY pointsChanged)
    Q_PROPERTY(double xMin READ xMin NOTIFY pointsChanged)
    Q_PROPERTY(double yMax READ yMax NOTIFY pointsChanged)
    Q_PROPERTY(double yMin READ yMin NOTIFY pointsChanged)

    Q_PROPERTY(FrunkCollector *collector READ collector WRITE setCollector NOTIFY collectorChanged)

  public:
    explicit FrunkField(const int &index, const double &depth = 0, QObject *parent = nullptr)
        : QObject(parent)
        , m_index(index)
        , m_depth(depth)
    {
    }

    const QString &key() const { return m_key; }
    const QString &val() const { return m_val; }
    const int &depth() const { return m_depth; }
    const QList<QPointF> &points() const { return m_points; }
    const double &xMax() const { return m_xMax; }
    const double &xMin() const { return m_xMin; }
    const double &yMax() const { return m_yMax; }
    const double &yMin() const { return m_yMin; }
    QPointer<FrunkCollector> collector() const { return m_collector; }

    void setCollector(QPointer<FrunkCollector> collector)
    {
        if (m_collector != collector) {
            m_collector = collector;
            QSettings settings;
            settings.setValue(u"field%1Collector"_s.arg(QString::number(m_index)),
                              m_collector ? m_collector->displayName() : u""_s);
            emit collectorChanged();
            clear();
            if (m_collector) {
                m_key = m_collector->frunkName();
                m_val = m_collector->getStr();
            }
            emit readoutChanged();
            emit pointsChanged();
        }
    }

    bool collect(const double &x = 0)
    {
        if (!m_collector)
            return false;
        QString k = m_collector->frunkName();
        QString v = m_collector->getStr();
        bool changed = k != m_key || v != m_val;
        m_key = k;
        m_val = v;
        if (changed) {
            emit readoutChanged();
            if (m_depth && m_collector->hasDbl()) {
                append(x, m_collector->getDbl());
                emit pointsChanged();
            }
        }
        return changed;
    }

  signals:
    void readoutChanged();
    void pointsChanged();
    void collectorChanged();

  private:
    int m_index = 0;
    QString m_key;
    QString m_val;
    int m_depth = 20;
    QList<QPointF> m_points;
    double m_xMax = std::numeric_limits<double>::min();
    double m_xMin = std::numeric_limits<double>::max();
    double m_yMax = std::numeric_limits<double>::min();
    double m_yMin = std::numeric_limits<double>::max();
    QPointer<FrunkCollector> m_collector;

    void append(const double &x, const double &y)
    {
        m_points.emplace_back(x, y);
        while (m_points.size() > m_depth) {
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
    }

    void clear()
    {
        m_key = u""_s;
        m_val = u""_s;
        m_points.clear();
        m_xMax = std::numeric_limits<double>::min();
        m_xMin = std::numeric_limits<double>::max();
        m_yMax = std::numeric_limits<double>::min();
        m_yMin = std::numeric_limits<double>::max();
    }
};

class FrunkState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool dirty READ dirty NOTIFY dataChanged)
    Q_PROPERTY(QString topLine READ topLine NOTIFY dataChanged)
    Q_PROPERTY(QString midLine READ midLine NOTIFY dataChanged)
    Q_PROPERTY(QString botLine READ botLine NOTIFY dataChanged)
    Q_PROPERTY(QList<FrunkField *> fields READ fields NOTIFY fieldsChanged)
    Q_PROPERTY(QList<FrunkCollector *> collectors READ collectors NOTIFY collectorsChanged)

  public:
    explicit FrunkState(QObject *parent = nullptr)
        : QObject(parent)
        , m_steam(new steam::Steam(this))
        , m_stats(new SysStats(this))
        , m_updateTimer(new QTimer(this))
    {
        // str only collectors
        registerCollector(u"OS Version"_s, u"OS"_s, u"Host operating system version."_s,
                          std::bind(&SysStats::getOSVersion, m_stats));
        registerCollector(u"BIOS Version"_s, u"BIOS"_s, u"Host BIOS version."_s,
                          std::bind(&SysStats::getBIOSVersion, m_stats));
        registerCollector(u"Steam Version"_s, u"STEAM"_s, u"Host Steam version."_s,
                          std::bind(&steam::Steam::steamVersion, m_steam));
        // double collectors that can be spark graphed
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

        // TODO: allow configuring different numbers/layouts of fields
        // TODO: allow re-linking collectors to fields if collector set changes
        //       (to handle collectors added later in runtime)
        QSettings settings;
        QString collectorName;
        FrunkField *field = nullptr;
        for (int i = 0; i < FRUNK_FIELD_COUNT; ++i) {
            if (i < FRUNK_READOUT_COUNT) {
                field = new FrunkField(i, 0, this);
            } else {
                field = new FrunkField(i, FRUNK_POINT_DEPTH, this);
            }
            // TODO: this is lame, but i want the value in the settings file to
            //       be human readable, probably we should just enforce unique
            //       display names when registering collectors and leave it at that.
            collectorName = settings.value(u"field%1Collector"_s.arg(i)).toString();
            if (collectorName.isEmpty()) {
                switch (i) {
                case 0:
                    collectorName = u"OS Version"_s;
                    break;
                case 1:
                    collectorName = u"BIOS Version"_s;
                    break;
                case 2:
                    collectorName = u"Steam Version"_s;
                    break;
                case 3:
                    collectorName = u"CPU Temperature"_s;
                    break;
                case 4:
                    collectorName = u"GPU Temperature"_s;
                    break;
                case 5:
                    collectorName = u"Fan RPM"_s;
                    break;
                case 6:
                    collectorName = u"CPU"_s;
                    break;
                case 7:
                    collectorName = u"GPU"_s;
                    break;
                case 8:
                    collectorName = u"Memory"_s;
                    break;
                }
            }
            if (!collectorName.isEmpty()) {
                for (auto collector : m_collectors) {
                    if (collector->displayName() == collectorName) {
                        field->setCollector(collector);
                        break;
                    }
                }
            }
            m_fields.append(field);
        }

        connect(m_steam, &steam::Steam::appStarted, this, &FrunkState::onAppStarted);
        connect(m_steam, &steam::Steam::appStopped, this, &FrunkState::onAppStopped);
        m_steam->watchConsoleLog(true);

        connect(m_updateTimer, &QTimer::timeout, this, &FrunkState::updateState);
        m_updateTimer->setInterval(2000);
        m_updateTimer->setSingleShot(false);
        updateState();
        m_updateTimer->start();
    }

    bool dirty() const { return m_dirty; }
    const QString &topLine() const { return m_topLine; }
    const QString &midLine() const { return m_midLine; }
    const QString &botLine() const { return m_botLine; }
    const QList<FrunkField *> fields() const { return m_fields; }
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

  signals:
    void dataChanged();
    void fieldsChanged();
    void collectorsChanged();

  public slots:
    void start() { m_updateTimer->start(); }
    void stop() { m_updateTimer->stop(); }
    void updateState()
    {
        bool changed = false;

        // top line is hostname from machine
        // TODO: allow configuring this
        changed |= m_topLine != m_stats->getHostName();
        m_topLine = m_stats->getHostName();

        // mid line is the most recent steam user
        // TODO: allow configuring this
        changed |= m_midLine != m_steam->currentUser();
        m_midLine = m_steam->currentUser();

        // bottom line is currently running app info
        // TODO: allow configuring this
        // TODO: add full utf-8 support to the e-ink font
        QString botLine;
        auto app = m_steam->runningApp();
        for (QChar c : app.name) {
            if (c.unicode() <= 127) {
                botLine.append(c);
            }
        }
        changed |= m_botLine != botLine;
        m_botLine = botLine;

        // collect new data into fields (for all fields that have collectors)
        double x = NOW_MS();
        for (auto field : m_fields) {
            changed |= field->collect(x);
        }

        if (changed) {
            m_dirty = true;
            emit dataChanged();
        }
    }

  private slots:
    void onAppStarted([[maybe_unused]] steam::App details) { updateState(); }
    void onAppStopped([[maybe_unused]] steam::App details) { updateState(); }

  private:
    steam::Steam *m_steam = nullptr;
    SysStats *m_stats = nullptr;
    QTimer *m_updateTimer = nullptr;

    bool m_dirty = false;
    QString m_topLine;
    QString m_midLine;
    QString m_botLine;
    QList<FrunkField *> m_fields;
    QList<FrunkCollector *> m_collectors;
};

#endif /* FRUNK_STATE_HPP */
