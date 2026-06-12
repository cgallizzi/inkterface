#ifndef FRUNK_FINDER_HPP
#define FRUNK_FINDER_HPP

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QtQmlIntegration/qqmlintegration.h>

using namespace Qt::StringLiterals;

class FrunkInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Backend only.")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(qint16 rssi READ rssi CONSTANT)
    Q_PROPERTY(QString ifaceVersion READ ifaceVersion CONSTANT)
    Q_PROPERTY(bool supported READ supported CONSTANT)

  public:
    explicit FrunkInfo(const QBluetoothDeviceInfo &info, QObject *parent = nullptr)
        : QObject(parent)
        , m_info(info)
    {
    }

    QString name() const { return m_info.name(); }
    qint16 rssi() const { return m_info.rssi(); }
    QString ifaceVersion() const { return QString::fromLatin1(m_info.manufacturerData(0x055d)); }
    bool supported() const { return ifaceVersion() == u"IFv01"_s; }
    const QBluetoothDeviceInfo &bleInfo() const { return m_info; }

  private:
    QBluetoothDeviceInfo m_info;
};

class FrunkFinder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<FrunkInfo *> frunks MEMBER m_frunks NOTIFY frunksChanged)
    Q_PROPERTY(bool frunkFound READ frunkFound NOTIFY frunksChanged)
    Q_PROPERTY(FrunkInfo *frunk READ frunk NOTIFY frunksChanged)

  signals:
    void frunksChanged();

  public:
    explicit FrunkFinder(QObject *parent = nullptr);
    ~FrunkFinder()
    {
        if (m_discoveryAgent) {
            m_discoveryAgent->stop();
        }
    }

    bool frunkFound() const { return m_frunkFound; }
    QPointer<FrunkInfo> frunk()
    {
        updatePlaceholder();
        auto frunkName = m_placeholderFrunk->name();
        for (auto frunk : std::as_const(m_frunks)) {
            if (frunk->name() == frunkName) {
                return frunk;
            }
        }
        return m_placeholderFrunk;
    }

  public slots:
    void startDiscovery()
    {
        m_stopping = false;
        m_discoveryAgent->setLowEnergyDiscoveryTimeout(10000);
        m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }
    void stopDiscovery()
    {
        m_stopping = true;
        m_discoveryAgent->stop();
        for (auto frunk : std::as_const(m_frunks)) {
            frunk->deleteLater();
        }
        m_frunks.clear();
        m_frunkFound = false;
        emit frunksChanged();
    }

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error)
    {
        qDebug() << "Discovery Error: " << error;
        if (!m_stopping)
            startDiscovery();
    }

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QList<FrunkInfo *> m_frunks;
    FrunkInfo *m_placeholderFrunk = nullptr;
    bool m_frunkFound = false;
    bool m_stopping = false;

    void updatePlaceholder();
};

#endif /* FRUNK_FINDER_HPP */
