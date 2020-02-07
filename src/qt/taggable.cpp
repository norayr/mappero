/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include <QDesktopServices>
#include <QDir>
#include <QFuture>
#include <QPixmap>
#include <QtConcurrent>
#include <exiv2/exiv2.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>


using namespace Mappero;

static Taggable::ImageProvider *imageProvider = 0;
static int uniqueId = 1;
#ifdef XDG_THUMBNAILS
static bool thumbnailsDirChecked = false;
static QString thumbnailsDir;
#endif

namespace Mappero {

bool saveMetadata(Exiv2::Image *image)
{
    std::string fileName = image->io().path();
    struct stat statBefore;
    stat(fileName.c_str(), &statBefore);

    try {
        image->writeMetadata();
    } catch (Exiv2::AnyError &e) {
        qCritical() << "Exiv2 exception, code" << e.code();
        return false;
    }

    /* preserve modification time */
    struct stat statAfter;
    stat(fileName.c_str(), &statAfter);
    struct utimbuf times;
    times.actime = statAfter.st_atime;
    times.modtime = statBefore.st_mtime;
    utime(fileName.c_str(), &times);

    return true;
}

class TaggablePrivate
{
    Q_DECLARE_PUBLIC(Taggable)

    TaggablePrivate(Taggable *taggable);
    ~TaggablePrivate();

    void setLocation(const GeoPoint &location, GeoPoint &dest);
    GeoPoint location() const;

    void loadExifInfo();
    static GeoPoint readExifGeoLocation(const Exiv2::ExifData &exifData);
    void readGeoData(const Exiv2::Image &image);
    void deleteLocationInfo();
    void save();

#ifdef XDG_THUMBNAILS
    QPixmap cachedThumbnail(QSize *size, const QSize &requestedSize) const;
#endif
    QPixmap preview(QSize *size, const QSize &requestedSize) const;
    static QString exifString(const Exiv2::Value &value) {
        return QString::fromLatin1(value.toString().c_str());
    }
    static Geo exifGeoCoord(const Exiv2::Value &value, bool positive);

private:
    mutable Taggable *q_ptr;
    QString fileName;
    time_t time;
    Exiv2::Image::AutoPtr image;
    GeoPoint correlatedGeoPoint;
    GeoPoint manualGeoPoint;
    GeoPoint exifGeoPoint;
    GeoPoint fileGeoPoint;
    qint64 lastChange;
    QFuture<bool> m_saveFuture;
    QFutureWatcher<bool> m_saveFutureWatcher;
};
}; // namespace

TaggablePrivate::TaggablePrivate(Taggable *taggable):
    q_ptr(taggable),
    time(0),
    image(0),
    correlatedGeoPoint(),
    manualGeoPoint(),
    exifGeoPoint(),
    fileGeoPoint(),
    lastChange(0)
{
    QObject::connect(&m_saveFutureWatcher, SIGNAL(finished()),
                     taggable, SIGNAL(saveStateChanged()));
}

TaggablePrivate::~TaggablePrivate()
{
    if (!m_saveFuture.isFinished()) {
        qWarning() << "Saving not complete!";
    }
}

void TaggablePrivate::setLocation(const GeoPoint &point, GeoPoint &dest)
{
    Q_Q(Taggable);

    if (point == dest) return;

    lastChange = Controller::clock();
    bool oldNeedsSave = (location() != fileGeoPoint);
    dest = point;
    Q_EMIT q->locationChanged();

    if ((location() != fileGeoPoint) != oldNeedsSave) {
        Q_EMIT q->saveStateChanged();
    }
}

GeoPoint TaggablePrivate::location() const
{
    if (manualGeoPoint.isValid()) return manualGeoPoint;
    if (correlatedGeoPoint.isValid()) return correlatedGeoPoint;
    return exifGeoPoint;
}

void TaggablePrivate::loadExifInfo()
{
    // reset the cached data
    time = 0;
    lastChange = Controller::clock();
    fileGeoPoint = GeoPoint();

    // load the EXIF data
    try {
        image = Exiv2::ImageFactory::open(fileName.toUtf8().constData());
        image->readMetadata();
    } catch (Exiv2::AnyError &e) {
        DEBUG() << "Exiv2 exception, code" << e.code();
        image.reset();
        return;
    }
    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        DEBUG() << "No exif data";
        return;
    }

    Exiv2::ExifKey key("Exif.Photo.DateTimeOriginal");
    Exiv2::ExifData::iterator i = exifData.findKey(key);
    if (i != exifData.end()) {
        QString timeString =
            QString::fromLatin1(i->value().toString().c_str());
        time = QDateTime::fromString(timeString,
                                     "yyyy:MM:dd HH:mm:ss").toTime_t();
    }

    readGeoData(*image);
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

GeoPoint TaggablePrivate::readExifGeoLocation(const Exiv2::ExifData &exifData)
{
    QString latRef;
    Exiv2::ExifData::const_iterator i =
        exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitudeRef"));
    if (i != exifData.end()) {
        latRef = exifString(i->value());
    }
    if (latRef.isEmpty() || (latRef[0] != 'N' && latRef[0] != 'S'))
        return GeoPoint();

    QString lonRef;
    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitudeRef"));
    if (i != exifData.end()) {
        lonRef = exifString(i->value());
    }
    if (lonRef.isEmpty() || (lonRef[0] != 'E' && lonRef[0] != 'W'))
        return GeoPoint();

    Geo lat, lon;

    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitude"));
    if (i == exifData.end()) return GeoPoint();
    lat = exifGeoCoord(i->value(), latRef[0] == 'N');

    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitude"));
    if (i == exifData.end()) return GeoPoint();
    lon = exifGeoCoord(i->value(), lonRef[0] == 'E');

    return GeoPoint(lat, lon);
}

struct GeoCoord {
    GeoCoord(const Exiv2::Value &v): positive(true) {
        int count = v.count();
        coord[0] = count > 0 ? v.toFloat(0) : NAN;
        coord[1] = count > 1 ? v.toFloat(1) : 0;
        coord[2] = count > 2 ? v.toFloat(2) : 0;
    }
    GeoCoord(const QStringList &parts): positive(true) {
        int count = parts.count();
        coord[0] = count > 0 ? parts[0].toFloat() : NAN;
        coord[1] = count > 1 ? parts[1].toFloat() : 0;
        coord[2] = count > 2 ? parts[2].toFloat() : 0;
    }
    void setPositive(bool p) { positive = p; }
    Geo toGeo() {
        Geo sum = coord[0] + coord[1] / 60.0 + coord[2] / 3600.0;
        return positive ? sum : -sum;
    }
private:
    Geo coord[3];
    bool positive;
};

static Geo parseXmpGeoCoord(const QString &s)
{
    if (s.isEmpty()) return 200.0;
    QChar reference = s.at(s.length() - 1).toUpper();
    QStringList coords = s.left(s.length() -1).split(',');
    GeoCoord c(coords);
    if (reference == 'W' || reference == 'S') {
        c.setPositive(false);
    }
    return c.toGeo();
}

static std::string geoToXmp(Geo geo, Qt::Orientation orientation)
{
    char direction;

    /* Vertical orientation means latitude, horizonal means longitude */
    if (geo < 0) {
        geo = -geo;
        direction = (orientation == Qt::Vertical) ? 'S' : 'W';
    } else {
        direction = (orientation == Qt::Vertical) ? 'N' : 'E';
    }

    int degrees = GFLOOR(geo);
    double minutes = (geo - degrees) * 60.0;

    QString coord = "%1,%2%3";
    coord = coord.arg(degrees).arg(minutes, 0, 'f', 8).arg(direction);
    return coord.toStdString();
}

void TaggablePrivate::readGeoData(const Exiv2::Image &image)
{
    const Exiv2::XmpData &xmpData = image.xmpData();
    Exiv2::XmpData::const_iterator i =
        xmpData.findKey(Exiv2::XmpKey("Xmp.exif.GPSLatitude"));
    if (i != xmpData.end()) {
        QString lat = QString::fromStdString(i->value().toString());
        i = xmpData.findKey(Exiv2::XmpKey("Xmp.exif.GPSLongitude"));
        if (i != xmpData.end()) {
            QString lon = QString::fromStdString(i->value().toString());
            fileGeoPoint = GeoPoint(parseXmpGeoCoord(lat),
                                    parseXmpGeoCoord(lon));
            return;
        }
    }

    /* Else, read the location from the EXIF data */
    fileGeoPoint = readExifGeoLocation(image.exifData());
}

static Exiv2::URationalValue geoToExif(Geo geo)
{
    Geo coord = qAbs(geo);

    int degrees = GFLOOR(coord);
    coord -= degrees;
    int icoord = coord * 60 * 3600;

    int minutes = icoord / 3600;
    int seconds = icoord % 3600;

    Exiv2::URationalValue value;
    value.value_.push_back(std::make_pair(degrees, 1));
    value.value_.push_back(std::make_pair(minutes, 1));
    value.value_.push_back(std::make_pair(seconds, 60));
    return value;
}

void TaggablePrivate::deleteLocationInfo()
{
    Exiv2::XmpData &xmpData = image->xmpData();
    Exiv2::XmpData::iterator x;

    x = xmpData.findKey(Exiv2::XmpKey("Xmp.exif.GPSLatitude"));
    if (x != xmpData.end()) xmpData.erase(x);
    x = xmpData.findKey(Exiv2::XmpKey("Xmp.exif.GPSLongitude"));
    if (x != xmpData.end()) xmpData.erase(x);

    Exiv2::ExifData &exifData = image->exifData();
    Exiv2::ExifData::iterator i;

    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitudeRef"));
    if (i != exifData.end()) exifData.erase(i);
    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitudeRef"));
    if (i != exifData.end()) exifData.erase(i);
    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitude"));
    if (i != exifData.end()) exifData.erase(i);
    i = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitude"));
    if (i != exifData.end()) exifData.erase(i);
}

void TaggablePrivate::save()
{
    Exiv2::ExifData &exifData = image->exifData();
    Exiv2::XmpData &xmpData = image->xmpData();

    GeoPoint geoPoint = location();
    if (geoPoint.isValid()) {
        xmpData["Xmp.exif.GPSLatitude"] = geoToXmp(geoPoint.lat,
                                                   Qt::Vertical);
        xmpData["Xmp.exif.GPSLongitude"] = geoToXmp(geoPoint.lon,
                                                    Qt::Horizontal);

        exifData["Exif.GPSInfo.GPSLatitudeRef"] = geoPoint.lat >= 0 ? "N" : "S";
        exifData["Exif.GPSInfo.GPSLatitude"] = geoToExif(geoPoint.lat);
        exifData["Exif.GPSInfo.GPSLongitudeRef"] = geoPoint.lon >= 0 ? "E" : "W";
        exifData["Exif.GPSInfo.GPSLongitude"] = geoToExif(geoPoint.lon);
    } else {
        deleteLocationInfo();
    }

    m_saveFuture = QtConcurrent::run(saveMetadata, image.get());
    m_saveFutureWatcher.setFuture(m_saveFuture);

    if (fileGeoPoint != geoPoint) {
        Q_Q(Taggable);
        fileGeoPoint = geoPoint;
        Q_EMIT q->saveStateChanged();
    }
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
    QPixmap thumbnail(thumbnailsDir + QString::fromLatin1(ba.toHex()));
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

bool Taggable::isValid() const
{
    Q_D(const Taggable);

    return d->image.get() != 0;
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
    d->exifGeoPoint = d->fileGeoPoint;
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

    d->setLocation(location, d->manualGeoPoint);
}

void Taggable::clearLocation()
{
    Q_D(Taggable);

    GeoPoint invalid;
    d->setLocation(invalid, d->manualGeoPoint);
    d->setLocation(invalid, d->correlatedGeoPoint);
    d->setLocation(invalid, d->exifGeoPoint);
}

void Taggable::setCorrelatedLocation(const GeoPoint &location)
{
    Q_D(Taggable);

    d->setLocation(location, d->correlatedGeoPoint);
}

GeoPoint Taggable::location() const
{
    Q_D(const Taggable);
    return d->location();
}

bool Taggable::hasLocation() const
{
    Q_D(const Taggable);
    return d->location().isValid();
}

Taggable::SaveState Taggable::saveState() const
{
    Q_D(const Taggable);
    if (d->m_saveFuture.isFinished()) {
        return (d->location() != d->fileGeoPoint) ? NeedsSave : Unchanged;
    } else {
        return Saving;
    }
}

qint64 Taggable::lastChange() const
{
    Q_D(const Taggable);
    return d->lastChange;
}

QDateTime Taggable::time() const
{
    Q_D(const Taggable);
    return QDateTime::fromTime_t(d->time);
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

    bool oldSaveState = saveState();
    GeoPoint oldLocation = location();
    d->manualGeoPoint = d->correlatedGeoPoint = GeoPoint();
    d->exifGeoPoint = d->fileGeoPoint;
    d->lastChange = Controller::clock();

    if (oldSaveState != Unchanged) {
        Q_EMIT saveStateChanged();
    }
    if (oldLocation != location()) {
        Q_EMIT locationChanged();
    }
}

void Taggable::save()
{
    Q_D(Taggable);

    d->save();
}

Taggable::ImageProvider::ImageProvider():
    QQuickImageProvider(QQmlImageProviderBase::Pixmap)
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
