#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MAC)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace LogHandler
{
using HandlerType = void (*)(QtMsgType, const QMessageLogContext &, const QString &);

inline QFile &logFile()
{
    static QFile file;
    return file;
}

inline QMutex &logMutex()
{
    static QMutex mutex;
    return mutex;
}

inline QString &logFileName()
{
    static QString name = "app.log";
    return name;
}

inline qint64 &maxLogSizeBytes()
{
    static qint64 size = 5 * 1024 * 1024;
    return size;
}

inline HandlerType &previousHandler()
{
    static HandlerType handler = nullptr;
    return handler;
}

inline QString executablePath()
{
#if defined(Q_OS_WIN)
    wchar_t buffer[MAX_PATH];
    DWORD size = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (size == 0)
        return {};
    return QString::fromWCharArray(buffer, size);
#elif defined(Q_OS_MAC)
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
        return QString::fromUtf8(path);
    return {};
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0)
        return {};
    path[len] = '\0';
    return QString::fromUtf8(path);
#endif
}

inline QString executableDir()
{
    const QString path = executablePath();
    if (path.isEmpty())
        return QDir::currentPath();
    return QFileInfo(path).absolutePath();
}

inline QString resolveBaseDir()
{
    QByteArray appImagePath = qgetenv("APPIMAGE");
    if (!appImagePath.isEmpty()) {
        QFileInfo fi(QString::fromLocal8Bit(appImagePath));
        if (fi.exists())
            return fi.absolutePath();
    }

    const QString exeDir = executableDir();
    if (!exeDir.isEmpty())
        return exeDir;

    return QDir::currentPath();
}

inline QString logFilePath() { return resolveBaseDir() + QDir::separator() + logFileName(); }

inline QString oldLogFilePath() { return logFilePath() + ".old"; }

inline void rotateLogs()
{
    const QString basePath = logFilePath();
    const QString oldPath = oldLogFilePath();

    QFile &file = logFile();

    if (!file.isOpen()) {
        file.setFileName(basePath);
        file.open(QIODevice::Append | QIODevice::Text);
    }

    if (!file.isOpen())
        return;

    if (file.size() < maxLogSizeBytes())
        return;

    file.close();

    if (QFile::exists(oldPath))
        QFile::remove(oldPath);

    if (QFile::exists(basePath))
        QFile::rename(basePath, oldPath);

    file.setFileName(basePath);
    file.open(QIODevice::Append | QIODevice::Text);
}

inline void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    HandlerType prev = previousHandler();

    {
        QMutexLocker locker(&logMutex());

        rotateLogs();

        QFile &file = logFile();
        if (file.isOpen() || file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << qFormatLogMessage(type, context, msg) << '\n';
            out.flush();
        }
    }

    if (prev)
        prev(type, context, msg);
}

inline void
install(const QString &fileName = "app.log", qint64 maxSizeMB = 5,
        const QString &pattern = "[%{time yyyy-MM-dd HH:mm:ss.zzz}] [%{type}] %{message}")
{
    logFileName() = fileName;
    maxLogSizeBytes() = maxSizeMB * 1024 * 1024;

    previousHandler() = qInstallMessageHandler(nullptr);

    qSetMessagePattern(pattern);
    qInstallMessageHandler(messageHandler);
}
} // namespace LogHandler

#endif
