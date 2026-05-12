#ifndef FRUNK_FINDER_HPP
#define FRUNK_FINDER_HPP

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <QSettings>
#include <QtQml>

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
    explicit FrunkInfo(const QBluetoothDeviceInfo &info, QObject *parent = nullptr);

    QString name() const { return m_info.name(); }
    qint16 rssi() const { return m_info.rssi(); }
    QString ifaceVersion() const { return QString::fromLatin1(m_info.manufacturerData(0x055d)); }
    bool supported() const { return ifaceVersion() == u"FRv01"_s; }
    const QBluetoothDeviceInfo& bleInfo() const { return m_info; }

  private:
    QBluetoothDeviceInfo m_info;
};

class FrunkFinder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<FrunkInfo *> frunks MEMBER m_frunks NOTIFY frunksChanged)
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

    QPointer<FrunkInfo> frunk() const
    {
        QSettings settings;
        auto frunkName = settings.value(u"frunkName"_s).toString();
        if (frunkName.isEmpty()) {
            return nullptr;
        }
        for (auto frunk : m_frunks) {
            if (frunk->name() == frunkName) {
                return frunk;
            }
        }
        return m_placeholderFrunk;
    }

  private slots:
    void onDiscoveryEnded();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);

  private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QList<FrunkInfo *> m_frunks;
    FrunkInfo *m_placeholderFrunk = nullptr;

    void startDiscovery();
};

#endif /* FRUNK_FINDER_HPP */
