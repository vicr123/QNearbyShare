

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

#include "dbusnearbysharesession.h"

#include "dbuserrors.h"
#include "dbushelpers.h"
#include <QDBusConnection>
#include <nearbyshare/nearbyshareclient.h>
#include <ranges>

struct DBusNearbyShareSessionPrivate {
        NearbyShareClient* client;

        static QString NearbyShareClientStateToString(NearbyShareClient::State state);
        static QString NearbyShareClientFailedReasonToString(NearbyShareClient::FailedReason reason);
};

QString DBusNearbyShareSessionPrivate::NearbyShareClientStateToString(NearbyShareClient::State state) {
    switch (state) {
        case NearbyShareClient::State::NotReady:
            return QStringLiteral("NotReady");
        case NearbyShareClient::State::WaitingForUserAccept:
            return QStringLiteral("WaitingForUserAccept");
        case NearbyShareClient::State::Transferring:
            return QStringLiteral("Transferring");
        case NearbyShareClient::State::Complete:
            return QStringLiteral("Complete");
        case NearbyShareClient::State::Failed:
            return QStringLiteral("Failed");
    }
    return QStringLiteral("Unknown");
}

QString DBusNearbyShareSessionPrivate::NearbyShareClientFailedReasonToString(NearbyShareClient::FailedReason reason) {
    switch (reason) {
        case NearbyShareClient::FailedReason::Unknown:
            return QStringLiteral("Unknown");
        case NearbyShareClient::FailedReason::RemoteDeclined:
            return QStringLiteral("RemoteDeclined");
        case NearbyShareClient::FailedReason::RemoteOutOfSpace:
            return QStringLiteral("RemoteOutOfSpace");
        case NearbyShareClient::FailedReason::RemoteUnsupported:
            return QStringLiteral("RemoteUnsupported");
        case NearbyShareClient::FailedReason::RemoteTimedOut:
            return QStringLiteral("RemoteTimedOut");
    }
    return QStringLiteral("Unknown");
}

DBusNearbyShareSession::DBusNearbyShareSession(NearbyShareClient* client, const QString& path, QObject* parent) :
    QObject(parent) {
    d = new DBusNearbyShareSessionPrivate();
    d->client = client;

    connect(client, &NearbyShareClient::filesToTransferChanged, this, [this] {
        emit TransfersChanged(Transfers());
    });
    connect(client, &NearbyShareClient::stateChanged, this, [this, path](NearbyShareClient::State state) {
        DBusHelpers::emitPropertiesChangedSignal(path, QNEARBYSHARE_DBUS_SERVICE ".Session", "State", DBusNearbyShareSessionPrivate::NearbyShareClientStateToString(state));
    });
}

DBusNearbyShareSession::~DBusNearbyShareSession() {
    delete d;
}

QString DBusNearbyShareSession::peerName() {
    return d->client->peerName();
}

[[maybe_unused]] void DBusNearbyShareSession::AcceptTransfer(const QDBusMessage& message) {
    if (d->client->isSending()) {
        QDBusConnection::sessionBus().send(message.createErrorReply(QNearbyShare::DBus::Error::INVALID_DIRECTION, "Can't accept an outbound transfer"));
        return;
    }
    if (d->client->state() != NearbyShareClient::State::WaitingForUserAccept) {
        QDBusConnection::sessionBus().send(message.createErrorReply(QNearbyShare::DBus::Error::INVALID_STATE, "Can't accept a transfer that isn't pending user acceptance"));
        return;
    }
    d->client->acceptTransfer();
}

[[maybe_unused]] void DBusNearbyShareSession::RejectTransfer(const QDBusMessage& message) {
    if (d->client->isSending()) {
        QDBusConnection::sessionBus().send(message.createErrorReply(QNearbyShare::DBus::Error::INVALID_DIRECTION, "Can't reject an outbound transfer"));
        return;
    }
    if (d->client->state() != NearbyShareClient::State::WaitingForUserAccept) {
        QDBusConnection::sessionBus().send(message.createErrorReply(QNearbyShare::DBus::Error::INVALID_STATE, "Can't reject a transfer that isn't pending user acceptance"));
        return;
    }
    d->client->rejectTransfer();
}

QString DBusNearbyShareSession::pin() {
    return d->client->pin();
}

QString DBusNearbyShareSession::state() {
    return DBusNearbyShareSessionPrivate::NearbyShareClientStateToString(d->client->state());
}

[[maybe_unused]] QList<QNearbyShare::DBus::TransferProgress> DBusNearbyShareSession::Transfers() {
    QList<QNearbyShare::DBus::TransferProgress> progress;
    for (const auto& item : d->client->filesToTransfer()) {
        QNearbyShare::DBus::TransferProgress prg;
        prg.fileName = item.fileName;
        prg.destination = item.destination;
        prg.transferred = item.transferred;
        prg.size = item.size;
        prg.complete = item.complete;
        progress.append(prg);
    }
    return progress;
}

QString DBusNearbyShareSession::failedReason() {
    return DBusNearbyShareSessionPrivate::NearbyShareClientFailedReasonToString(d->client->failedReason());
}

bool DBusNearbyShareSession::isSending() {
    return d->client->isSending();
}
