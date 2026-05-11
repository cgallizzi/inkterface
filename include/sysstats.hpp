#ifndef SYSSTATS_HPP
#define SYSSTATS_HPP

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSysInfo>

using namespace Qt::Literals::StringLiterals;

class SysStats : public QObject
{
    Q_OBJECT

  public:
    explicit SysStats(QObject *parent = nullptr);

    QString getHostName() { return QSysInfo::machineHostName(); }
    QString getOSVersion() { return QSysInfo::productVersion(); }
    QString getBIOSVersion()
    {
        QString val;
        QFile f("/sys/class/dmi/id/bios_version");
        if (f.exists() && f.open(QFile::ReadOnly)) {
            val = QString::fromUtf8(f.readAll()).trimmed();
            f.close();
        }
        return val;
    }
    // in RPM
    double getFanRPM() { return readHwmonNode("steamdeck_hwmon", "fan1_input"); }
    // in GHz
    double getGPUSCLK() { return readHwmonNode("amdgpu", "freq1_input", 0.000000001); }
    // in MHz
    double getGPUMCLK() { return readHwmonNode("amdgpu", "freq2_input", 0.000001); }
    // in volts
    double getGPUV() { return readHwmonNode("amdgpu", "in0_input", 0.001); }
    // in watts
    double getGPUW() { return readHwmonNode("amdgpu", "power1_average", 0.000001); }
    // in degrees C
    double getGPUTemp() { return readHwmonNode("amdgpu", "temp2_input", 0.001); }
    // in degrees C
    double getGPUMemTemp() { return readHwmonNode("amdgpu", "temp3_input", 0.001); }
    // in percentage (0-100)
    double getGPUPerc() { return readHwmonDeviceNode("amdgpu", "gpu_busy_percent"); }
    // in percentage (0-100)
    double getGPUMemPerc() { return readHwmonDeviceNode("amdgpu", "mem_busy_percent"); }
    // in degrees C
    double getSSDTemp() { return readHwmonNode("nvme", "temp1_input", 0.001); }
    // in degrees C
    double getCPUTemp() { return readHwmonNode("k10temp", "temp1_input", 0.001); }
    // in percentage (0-100)
    double getCPUPerc();
    // average of all cores in GHz
    double getCPUCLK();
    // in percentage (0-100)
    double getRAMPerc();
    // in seconds
    double getUptime();

    // f.setFileName("/etc/os-release");
    // if (f.exists() && f.open(QFile::ReadOnly)) {
    //     ba = f.readAll();
    //     f.close();
    //     val = QString::fromUtf8(ba).trimmed();
    //     auto idx = val.indexOf("BUILD_ID=") + 9;
    //     val = val.mid(idx, val.indexOf("\n", idx));
    // }
    // state.setKeyVal(0, "OS", val);

  private:
    double readAsDouble(const QString &filepath)
    {
        QString data;
        QFile f(filepath);
        if (f.exists() && f.open(QFile::ReadOnly)) {
            data = QString::fromUtf8(f.readAll()).trimmed();
            f.close();
        }
        return data.toDouble();
    }
    QString findHwmonNode(const QString &name)
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
    double readHwmonNode(const QString &name, const QString &field, const double &scale = 1.0)
    {
        auto path = findHwmonNode(name);
        if (path.isEmpty()) {
            return 0.0;
        }
        path += "/" + field;
        return readAsDouble(path) * scale;
    }
    double readHwmonDeviceNode(const QString &name, const QString &field, const double &scale = 1.0)
    {
        auto path = findHwmonNode(name);
        if (path.isEmpty()) {
            return 0.0;
        }
        path += "/device/" + field;
        return readAsDouble(path) * scale;
    }
};

#endif /* SYSSTATS_HPP */
