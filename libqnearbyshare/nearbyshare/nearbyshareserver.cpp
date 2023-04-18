//
// Created by victor on 15/04/23.
//

#include "nearbyshareserver.h"
#include <qzeroconf.h>
#include <QTcpServer>
#include <QRandomGenerator>
#include <QHostInfo>
#include <QTextStream>
#include <QTcpSocket>

#include "endpointinfo.h"

// Protocol documentation: https://github.com/grishka/NearDrop/blob/master/PROTOCOL.md

#include "nearbyshareclient.h"

struct NearbyShareServerPrivate {
    bool running = false;

    QTcpServer* tcp;
    QZeroConf zeroconf;
    QByteArray serviceName;
};

NearbyShareServer::NearbyShareServer() : QObject(nullptr) {
    d = new NearbyShareServerPrivate();

    // TODO: Generate endpointId randomly?
    QString endpointId = "AHGE";
    d->serviceName.append("\x23");
    d->serviceName.append(endpointId.toUtf8());
    d->serviceName.append("\xFC\x9F\x5E");
    d->serviceName.append("\x00\x00");

    EndpointInfo info;
    info.deviceName = QHostInfo::localHostName();

    d->zeroconf.addServiceTxtRecord("n", info.toByteArray().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

NearbyShareServer::~NearbyShareServer() {
    this->stop();
    delete d;
}

void NearbyShareServer::start() {
    if (d->running) return;

    d->tcp = new QTcpServer(this);
    connect(d->tcp, &QTcpServer::pendingConnectionAvailable, this, &NearbyShareServer::acceptPendingConnection);
    d->tcp->listen();

    d->zeroconf.startServicePublish(d->serviceName.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals).data(), "_FC9F5ED42C8A._tcp.", "", d->tcp->serverPort());
    d->running = true;
}

void NearbyShareServer::stop() {
    if (!d->running) return;

    d->zeroconf.stopServicePublish();

    d->tcp->close();
    d->tcp->deleteLater();
}

void NearbyShareServer::acceptPendingConnection() {
    auto socket = d->tcp->nextPendingConnection();
    QTextStream(stdout) << "Pending connection accepted\n";

    auto ns = new NearbyShareClient(socket, true);
    connect(ns, &NearbyShareClient::negotiationCompleted, this, [this, ns] {
        emit newShare(ns);
    });
}
