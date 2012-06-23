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

#include "debug.h"
#include "taggable.h"

#include <QDateTime>
#include <QPixmap>
#include <exiv2/image.hpp>
#include <exiv2/preview.hpp>

using namespace Mappero;

static Taggable::ImageProvider *imageProvider = 0;
static int uniqueId = 1;

namespace Mappero {

class TaggablePrivate
{
    Q_DECLARE_PUBLIC(Taggable)

    TaggablePrivate(Taggable *taggable);
    ~TaggablePrivate();

    void loadExifInfo();
    QPixmap preview(QSize *size, const QSize &requestedSize) const;

private:
    mutable Taggable *q_ptr;
    QString fileName;
    time_t time;
    Exiv2::Image::AutoPtr image;
};
}; // namespace

TaggablePrivate::TaggablePrivate(Taggable *taggable):
    q_ptr(taggable),
    time(0),
    image(0)
{
}

TaggablePrivate::~TaggablePrivate()
{
}

void TaggablePrivate::loadExifInfo()
{
    // reset the cached data
    time = 0;

    // load the EXIF data
    image = Exiv2::ImageFactory::open(fileName.toStdString());
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
    QPixmap preview;
    preview.loadFromData(exivPreview.pData(), exivPreview.size());
    return preview;
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

time_t Taggable::time() const
{
    Q_D(const Taggable);
    return d->time;
}

QPixmap Taggable::pixmap(QSize *size, const QSize &requestedSize) const
{
    Q_D(const Taggable);

    QPixmap preview = d->preview(size, requestedSize);
    if (!preview.isNull()) return preview;

    /* If a thumbnail is not available, use the image itself */
    QPixmap pixmap(d->fileName);
    if (size != 0) *size = pixmap.size();
    if (requestedSize.isValid()) {
        pixmap = pixmap.scaled(requestedSize,
                               Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);
    }
    return pixmap;
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

