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

//
// Created by victor on 1/05/23.
//

#include "dbusnearbysharediscovery.h"
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>
#include <nearbyshare/nearbysharediscovery.h>

struct DBusNearbyShareDiscoveryPrivate {
        QString service;
        QString path;
        QDBusServiceWatcher watcher;

        NearbyShareDiscovery* discovery;
};

QDBusArgument& operator<<(QDBusArgument& argument, const DBusNearbyShareDiscovery::NearbyShareTarget& target) {
    argument.beginStructure();
    argument << target.connectionString << target.name << target.deviceType;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusNearbyShareDiscovery::NearbyShareTarget& target) {
    argument.beginStructure();
    argument >> target.connectionString >> target.name >> target.deviceType;
    argument.endStructure();
    return argument;
}

DBusNearbyShareDiscovery::DBusNearbyShareDiscovery(QString service, QString path, QObject* parent) :
    QObject(parent) {
    qDBusRegisterMetaType<DBusNearbyShareDiscovery::NearbyShareTarget>();
    qDBusRegisterMetaType<QList<DBusNearbyShareDiscovery::NearbyShareTarget>>();

    d = new DBusNearbyShareDiscoveryPrivate();
    d->path = path;
    d->service = service;

    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportScriptableContents);
    d->watcher.setConnection(QDBusConnection::sessionBus());
    d->watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    d->watcher.addWatchedService(service);
    connect(&d->watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DBusNearbyShareDiscovery::stopDiscovery);

    d->discovery = new NearbyShareDiscovery(this);
    connect(d->discovery, &NearbyShareDiscovery::newTarget, this, [this](NearbyShareDiscovery::NearbyShareTarget t) {
        NearbyShareTarget target;
        target.connectionString = t.connectionString;
        target.name = t.name;

        emit DiscoveredNewTarget(target);
    });
    connect(d->discovery, &NearbyShareDiscovery::targetGone, this, &DBusNearbyShareDiscovery::DiscoveredTargetGone);
}

DBusNearbyShareDiscovery::~DBusNearbyShareDiscovery() {
    delete d;
}

void DBusNearbyShareDiscovery::stopDiscovery() {
    QDBusConnection::sessionBus().unregisterObject(d->path);
    emit stoppedDiscovery();
    this->deleteLater();
}

QList<DBusNearbyShareDiscovery::NearbyShareTarget> DBusNearbyShareDiscovery::DiscoveredTargets(const QDBusMessage& message) {
    QList<NearbyShareTarget> targets;
    for (const auto& t : d->discovery->availableTargets()) {
        NearbyShareTarget target;
        target.connectionString = t.connectionString;
        target.name = t.name;
        targets.append(target);
    }
    return targets;
}

void DBusNearbyShareDiscovery::StopDiscovery(const QDBusMessage& message) {
    if (message.service() != d->service) {
        return;
    }
    this->stopDiscovery();
}
