/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2012 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Mappero.
 *
 * Mappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "controller.h"
#include "debug.h"
#include "taggable.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QPixmap>
#include <exiv2/image.hpp>
#include <exiv2/preview.hpp>

using namespace Mappero;

static Taggable::ImageProvider *imageProvider = 0;
static int uniqueId = 1;
#ifdef XDG_THUMBNAILS
static bool thumbnailsDirChecked = false;
static QString thumbnailsDir;
#endif

namespace Mappero {

class TaggablePrivate
{
    Q_DECLARE_PUBLIC(Taggable)

    TaggablePrivate(Taggable *taggable);
    ~TaggablePrivate();

    void loadExifInfo();
    void readGeoData(const Exiv2::ExifData &exifData);
#ifdef XDG_THUMBNAILS
    QPixmap cachedThumbnail(QSize *size, const QSize &requestedSize) const;
#endif
    QPixmap preview(QSize *size, const QSize &requestedSize) const;
    static QString exifString(const Exiv2::Value &value) {
        return QString::fromAscii(value.toString().c_str());
    }
    static Geo exifGeoCoord(const Exiv2::Value &value, bool positive);
    void setNeedsSave(bool needsSave);

private:
    mutable Taggable *q_ptr;
    QString fileName;
    time_t time;
    Exiv2::Image::AutoPtr image;
    GeoPoint geoPoint;
    bool needsSave;
    qint64 lastChange;
};
}; // namespace

TaggablePrivate::TaggablePrivate(Taggable *taggable):
    q_ptr(taggable),
    time(0),
    image(0),
    needsSave(false),
    lastChange(0)
{
}

TaggablePrivate::~TaggablePrivate()
{
}

void TaggablePrivate::loadExifInfo()
{
    // reset the cached data
    time = 0;
    geoPoint = GeoPoint();
    lastChange = Controller::clock();

    // load the EXIF data
    image = Exiv2::ImageFactory::open(fileName.toUtf8().constData());
    image->readMetadata();
    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        DEBUG() << "No exif data";
        return;
    }

    Exiv2::ExifKey key("Exif.Photo.DateTimeOriginal");
    Exiv2::ExifData::iterator i = exifData.findKey(key);
    if (i != exifData.end()) {
        QString timeString =
            QString::fromAscii(i->value().toString().c_str());
        time = QDateTime::fromString(timeString,
                                     "yyyy:MM:dd HH:mm:ss").toTime_t();
    }

    readGeoData(exifData);
        /*
        Exiv2::ExifData::const_iterator end = exifData.end();
        for (Exiv2::ExifData::const_iterator i = exifData.begin();
             i != end;
             ++i) {
            const char *tn = i->typeName();
            DEBUG() << QString::fromStdString(i->key()) <<
                "tag" << i->tag() <<
                "type" << (tn ? tn : "Unknown") <<
                "count" << i->count() <<
                "value" << QString::fromStdString(i->value().toString());
        }
        */
}

void TaggablePrivate::readGeoData(const Exiv2::ExifData &exifData)
{
    QString latRef;
    Exiv2::ExifData::const_iterator i =
        exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitudeRef"));
    if (i != exifData.end()) {
        latRef = exifString(i->value());
    }
    if (latRef.isEmpty() || (latRef[0] != 'N' && latRef[0] != 'S')) return;

    QString lonRef;
    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitudeRef"));
    if (i != exifData.end()) {
        lonRef = exifString(i->value());
    }
    if (lonRef.isEmpty() || (lonRef[0] != 'E' && lonRef[0] != 'W')) return;

    Geo lat, lon;

    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitude"));
    if (i == exifData.end()) return;
    lat = exifGeoCoord(i->value(), latRef[0] == 'N');

    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitude"));
    if (i == exifData.end()) return;
    lon = exifGeoCoord(i->value(), lonRef[0] == 'E');

    geoPoint = GeoPoint(lat, lon);
}

Geo TaggablePrivate::exifGeoCoord(const Exiv2::Value &value, bool positive)
{
    Geo geo;

    int count = value.count();
    if (count <= 0) return 0;

    geo = value.toFloat(0);
    if (count > 1) geo += value.toFloat(1) / 60.0;
    if (count > 2) geo += value.toFloat(2) / 3600.0;
    return positive ? geo : -geo;
}

#ifdef XDG_THUMBNAILS
static inline bool hasXdgThumbnails()
{
    if (!thumbnailsDirChecked) {
        thumbnailsDirChecked = true;
        QString newStandard(QDir::homePath() + "/.cache/thumbnails/large/");
        QString oldStandard(QDir::homePath() + "/.thumbnails/large/");
        if (QDir(newStandard).isReadable()) {
            thumbnailsDir = newStandard;
        } else if (QDir(oldStandard).isReadable()) {
            thumbnailsDir = oldStandard;
        }
    }
    return !thumbnailsDir.isEmpty();
}

QPixmap TaggablePrivate::cachedThumbnail(QSize *size, const QSize &) const
{
    if (!hasXdgThumbnails()) return QPixmap();
    QByteArray ba;
    QUrl uri = QUrl::fromLocalFile(fileName);
    ba = QCryptographicHash::hash(uri.toEncoded(), QCryptographicHash::Md5);
    QPixmap thumbnail(thumbnailsDir + QString::fromAscii(ba.toHex()));
    if (size != 0) *size = thumbnail.size();
    return thumbnail;
}
#endif

QPixmap TaggablePrivate::preview(QSize *size,
                                 const QSize &requestedSize) const
{
    Exiv2::PreviewManager loader(*image);
    Exiv2::PreviewPropertiesList list = loader.getPreviewProperties();

    if (list.empty()) return QPixmap();

    Exiv2::PreviewPropertiesList::const_iterator i, best;
    QSize previewSize;
    for (i = list.begin(); i != list.end(); i++) {
        best = i;
        if (i->width_ >= uint(requestedSize.width()) &&
            i->height_ >= uint(requestedSize.height()))
            break;
    }

    previewSize = QSize(best->width_, best->height_);
    if (size != 0) *size = previewSize;

    Exiv2::PreviewImage exivPreview = loader.getPreviewImage(*best);
    QImage preview;
    preview.loadFromData(exivPreview.pData(), exivPreview.size());

    /* Handle image orientation */
    Exiv2::ExifData &exifData = image->exifData();
    QString orientationString =
        exifData["Exif.Thumbnail.Orientation"].toString().c_str();
    if (orientationString.isEmpty()) {
        orientationString =
            exifData["Exif.Image.Orientation"].toString().c_str();
    }
    int orientation = orientationString.toInt();
    if (orientation < 1 || orientation > 8) orientation = 1;
    int rotation, mirror;

    switch (orientation) {
    case 1: rotation = 0; mirror = 0; break;
    case 2: rotation = 0; mirror = 1; break;
    case 3: rotation = 2; mirror = 0; break;
    case 4: rotation = 2; mirror = 1; break;
    case 5: rotation = 1; mirror = 1; break;
    case 6: rotation = 1; mirror = 0; break;
    case 7: rotation = 3; mirror = 1; break;
    case 8: rotation = 3; mirror = 0; break;
    }

    QTransform transformation;
    if (!requestedSize.isEmpty()) {
        QSize newSize = preview.size();
        newSize.scale(requestedSize, Qt::KeepAspectRatioByExpanding);
        newSize.rwidth() = qMax(newSize.width(), 1);
        newSize.rheight() = qMax(newSize.height(), 1);
        transformation.scale((qreal)newSize.width() / preview.width(),
                             (qreal)newSize.height() / preview.height());
    }

    if (rotation != 0) {
        transformation.rotate(rotation * 90);
    }

    if (mirror != 0) {
        transformation.scale(-1, 1);
    }

    preview = preview.transformed(transformation, Qt::SmoothTransformation);
    return QPixmap::fromImage(preview);
}

void TaggablePrivate::setNeedsSave(bool needsSave)
{
    Q_Q(Taggable);
    if (needsSave) {
        lastChange = Controller::clock();
    }

    if (needsSave == this->needsSave) return;
    this->needsSave = needsSave;
    Q_EMIT q->needsSaveChanged();
}

Taggable::Taggable(QObject *parent):
    QObject(parent),
    d_ptr(new TaggablePrivate(this))
{
    QString id = QString("t%1").arg(uniqueId++);
    setObjectName(id);
    ImageProvider::instance()->taggables[id] = this;
}

Taggable::~Taggable()
{
    imageProvider->taggables.remove(objectName());
    delete d_ptr;
}

QUrl Taggable::pixmapUrl() const
{
    QUrl url;
    url.setScheme("image");
    url.setAuthority(ImageProvider::name());
    url.setPath("/" + objectName());
    return url;
}

void Taggable::setFileName(const QString &fileName)
{
    Q_D(Taggable);

    if (d->fileName == fileName) return;

    d->fileName = fileName;
    d->loadExifInfo();
    Q_EMIT fileNameChanged();
}

QString Taggable::fileName() const
{
    Q_D(const Taggable);
    return d->fileName;
}

void Taggable::setLocation(const GeoPoint &location)
{
    Q_D(Taggable);

    if (location == d->geoPoint) return;

    d->geoPoint = location;
    Q_EMIT locationChanged();

    d->setNeedsSave(true);
}

GeoPoint Taggable::location() const
{
    Q_D(const Taggable);
    return d->geoPoint;
}

bool Taggable::hasLocation() const
{
    Q_D(const Taggable);
    return d->geoPoint.isValid();
}

bool Taggable::needsSave() const
{
    Q_D(const Taggable);
    return d->needsSave;
}

qint64 Taggable::lastChange() const
{
    Q_D(const Taggable);
    return d->lastChange;
}

time_t Taggable::time() const
{
    Q_D(const Taggable);
    return d->time;
}

QPixmap Taggable::pixmap(QSize *size, const QSize &requestedSize) const
{
    Q_D(const Taggable);

#ifdef XDG_THUMBNAILS
    QPixmap thumbnail = d->cachedThumbnail(size, requestedSize);
    if (!thumbnail.isNull()) return thumbnail;
#endif

    QPixmap preview = d->preview(size, requestedSize);
    if (!preview.isNull()) return preview;

    /* If a thumbnail is not available, use the image itself */
    QPixmap pixmap(d->fileName);
    if (size != 0) *size = pixmap.size();
    if (!requestedSize.isEmpty()) {
        pixmap = pixmap.scaled(requestedSize,
                               Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);
    }
    return pixmap;
}

void Taggable::open() const
{
    Q_D(const Taggable);
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->fileName));
}

void Taggable::reload()
{
    Q_D(Taggable);

    GeoPoint oldLocation = d->geoPoint;
    bool oldNeedsSave = d->needsSave;

    d->loadExifInfo();
    d->needsSave = false;

    if (oldNeedsSave) {
        Q_EMIT needsSaveChanged();
    }
    if (oldLocation != d->geoPoint) {
        Q_EMIT locationChanged();
    }
}

Taggable::ImageProvider::ImageProvider():
    QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap)
{
    if (imageProvider != 0) {
        qWarning() << "Second instance of Taggable::ImageProvider!";
        return;
    }
    imageProvider = this;
}

Taggable::ImageProvider::~ImageProvider()
{
    imageProvider = 0;
}

QString Taggable::ImageProvider::name()
{
    return "taggable";
}

Taggable::ImageProvider *Taggable::ImageProvider::instance()
{
    if (imageProvider == 0) {
        imageProvider = new Taggable::ImageProvider();
    }

    return imageProvider;
}

QPixmap Taggable::ImageProvider::requestPixmap(const QString &id,
                                               QSize *size,
                                               const QSize &requestedSize)
{
    if (!taggables.contains(id)) {
        qWarning() << "Taggable not found:" << id;
        return QPixmap();
    }

    return taggables.value(id)->pixmap(size, requestedSize);
}
