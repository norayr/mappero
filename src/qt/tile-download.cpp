/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2011-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "debug.h"
#include "tile-download.h"
#include "tiled-layer.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkSession>
#include <QRunnable>
#include <QThreadPool>
#include <QUrl>

using namespace Mappero;

namespace Mappero {

struct TileTask
{
    int priority;
    TileSpec spec;
    QString fileName;
    QUrl url;

    TileTask(): priority(0), spec(0, 0, 0, 0) {}
    TileTask(const TileSpec &spec, int priority):
        priority(priority),
        spec(spec)
    {
        TiledLayer *layer =
            qobject_cast<TiledLayer*>(Layer::find(spec.layerId));
        if (Q_LIKELY(layer)) {
            fileName = layer->tileFileName(spec.zoom, spec.x, spec.y);
            url = layer->urlForTile(spec.zoom, spec.x, spec.y);
        }
    }
    ~TileTask() {}
};

struct TaskData
{
    enum Status {
        Queued = 0,
        InProgress,
        Completed,
    };

    Status status;
    TileContents tileContents;

    TaskData():
        status(Queued),
        tileContents()
    {}
};

typedef QMap<TileTask,TaskData> TaskMap;

inline bool operator<(const TileTask &t1, const TileTask &t2)
{
    int diff;
    diff = t1.priority - t2.priority;
    if (diff != 0) return diff < 0;

    /* We don't really care about this, but we need to provide a global sorting
     */
    diff = t1.spec.x - t2.spec.x;
    if (diff != 0) return diff < 0;
    diff = t1.spec.y - t2.spec.y;
    if (diff != 0) return diff < 0;
    diff = t1.spec.zoom - t2.spec.zoom;
    if (diff != 0) return diff < 0;

    return t1.spec.layerId < t2.spec.layerId;
}

class Downloader: public QRunnable
{
public:
    Downloader(TaskMap &tasks, QMutex &mutex,
               QNetworkConfigurationManager *ncm,
               QObject *listener);
    ~Downloader();

    // reimplemented virtual method
    void run();

private:
    void processTask(const TileTask &tile);
    QByteArray downloadTile(const TileTask &tile);

private:
    TaskMap &tasks;
    QMutex &mutex;
    QNetworkConfigurationManager *ncm;
    QObject *listener;
    QNetworkAccessManager *networkAccessManager;
};

class TileDownloadPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(TileDownload)

    inline TileDownloadPrivate(TileDownload *tileDownload);
    ~TileDownloadPrivate() {};

public Q_SLOTS:
    void taskCompleted(const TileTask &t);

private:
    void requestTile(const TileTask &task);

private:
    mutable TileDownload *q_ptr;
    bool networkRequested;
    TaskMap tasks;
    QMutex tasksMutex;
    QThreadPool *pool;
    QNetworkConfigurationManager ncm;
};
}; // namespace

Q_DECLARE_METATYPE(Mappero::TileTask)

TileDownloadPrivate::TileDownloadPrivate(TileDownload *tileDownload):
    QObject(tileDownload),
    q_ptr(tileDownload),
    networkRequested(false)
{
    pool = QThreadPool::globalInstance();
    qRegisterMetaType<TileTask>("TileTask");

    QObject::connect(&ncm,
                     SIGNAL(onlineStateChanged(bool)),
                     q_ptr,
                     SIGNAL(onlineStateChanged(bool)));
}

inline QDebug operator<<(QDebug dbg, const TileTask &t)
{
    dbg.nospace() << t.spec;
    return dbg.space();
}

Downloader::Downloader(TaskMap &tasks, QMutex &mutex,
                       QNetworkConfigurationManager *ncm,
                       QObject *listener):
    tasks(tasks),
    mutex(mutex),
    ncm(ncm),
    listener(listener)
{
}

Downloader::~Downloader()
{
}

void Downloader::run()
{
    bool hasTasks;

    networkAccessManager = new QNetworkAccessManager;

    do {
        /* pick a task */
        const TileTask *t;
        hasTasks = false;
        mutex.lock();
        TaskMap::iterator i = tasks.begin();
        while (i != tasks.constEnd()) {
            if (i.value().status == TaskData::Queued) {
                hasTasks = true;
                /* mark the task as in progress so that other threads won't try
                 * to take it */
                i.value().status = TaskData::InProgress;
                t = &i.key();
                break;
            }
            i++;
        }
        mutex.unlock();

        if (hasTasks)
            processTask(*t);
    } while (hasTasks);

    delete networkAccessManager;
    networkAccessManager = 0;
}

void Downloader::processTask(const TileTask &tile)
{
    mutex.lock();
    TaskData &data = tasks[tile];
    mutex.unlock();

    QFile tileFile(tile.fileName);
    if (tileFile.exists() &&
        tileFile.open(QIODevice::ReadOnly)) {
        data.tileContents.image = tileFile.readAll();
    } else {
#ifndef Q_OS_WIN32
        if (ncm->isOnline()) {
#else
        if (1) {
#endif
            data.tileContents.image = downloadTile(tile);
            /* save the tile */
            if (!data.tileContents.image.isEmpty()) {
                QFileInfo info(tileFile.fileName());
                info.dir().mkpath(QLatin1String("."));
                if (tileFile.open(QIODevice::WriteOnly)) {
                    tileFile.write(data.tileContents.image);
                } else {
                    qWarning() << "Couldn't save tile" << tileFile.fileName();
                }
            }
        } else {
            /* TODO: upscale/downscale tiles as needed */
            data.tileContents.needsNetwork = true;
        }
    }

    data.status = TaskData::Completed;
    QMetaObject::invokeMethod(listener, "taskCompleted", Qt::AutoConnection,
                              Q_ARG(TileTask, tile));
}

QByteArray Downloader::downloadTile(const TileTask &tile)
{
    QNetworkRequest request(tile.url);
    request.setRawHeader("User-Agent", "Mappero 1.0");
    QNetworkReply *reply = networkAccessManager->get(request);

    /* QNetworkReply does not implement QIODevice::waitForReadyRead(); let's
     * run an event loop till the request is finished */
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()),
                     &loop, SLOT(quit()));
    loop.exec();

    QByteArray tileData;
    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError) {
        DEBUG() << "Got error" << error << reply->errorString();
    } else {
        tileData = reply->readAll();
    }
    delete reply;

    return tileData;
}

void TileDownloadPrivate::taskCompleted(const TileTask &tile)
{
    Q_Q(TileDownload);

    tasksMutex.lock();
    TaskMap::iterator t = tasks.find(tile);
    TaskData &data = t.value();

    Q_EMIT q->tileDownloaded(tile.spec, data.tileContents);

    /* Note: we can destroy the TaskData only because we know that the signal
     * connection is immediate; otherwise the receiver might end up accessing
     * deleted data */
    bool needsNetwork = data.tileContents.needsNetwork;
    tasks.erase(t);
    tasksMutex.unlock();

    if (needsNetwork && !networkRequested) {
        networkRequested = true;
        QNetworkConfiguration cfg = ncm.defaultConfiguration();
        QNetworkSession *session = new QNetworkSession(cfg, this);
        session->open();
    }
}

void TileDownloadPrivate::requestTile(const TileTask &task)
{
    tasksMutex.lock();
    /* Check if the task is already in progress */
    TaskMap::iterator t = tasks.find(task);
    if (t != tasks.end() && t.value().status >= TaskData::InProgress) {
        tasksMutex.unlock();
        return;
    }
    tasks.insert(task, TaskData());
    tasksMutex.unlock();

    if (pool->activeThreadCount() < pool->maxThreadCount()) {
        Downloader *downloader = new Downloader(tasks, tasksMutex, &ncm, this);
        pool->start(downloader);
    }
}

TileDownload::TileDownload(QObject *parent):
    QObject(parent),
    d_ptr(new TileDownloadPrivate(this))
{
}

TileDownload::~TileDownload()
{
    delete d_ptr;
}

void TileDownload::requestTile(const TileSpec &tileSpec, int priority)
{
    Q_D(TileDownload);

    TileTask task(tileSpec, priority);
    if (Q_UNLIKELY(task.fileName.isEmpty())) {
        qWarning() << "Tile task has no filename, skipping";
        return;
    }
    d->requestTile(task);
}

#include "tile-download.moc"
