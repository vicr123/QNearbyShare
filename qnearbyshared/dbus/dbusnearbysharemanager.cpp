

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

#include "dbusnearbysharemanager.h"
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QFile>
#include <nearbyshare/nearbyshareserver.h>
#include <utility>

#include "dbushelpers.h"
#include "dbusnearbysharediscovery.h"
#include "dbusnearbysharelistener.h"
#include "dbusnearbysharesession.h"

struct DBusNearbyShareManagerPrivate {
        NearbyShareServer* server{};

        QList<QDBusObjectPath> sessions;

        quint64 currentlyListening = 0;

        quint64 sessionNum = 0;
        quint64 listenerNum = 0;
        quint64 targetDiscoveryNum = 0;
};

DBusNearbyShareManager::DBusNearbyShareManager(QObject* parent) :
    QObject(parent) {
    d = new DBusNearbyShareManagerPrivate();

    d->server = new NearbyShareServer();

    QObject::connect(d->server, &NearbyShareServer::newShare, [this](NearbyShareClient* client) {
        registerNewShare(client);
    });

    //    this->setRunning(true);

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

QDBusObjectPath DBusNearbyShareManager::StartListening(const QDBusMessage& message) {
    auto path = QStringLiteral("%1/listeners/%2").arg(QNearbyShare::DBUS_ROOT_PATH).arg(d->listenerNum);
    d->listenerNum++;
    d->currentlyListening++;

    auto listener = new DBusNearbyShareListener(message.service(), path, this);
    connect(listener, &DBusNearbyShareListener::stoppedListening, this, [this] {
        d->currentlyListening--;
        if (d->currentlyListening == 0) {
            this->setRunning(false);
        }
    });

    this->setRunning(true);

    return QDBusObjectPath(path);
}

QDBusObjectPath DBusNearbyShareManager::DiscoverTargets(const QDBusMessage& message) {
    auto path = QStringLiteral("%1/targetDiscovery/%2").arg(QNearbyShare::DBUS_ROOT_PATH).arg(d->targetDiscoveryNum);
    d->targetDiscoveryNum++;

    new DBusNearbyShareDiscovery(message.service(), path, this);
    return QDBusObjectPath(path);
}

QDBusObjectPath DBusNearbyShareManager::SendToTarget(const QString& connectionString, QString peerName, const QList<QNearbyShare::DBus::SendingFile>& files, const QDBusMessage& message) {
    QIODevice* device = NearbyShareClient::resolveConnectionString(connectionString);

    if (!device) {
        // Error
        message.createErrorReply(QNearbyShare::DBUS_ERROR_INVALID_CONNECTION_STRING, "The connection string is invalid");
        return {};
    }

    QList<NearbyShareClient::LocalFile> filesToTransfer;
    for (const auto& file : files) {
        auto qf = new QFile();
        qf->open(dup(file.fd.fileDescriptor()), QFile::ReadOnly, QFile::AutoCloseHandle);

        filesToTransfer.append({qf,
            file.filename,
            static_cast<quint64>(qf->size())});
    }

    auto client = NearbyShareClient::clientForSend(device, std::move(peerName), filesToTransfer);
    return registerNewShare(client);
}

QDBusObjectPath DBusNearbyShareManager::registerNewShare(NearbyShareClient* client) {
    d->sessionNum++;
    auto path = QStringLiteral("%1/sessions/%2").arg(QNearbyShare::DBUS_ROOT_PATH).arg(d->sessionNum);
    auto session = new DBusNearbyShareSession(client, path);
    QDBusConnection::sessionBus().registerObject(path, session, QDBusConnection::ExportScriptableContents);
    d->sessions.append(QDBusObjectPath(path));

    emit NewSession(QDBusObjectPath(path));

    return QDBusObjectPath(path);
}
