//
// Created by victor on 19/04/23.
//

#include "dbusnearbysharemanager.h"
#include <QDBusConnection>
#include <nearbyshare/nearbyshareserver.h>

#include "dbushelpers.h"
#include "dbusnearbysharesession.h"

struct DBusNearbyShareManagerPrivate {
    NearbyShareServer* server{};

    QList<QDBusObjectPath> sessions;

    quint64 sessionNum = 0;
};

DBusNearbyShareManager::DBusNearbyShareManager(QObject *parent) : QObject(parent) {
    d = new DBusNearbyShareManagerPrivate();

    d->server = new NearbyShareServer();

    QObject::connect(d->server, &NearbyShareServer::newShare, [this](NearbyShareClient* client) {
        d->sessionNum++;
        auto path = QStringLiteral("%1/sessions/%2").arg(QNearbyShare::DBUS_ROOT_PATH).arg(d->sessionNum);
        auto session = new DBusNearbyShareSession(client, path);
        QDBusConnection::sessionBus().registerObject(path, session, QDBusConnection::ExportScriptableContents);
        d->sessions.append(QDBusObjectPath(path));

        emit NewSession(QDBusObjectPath(path));
    });

    this->setRunning(true);

    connect(this, &DBusNearbyShareManager::isRunningChanged, this, [](bool isRunning) {
        DBusHelpers::emitPropertiesChangedSignal(QNearbyShare::DBUS_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager", "IsRunning", isRunning);
    });
}

DBusNearbyShareManager::~DBusNearbyShareManager() {
    delete d;
}

QString DBusNearbyShareManager::serverName() {
    return d->server->serverName();
}

bool DBusNearbyShareManager::isRunning() {
    return d->server->running();
}

void DBusNearbyShareManager::setRunning(bool running) {
    if (d->server->running() && !running) {
        d->server->stop();
        emit isRunningChanged(running);
    } else if (!d->server->running() && running) {
        d->server->start();
        emit isRunningChanged(running);
    }
}

[[maybe_unused]] QList<QDBusObjectPath> DBusNearbyShareManager::Sessions() {
    return d->sessions;
}
