//
// Created by victor on 19/04/23.
//

#ifndef QNEARBYSHARE_DBUSNEARBYSHARELISTENER_H
#define QNEARBYSHARE_DBUSNEARBYSHARELISTENER_H

#include "constants.h"
#include <QDBusMessage>
#include <QObject>

struct DBusNearbyShareListenerPrivate;
class DBusNearbyShareListener : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", QNEARBYSHARE_DBUS_SERVICE ".Listener")
public:
    explicit DBusNearbyShareListener(QString service, QString path, QObject* parent);
    ~DBusNearbyShareListener();

public slots:
    Q_SCRIPTABLE void StopListening(const QDBusMessage& message);

    signals:
        void stoppedListening();

private:
    DBusNearbyShareListenerPrivate* d;

    void stopListening();
};


#endif//QNEARBYSHARE_DBUSNEARBYSHARELISTENER_H
