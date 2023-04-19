//
// Created by victor on 19/04/23.
//

#include "dbusnearbysharelistener.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>

struct DBusNearbyShareListenerPrivate {
    QString service;
    QString path;
    QDBusServiceWatcher watcher;
};

DBusNearbyShareListener::DBusNearbyShareListener(QString service, QString path, QObject *parent) : QObject(parent) {
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

void DBusNearbyShareListener::StopListening(const QDBusMessage &message) {
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

