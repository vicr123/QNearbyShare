//
// Created by victor on 19/04/23.
//

#include "dbusnearbysharesession.h"

#include <ranges>
#include <nearbyshare/nearbyshareclient.h>
#include "dbushelpers.h"
#include <QDBusMetaType>

struct DBusNearbyShareSessionPrivate {
    NearbyShareClient* client;

    static QString NearbyShareClientStateToString(NearbyShareClient::State state);
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

QDBusArgument& operator<<(QDBusArgument& argument, const TransferProgress& transferProgress) {
    argument.beginStructure();
    argument << transferProgress.fileName << transferProgress.destination << transferProgress.transferred << transferProgress.size << transferProgress.complete;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, TransferProgress& transferProgress) {
    argument.beginStructure();
    argument >> transferProgress.fileName >> transferProgress.destination >> transferProgress.transferred >> transferProgress.size >> transferProgress.complete;
    argument.endStructure();
    return argument;
}

DBusNearbyShareSession::DBusNearbyShareSession(NearbyShareClient* client, const QString& path, QObject *parent) : QObject(parent) {
    d = new DBusNearbyShareSessionPrivate();
    d->client = client;

    connect(client, &NearbyShareClient::filesToTransferChanged, this, [this] {
        emit TransfersChanged(Transfers());
    });
    connect(client, &NearbyShareClient::stateChanged, this, [this, path](NearbyShareClient::State state) {
        DBusHelpers::emitPropertiesChangedSignal(path, QNEARBYSHARE_DBUS_SERVICE ".Session", "State", DBusNearbyShareSessionPrivate::NearbyShareClientStateToString(state));
    });

    qDBusRegisterMetaType<TransferProgress>();
    qDBusRegisterMetaType<QList<TransferProgress>>();
}

DBusNearbyShareSession::~DBusNearbyShareSession() {
    delete d;
}

QString DBusNearbyShareSession::peerName() {
    return d->client->peerName();
}

[[maybe_unused]] void DBusNearbyShareSession::AcceptTransfer(const QDBusMessage &message) {
    if (d->client->state() != NearbyShareClient::State::WaitingForUserAccept) {
        message.createErrorReply(QNearbyShare::DBUS_ERROR_INVALID_STATE, "Can't accept a transfer that isn't pending user acceptance");
        return;
    }
    d->client->acceptTransfer();
}

[[maybe_unused]] void DBusNearbyShareSession::RejectTransfer(const QDBusMessage &message) {
    if (d->client->state() != NearbyShareClient::State::WaitingForUserAccept) {
        message.createErrorReply(QNearbyShare::DBUS_ERROR_INVALID_STATE, "Can't accept a transfer that isn't pending user acceptance");
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

[[maybe_unused]] QList<TransferProgress> DBusNearbyShareSession::Transfers() {
    QList<TransferProgress> progress;
    for (const auto& item : d->client->filesToTransfer()) {
        TransferProgress prg;
        prg.fileName = item.fileName;
        prg.destination = item.destination;
        prg.transferred = item.transferred;
        prg.size = item.size;
        prg.complete = item.complete;
        progress.append(prg);
    }
    return progress;
}
