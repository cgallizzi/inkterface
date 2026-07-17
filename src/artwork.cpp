#include "artwork.hpp"

#include <QDebug>
#include <QDir>
#include <QFont>
#include <QFontMetrics>
#include <QNetworkReply>
#include <QPainter>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

// layout constants for the composed frame
#define ART_MARGIN 16
#define ART_BOX_WIDTH 300
#define ART_BOX_HEIGHT 448

Artwork::Artwork(steam::Steam *steam, QObject *parent)
    : QObject(parent)
    , m_steam(steam)
    , m_netman(new QNetworkAccessManager(this))
{
}

QByteArray Artwork::packMono(const QImage &image)
{
    // Qt's DiffuseDither is a Floyd-Steinberg style error diffusion which
    // reads well on e-ink
    QImage mono = image.convertToFormat(QImage::Format_Grayscale8)
                      .convertToFormat(QImage::Format_Mono, Qt::DiffuseDither);
    // Format_Mono is MSB-first like our wire format, but the index->color
    // mapping is not fixed, so work out which index is ink
    bool oneIsInk = true;
    const auto table = mono.colorTable();
    if (table.size() > 1) {
        oneIsInk = qGray(table.at(1)) < qGray(table.at(0));
    }
    const int rowBytes = (mono.width() + 7) / 8;
    QByteArray bits(rowBytes * mono.height(), 0);
    for (int y = 0; y < mono.height(); ++y) {
        const uchar *src = mono.constScanLine(y);
        uchar *dst = (uchar *)bits.data() + (y * rowBytes);
        if (oneIsInk) {
            memcpy(dst, src, rowBytes);
        } else {
            for (int b = 0; b < rowBytes; ++b) {
                dst[b] = ~src[b];
            }
        }
    }
    return bits;
}

void Artwork::composeForApp(const steam::App &app, double playtimeMinutes,
                            QPair<int, int> achievements)
{
    m_app = app;
    m_playtime = playtimeMinutes;
    m_achievements = achievements;

    if (m_artAppid == app.appid && !m_artImage.isNull()) {
        compose();
        return;
    }

    m_artAppid = app.appid;
    m_artImage = loadLocalBoxArt(app.appid);
    if (!m_artImage.isNull()) {
        compose();
        return;
    }

    // no local art, try the storefront CDN; portrait capsule art fits our
    // layout best
    auto req = QNetworkRequest(
        u"https://cdn.cloudflare.steamstatic.com/steam/apps/%1/library_600x900.jpg"_s.arg(
            app.appid));
    auto reply = m_netman->get(req);
    QTimer::singleShot(10000, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &Artwork::boxArtReply);
}

QImage Artwork::loadLocalBoxArt(const QString &appid)
{
    // steam has cached art under a couple of different layouts over the years
    const QString cache = m_steam->steamDir() + u"/appcache/librarycache"_s;
    const QStringList candidates = {
        u"%1/%2/library_600x900.jpg"_s.arg(cache, appid),
        u"%1/%2_library_600x900.jpg"_s.arg(cache, appid),
        u"%1/%2/header.jpg"_s.arg(cache, appid),
        u"%1/%2_header.jpg"_s.arg(cache, appid),
    };
    for (const auto &path : candidates) {
        if (QFile::exists(path)) {
            QImage img(path);
            if (!img.isNull()) {
                qDebug() << "using local box art:" << path;
                return img;
            }
        }
    }
    // newer steam builds hash-name the files inside the appid folder, take
    // the largest portrait-ish image we can find
    QDir d(u"%1/%2"_s.arg(cache, appid));
    QImage best;
    const auto entries = d.entryList({u"*.jpg"_s, u"*.png"_s}, QDir::Files);
    for (const auto &entry : entries) {
        QImage img(d.absoluteFilePath(entry));
        if (img.isNull() || img.width() > img.height()) {
            continue;
        }
        if (img.width() * img.height() > best.width() * best.height()) {
            best = img;
        }
    }
    if (!best.isNull()) {
        qDebug() << "using local box art from librarycache folder for" << appid;
    }
    return best;
}

void Artwork::boxArtReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    reply->deleteLater();
    if (reply->isOpen() && reply->error() == QNetworkReply::NoError) {
        m_artImage.loadFromData(reply->readAll());
    } else {
        qDebug() << "box art fetch failed:" << reply->errorString();
    }
    // compose regardless, the frame still reads fine without art
    compose();
}

void Artwork::compose()
{
    QImage frame(ART_FRAME_WIDTH, ART_FRAME_HEIGHT, QImage::Format_RGB32);
    frame.fill(Qt::white);
    QPainter p(&frame);
    p.setPen(Qt::black);

    int textLeft = ART_MARGIN;
    if (!m_artImage.isNull()) {
        QImage art = m_artImage.scaled(ART_BOX_WIDTH, ART_BOX_HEIGHT, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        const int artX = ART_MARGIN;
        const int artY = (ART_FRAME_HEIGHT - art.height()) / 2;
        p.drawImage(artX, artY, art);
        p.drawRect(artX - 2, artY - 2, art.width() + 3, art.height() + 3);
        p.drawRect(artX - 1, artY - 1, art.width() + 1, art.height() + 1);
        textLeft = artX + art.width() + ART_MARGIN * 2;
    }
    const int textWidth = ART_FRAME_WIDTH - textLeft - ART_MARGIN;

    // game title, wrapped, sized down until it fits
    QFont titleFont(u"IBM Plex Mono"_s, -1, QFont::Bold);
    QRect titleRect;
    for (int px = 44; px >= 22; px -= 2) {
        titleFont.setPixelSize(px);
        QFontMetrics fm(titleFont);
        titleRect = fm.boundingRect(QRect(textLeft, ART_MARGIN * 2, textWidth, 0),
                                    Qt::TextWordWrap, m_app.name);
        if (titleRect.height() <= 200) {
            break;
        }
    }
    p.setFont(titleFont);
    p.drawText(QRect(textLeft, ART_MARGIN * 2, textWidth, titleRect.height()), Qt::TextWordWrap,
               m_app.name);

    QFont bodyFont(u"IBM Plex Mono"_s, -1, QFont::Medium);
    bodyFont.setPixelSize(24);
    QFont valueFont(u"IBM Plex Mono"_s, -1, QFont::Bold);
    valueFont.setPixelSize(34);

    int y = ART_MARGIN * 2 + titleRect.height() + 40;
    if (m_playtime >= 0) {
        p.setFont(bodyFont);
        p.drawText(textLeft, y, u"PLAYTIME"_s);
        p.setFont(valueFont);
        y += 38;
        if (m_playtime < 60) {
            p.drawText(textLeft, y, u"%1 MIN"_s.arg(QString::number(m_playtime, 'f', 0)));
        } else {
            p.drawText(textLeft, y, u"%1 HRS"_s.arg(QString::number(m_playtime / 60.0, 'f', 1)));
        }
        y += 48;
    }
    if (m_achievements.second > 0) {
        p.setFont(bodyFont);
        p.drawText(textLeft, y, u"ACHIEVEMENTS"_s);
        p.setFont(valueFont);
        y += 38;
        p.drawText(textLeft, y,
                   u"%1 / %2"_s.arg(QString::number(m_achievements.first),
                                    QString::number(m_achievements.second)));
        y += 16;
        // progress bar
        const int barH = 22;
        p.drawRect(textLeft, y, textWidth - 1, barH);
        p.drawRect(textLeft + 1, y + 1, textWidth - 3, barH - 2);
        const int fill = (textWidth - 8) * m_achievements.first / m_achievements.second;
        p.fillRect(textLeft + 4, y + 4, fill, barH - 7, Qt::black);
        y += barH + 24;
    }

    // small tag in the bottom right corner
    QFont tagFont(u"IBM Plex Mono"_s, -1, QFont::Medium);
    tagFont.setPixelSize(16);
    p.setFont(tagFont);
    p.drawText(QRect(textLeft, ART_FRAME_HEIGHT - ART_MARGIN - 20, textWidth, 20),
               Qt::AlignRight | Qt::AlignBottom, u"NOW PLAYING"_s);
    p.end();

    emit frameReady(packMono(frame), ART_FRAME_WIDTH, ART_FRAME_HEIGHT);
}
