/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2011 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "tile-download.moc.hpp"
#include "tiled-layer.h"

#include <QEventLoop>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRunnable>
#include <QThreadPool>
#include <QUrl>

using namespace Mappero;

QDebug operator<<(QDebug dbg, const TileTask &t)
{
    dbg.nospace() << "(" << t.x << ", " << t.y << ")";
    return dbg.space();
}

namespace Mappero {

struct TaskData
{
    enum Status {
        Queued = 0,
        InProgress,
        Completed,
    };

    Status status;
    QByteArray tileData;

    TaskData():
        status(Queued),
        tileData()
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
    diff = t1.x - t2.x;
    if (diff != 0) return diff < 0;
    diff = t1.y - t2.y;
    if (diff != 0) return diff < 0;
    diff = t1.zoom - t2.zoom;
    if (diff != 0) return diff < 0;

    return t1.layer->id() < t2.layer->id();
}

class Downloader: public QRunnable
{
public:
    Downloader(TaskMap &tasks, QMutex &mutex, QObject *listener);
    ~Downloader();

    // reimplemented virtual method
    void run();

private:
    void processTask(TaskMap::iterator t);

private:
    TaskMap &tasks;
    QMutex &mutex;
    QObject *listener;
    QNetworkAccessManager *networkAccessManager;
};

Downloader::Downloader(TaskMap &tasks, QMutex &mutex, QObject *listener):
    tasks(tasks),
    mutex(mutex),
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
        hasTasks = false;
        mutex.lock();
        TaskMap::iterator i = tasks.begin();
        while (i != tasks.constEnd()) {
            if (i.value().status == TaskData::Queued) {
                hasTasks = true;
                /* mark the task as in progress so that other threads won't try
                 * to take it */
                i.value().status = TaskData::InProgress;
                break;
            }
            i++;
        }
        mutex.unlock();

        if (hasTasks)
            processTask(i);
    } while (hasTasks);

    delete networkAccessManager;
    networkAccessManager = 0;

    DEBUG() << "No more tasks, exiting";
}

void Downloader::processTask(TaskMap::iterator t)
{
    const TileTask &tile = t.key();

    DEBUG() << "Processing task: " << tile;

    QUrl url(tile.layer->urlForTile(tile.zoom, tile.x, tile.y));
    QNetworkReply *reply = networkAccessManager->get(QNetworkRequest(url));

    /* QNetworkReply does not implement QIODevice::waitForReadyRead(); let's
     * run an event loop till the request is finished */
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()),
                     &loop, SLOT(quit()));
    loop.exec();

    TaskData &data = t.value();
    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError) {
        DEBUG() << "Got error" << error;
        data.tileData.clear();
    } else {
        data.tileData += reply->readAll();
    }
    delete reply;

    t.value().status = TaskData::Completed;
    QMetaObject::invokeMethod(listener, "taskCompleted", Qt::AutoConnection,
                              Q_ARG(TaskMap::iterator, t));
}

class TileDownloadPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(TileDownload)

    TileDownloadPrivate(TileDownload *tileDownload):
        QObject(tileDownload),
        q_ptr(tileDownload)
    {
        pool = QThreadPool::globalInstance();
        qRegisterMetaType<TaskMap::iterator>("TaskMap::iterator");
    }
    ~TileDownloadPrivate() {};

public Q_SLOTS:
    void taskCompleted(TaskMap::iterator t);

private:
    void requestTile(const TileTask &task);

private:
    mutable TileDownload *q_ptr;
    TaskMap tasks;
    QMutex tasksMutex;
    QThreadPool *pool;
};
}; // namespace

Q_DECLARE_METATYPE(Mappero::TaskMap::iterator)

void TileDownloadPrivate::taskCompleted(TaskMap::iterator t)
{
    Q_Q(TileDownload);

    Q_EMIT q->tileDownloaded(t.key(), t.value().tileData);

    /* Note: we can destroy the TaskData only because we know that the signal
     * connection is immediate; otherwise the receiver might end up accessing
     * deleted data */
    tasksMutex.lock();
    tasks.erase(t);
    tasksMutex.unlock();
}

void TileDownloadPrivate::requestTile(const TileTask &task)
{
    DEBUG() << task;
    tasksMutex.lock();
    tasks.insert(task, TaskData());
    tasksMutex.unlock();

    if (pool->activeThreadCount() < pool->maxThreadCount()) {
        Downloader *downloader = new Downloader(tasks, tasksMutex, this);
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

void TileDownload::requestTile(const TileTask &task)
{
    Q_D(TileDownload);

    d->requestTile(task);
}

#include "tile-download.moc.cpp"
