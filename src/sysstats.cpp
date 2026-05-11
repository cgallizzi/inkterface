#include "sysstats.hpp"

SysStats::SysStats(QObject *parent)
    : QObject(parent)
{
}

double SysStats::getCPUPerc()
{
    static double prevBusyTime = 0;
    static double prevTotalTime = 0;
    double currBusyTime = 0;
    double currTotalTime = 0;
    double result = 0;

    QFile f("/proc/stat");
    if (!f.exists() || !f.open(QFile::ReadOnly)) {
        return result;
    }
    QByteArray line = f.readLine();
    f.close();
    if (!line.startsWith("cpu"_ba)) {
        return result;
    }
    line = line.mid(3).trimmed();
    auto parts = line.split(' ');
    if (parts.size() < 7) {
        return result;
    }
    currBusyTime = parts[0].toDouble()                  // user busy time
                   + parts[1].toDouble()                // nice busy time
                   + parts[2].toDouble()                // system busy time
                   + parts[5].toDouble()                // irq busy time
                   + parts[6].toDouble();               // softirq busy time
    currTotalTime = currBusyTime + parts[3].toDouble(); // add system idle time
    if (prevBusyTime > 0 && prevTotalTime > 0 && currTotalTime - prevTotalTime > 0) {
        result = (currBusyTime - prevBusyTime) / (currTotalTime - prevTotalTime) * 100.0;
    }
    prevBusyTime = currBusyTime;
    prevTotalTime = currTotalTime;
    return result;
}

double SysStats::getCPUCLK()
{
    const static QByteArray tag = "cpu MHz"_ba;
    double cores = 0;
    double sum = 0;
    QFile f("/proc/cpuinfo");
    if (!f.exists() || !f.open(QFile::ReadOnly)) {
        return 0.0;
    }
    QByteArray line;
    do {
        line = f.readLine();
        if (line.startsWith(tag)) {
            ++cores;
            line = line.mid(line.indexOf(':') + 2, line.size() - line.indexOf(':'));
            sum += line.toDouble();
        }
    } while (!line.isEmpty());
    f.close();
    if (cores <= 0) {
        return 0.0;
    }
    return (sum / cores) / 1000;
}

double SysStats::getRAMPerc()
{
    const static QByteArray memTotalTag = "MemTotal:"_ba;
    const static QByteArray memAvailTag = "MemAvailable:"_ba;
    double memTotal = 0;
    double memAvail = 0;
    QFile f("/proc/meminfo");
    if (!f.exists() || !f.open(QFile::ReadOnly)) {
        return 0.0;
    }
    QByteArray line;
    do {
        line = f.readLine().trimmed();
        if (line.startsWith(memTotalTag)) {
            line = line.mid(memTotalTag.size(), line.size() - (memTotalTag.size() + 3));
            memTotal = line.toDouble();
        } else if (line.startsWith(memAvailTag)) {
            line = line.mid(memAvailTag.size(), line.size() - (memAvailTag.size() + 3));
            memAvail = line.toDouble();
        }
    } while (!line.isEmpty());
    f.close();
    if (memTotal <= 0) {
        return 0.0;
    }
    return (memTotal - memAvail) / memTotal * 100.0;
}

double SysStats::getUptime()
{
    QFile f("/proc/uptime");
    if (!f.exists() || !f.open(QFile::ReadOnly)) {
        return 0.0;
    }
    QByteArray line = f.readLine();
    f.close();
    return line.split(' ').value(0).toDouble();
}
