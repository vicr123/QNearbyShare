

/*
 * Copyright (c) 2023 Victor Tran
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "dbusnearbysharelistener.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>

struct DBusNearbyShareListenerPrivate {
        QString service;
        QString path;
        QDBusServiceWatcher watcher;
};

DBusNearbyShareListener::DBusNearbyShareListener(QString service, QString path, QObject* parent) :
    QObject(parent) {
    d = new DBusNearbyShareListenerPrivate();
    d->path = path;
    d->service = service;

    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportScriptableContents);
    d->watcher.setConnection(QDBusConnection::sessionBus());
    d->watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    d->watcher.addWatchedService(service);
    connect(&d->watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DBusNearbyShareListener::stopListening);
}

DBusNearbyShareListener::~DBusNearbyShareListener() {
    delete d;
}

void DBusNearbyShareListener::StopListening(const QDBusMessage& message) {
    if (message.service() != d->service) {
        return;
    }
    this->stopListening();
}

void DBusNearbyShareListener::stopListening() {
    QDBusConnection::sessionBus().unregisterObject(d->path);
    emit stoppedListening();
    this->deleteLater();
}
