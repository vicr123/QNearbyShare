//
// Created by victor on 19/04/23.
//

#ifndef QNEARBYSHARE_DBUSNEARBYSHAREMANAGER_H
#define QNEARBYSHARE_DBUSNEARBYSHAREMANAGER_H

#include "constants.h"
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QObject>

struct DBusNearbyShareManagerPrivate;
class DBusNearbyShareManager : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", QNEARBYSHARE_DBUS_SERVICE ".Manager")
    Q_SCRIPTABLE Q_PROPERTY(QString ServerName READ serverName);
    Q_SCRIPTABLE Q_PROPERTY(bool IsRunning READ isRunning NOTIFY isRunningChanged)

public:
    explicit DBusNearbyShareManager(QObject* parent = nullptr);
    ~DBusNearbyShareManager();

    QString serverName();

    bool isRunning();
    void setRunning(bool running);

public slots:
    Q_SCRIPTABLE [[maybe_unused]] QList<QDBusObjectPath> Sessions();
    Q_SCRIPTABLE QDBusObjectPath StartListening(const QDBusMessage& message);

    signals:
        void isRunningChanged(bool running);

        Q_SCRIPTABLE void NewSession(QDBusObjectPath sessionPath);

private:
    DBusNearbyShareManagerPrivate* d;
};


#endif//QNEARBYSHARE_DBUSNEARBYSHAREMANAGER_H
