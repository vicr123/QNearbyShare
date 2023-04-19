//
// Created by victor on 15/04/23.
//

#ifndef QNEARBYSHARE_NEARBYSHARESERVER_H
#define QNEARBYSHARE_NEARBYSHARESERVER_H

#include <QObject>
#include "nearbyshareclient.h"

struct NearbyShareServerPrivate;
class NearbyShareServer : public QObject {
    Q_OBJECT
public:
    NearbyShareServer();
    ~NearbyShareServer();

    QString serverName();

    void start();
    void stop();
    bool running();

    signals:
        void newShare(NearbyShareClient* client);

private:
    NearbyShareServerPrivate* d;

    void acceptPendingConnection();
};


#endif//QNEARBYSHARE_NEARBYSHARESERVER_H
