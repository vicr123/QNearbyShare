

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

#include "nearbyshareserver.h"
#include "nearbyshareclient.h"
#include "nearbyshareconstants.h"
#include "qzeroconf.h"
#include <QHostInfo>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>

#include "endpointinfo.h"

// Protocol documentation: https://github.com/grishka/NearDrop/blob/master/PROTOCOL.md


struct NearbyShareServerPrivate {
        bool running = false;

        QTcpServer* tcp{};
        QZeroConf zeroconf;
        QByteArray serviceName;
};

NearbyShareServer::NearbyShareServer() :
    QObject(nullptr) {
    d = new NearbyShareServerPrivate();

    // TODO: Generate endpointId randomly?
    QString endpoints = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    QString endpointId;

    for (auto i = 0; i < 4; i++) {
        endpointId.append(endpoints.at(QRandomGenerator::system()->bounded(endpoints.length())));
    }

    d->serviceName.append("\x23"); // NOLINT(modernize-raw-string-literal)
    d->serviceName.append(endpointId.toUtf8());
    d->serviceName.append("\xFC\x9F\x5E");
    d->serviceName.append("\x00\x00", 2);

    d->zeroconf.addServiceTxtRecord("n", EndpointInfo::system().toByteArray().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

NearbyShareServer::~NearbyShareServer() {
    this->stop();
    delete d;
}

bool NearbyShareServer::start() {
    if (d->running) return true;

    d->tcp = new QTcpServer(this);
#if QT_VERSION > QT_VERSION_CHECK(6, 4, 0)
    connect(d->tcp, &QTcpServer::pendingConnectionAvailable, this, &NearbyShareServer::acceptPendingConnection);
#else
    connect(d->tcp, &QTcpServer::newConnection, this, &NearbyShareServer::acceptPendingConnection);
#endif
    d->tcp->listen();

    d->zeroconf.startServicePublish(d->serviceName.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals).data(), QNearbyShare::ZEROCONF_TYPE, "", d->tcp->serverPort());
    if (!d->zeroconf.publishExists()) {
        d->tcp->close();
        d->tcp->deleteLater();
        return false;
    }
    d->running = true;
    return true;
}

void NearbyShareServer::stop() {
    if (!d->running) return;

    d->zeroconf.stopServicePublish();

    d->tcp->close();
    d->tcp->deleteLater();

    d->running = false;
}

void NearbyShareServer::acceptPendingConnection() {
    auto socket = d->tcp->nextPendingConnection();
    QTextStream(stdout) << "Pending connection accepted\n";

    auto ns = NearbyShareClient::clientForReceive(socket);
    connect(ns, &NearbyShareClient::negotiationCompleted, this, [this, ns] {
        emit newShare(ns);
    });
}

QString NearbyShareServer::serverName() { // NOLINT(readability-convert-member-functions-to-static)
    return QHostInfo::localHostName();
}

bool NearbyShareServer::running() {
    return d->running;
}
