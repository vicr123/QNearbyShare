//
// Created by victor on 19/04/23.
//

#ifndef QNEARBYSHARE_DBUSNEARBYSHARESESSION_H
#define QNEARBYSHARE_DBUSNEARBYSHARESESSION_H

#include "constants.h"
#include <QDBusArgument>
#include <QDBusMessage>
#include <QObject>

struct TransferProgress {
    QString fileName;
    QString destination;
    quint64 size;

    quint64 transferred = 0;
    bool complete = false;
};

QDBusArgument& operator<<(QDBusArgument& argument, const TransferProgress& transferProgress);
const QDBusArgument& operator>>(const QDBusArgument& argument, TransferProgress& transferProgress);

class NearbyShareClient;
struct DBusNearbyShareSessionPrivate;
class DBusNearbyShareSession : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", QNEARBYSHARE_DBUS_SERVICE ".Session")
    Q_SCRIPTABLE Q_PROPERTY(QString PeerName READ peerName)
    Q_SCRIPTABLE Q_PROPERTY(QString Pin READ pin)
    Q_SCRIPTABLE Q_PROPERTY(QString State READ state)
public:
    explicit DBusNearbyShareSession(NearbyShareClient* client, const QString& path, QObject* parent = nullptr);
    ~DBusNearbyShareSession();

    QString peerName();
    QString pin();
    QString state();

    Q_SCRIPTABLE [[maybe_unused]] QList<TransferProgress> Transfers();

public slots:
    Q_SCRIPTABLE [[maybe_unused]] void AcceptTransfer(const QDBusMessage &message);
    Q_SCRIPTABLE [[maybe_unused]] void RejectTransfer(const QDBusMessage &message);

    signals:
        Q_SCRIPTABLE void TransfersChanged(QList<TransferProgress> transfers);

private:
    DBusNearbyShareSessionPrivate* d;
};

Q_DECLARE_METATYPE(TransferProgress)
Q_DECLARE_METATYPE(QList<TransferProgress>)

#endif//QNEARBYSHARE_DBUSNEARBYSHARESESSION_H
