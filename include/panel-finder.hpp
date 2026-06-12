#ifndef PANEL_FINDER_HPP
#define PANEL_FINDER_HPP

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QtQmlIntegration/qqmlintegration.h>

using namespace Qt::StringLiterals;

class PanelInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Backend only.")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(qint16 rssi READ rssi CONSTANT)
    Q_PROPERTY(QString ifaceVersion READ ifaceVersion CONSTANT)
    Q_PROPERTY(bool supported READ supported CONSTANT)

  public:
    explicit PanelInfo(const QBluetoothDeviceInfo &info, QObject *parent = nullptr)
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

class PanelFinder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<PanelInfo *> panels MEMBER m_panels NOTIFY panelsChanged)
    Q_PROPERTY(bool panelFound READ panelFound NOTIFY panelsChanged)
    Q_PROPERTY(PanelInfo *panel READ panel NOTIFY panelsChanged)

  signals:
    void panelsChanged();

  public:
    explicit PanelFinder(QObject *parent = nullptr);
    ~PanelFinder()
    {
        if (m_discoveryAgent) {
            m_discoveryAgent->stop();
        }
    }

    bool panelFound() const { return m_panelFound; }
    QPointer<PanelInfo> panel()
    {
        updatePlaceholder();
        auto panelName = m_placeholderPanel->name();
        for (auto panel : std::as_const(m_panels)) {
            if (panel->name() == panelName) {
                return panel;
            }
        }
        return m_placeholderPanel;
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
        for (auto panel : std::as_const(m_panels)) {
            panel->deleteLater();
        }
        m_panels.clear();
        m_panelFound = false;
        emit panelsChanged();
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
    QList<PanelInfo *> m_panels;
    PanelInfo *m_placeholderPanel = nullptr;
    bool m_panelFound = false;
    bool m_stopping = false;

    void updatePlaceholder();
};

#endif /* PANEL_FINDER_HPP */
