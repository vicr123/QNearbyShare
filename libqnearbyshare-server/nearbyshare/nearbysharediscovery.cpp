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

//
// Created by victor on 1/05/23.
//

#include "nearbysharediscovery.h"
#include "endpointinfo.h"
#include "nearbyshareconstants.h"
#include "qzeroconf.h"
#include <QNetworkInterface>

struct NearbyShareDiscoveryPrivate {
        QZeroConf zeroconf;
        QMap<QZeroConfService, NearbyShareDiscovery::NearbyShareTarget> targets;
};

NearbyShareDiscovery::NearbyShareDiscovery(QObject* parent) :
    QObject(parent) {
    d = new NearbyShareDiscoveryPrivate();

    connect(&d->zeroconf, &QZeroConf::serviceAdded, this, [this](const QZeroConfService& service) {
        bool ok;
        auto endpointInfo = EndpointInfo::fromByteArray(QByteArray::fromBase64(service->txt().value("n"), QByteArray::Base64UrlEncoding), &ok);
        if (!ok) return;

        if (endpointInfo.version > 1) return;

        // Filter out ourselves :)
        if (QNetworkInterface::allAddresses().contains(service->ip())) return;

        // Also filter out duplicates
        auto connectionString = QStringLiteral("%1:%2:%3").arg("tcp", service->ip().toString(), QString::number(service->port()));
        for (const auto& target : d->targets) {
            if (target.connectionString == connectionString) return;
        }

        NearbyShareTarget target;
        target.connectionString = connectionString;
        target.name = endpointInfo.deviceName;
        target.deviceType = endpointInfo.deviceType;

        d->targets.insert(service, target);

        emit newTarget(target);
    });
    connect(&d->zeroconf, &QZeroConf::serviceRemoved, this, [this](const QZeroConfService& service) {
        if (!d->targets.contains(service)) return;
        auto connectionString = QStringLiteral("%1:%2:%3").arg("tcp", service->ip().toString(), QString::number(service->port()));

        d->targets.remove(service);
        emit targetGone(connectionString);
    });
}

NearbyShareDiscovery::~NearbyShareDiscovery() {
    d->zeroconf.stopBrowser();
    delete d;
}

QList<NearbyShareDiscovery::NearbyShareTarget> NearbyShareDiscovery::availableTargets() {
    return d->targets.values();
}

bool NearbyShareDiscovery::start() {
    d->zeroconf.startBrowser(QNearbyShare::ZEROCONF_TYPE);
    return d->zeroconf.browserExists();
}
