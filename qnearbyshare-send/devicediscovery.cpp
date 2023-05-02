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

#include "devicediscovery.h"
#include <QDBusArgument>
#include <QDBusInterface>
#include <QThread>
#include <nearbysharetarget.h>

struct DeviceDiscoveryPrivate {
        QDBusInterface* manager{};
        QDBusInterface* targetDiscovery{};
        Console* console;

        bool valid = true;
};

DeviceDiscovery::DeviceDiscovery(Console* console, QObject* parent) :
    QObject(parent) {
    d = new DeviceDiscoveryPrivate();
    d->console = console;
    d->manager = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager", QDBusConnection::sessionBus(), this);

    auto reply = d->manager->call("DiscoverTargets");
    if (reply.type() != QDBusMessage::ReplyMessage) {
        d->valid = false;
    }

    auto targetDiscoveryPath = reply.arguments().first().value<QDBusObjectPath>();
    d->targetDiscovery = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, targetDiscoveryPath.path(), QNEARBYSHARE_DBUS_SERVICE ".TargetDiscovery", QDBusConnection::sessionBus(), this);
}

DeviceDiscovery::~DeviceDiscovery() {
    d->targetDiscovery->call("StopDiscovery");
    delete d;
}

QString DeviceDiscovery::exec() {
    if (!d->valid) {
        d->console->outputErrorLine("Could not scan for devices. Is the DBus service running?");
        return {};
    }

    d->console->outputErrorLine(":: Scanning for devices...");
    QThread::sleep(5);

    QString connectionString;

reloadTargets:
    auto targets = printAvailableTargets();
    if (targets.isEmpty()) {
        d->console->outputErrorLine("  No targets found.");
    }

    while (connectionString.isEmpty()) {
        QString response;
        if (targets.isEmpty()) {
            response = d->console->question("What now? (r to scan again, q to quit)");
        } else {
            response = d->console->question("Send to (r to scan again)");
        }
        if (response == "r") {
            goto reloadTargets;
        } else if (response == "q") {
            return "";
        } else {
            bool ok;
            auto index = response.toInt(&ok) - 1;
            if (!ok) continue;
            if (index < 0) continue;
            if (index >= targets.length()) continue;
            connectionString = targets.at(index);
        }
    }

    return connectionString;
}

QStringList DeviceDiscovery::printAvailableTargets() {
    auto targetsReply = d->targetDiscovery->call("DiscoveredTargets");
    if (targetsReply.type() != QDBusMessage::ReplyMessage) {
        return {};
    }

    auto targetsArg = targetsReply.arguments().first().value<QDBusArgument>();
    QList<QNearbyShare::DBus::NearbyShareTarget> targets;
    targetsArg >> targets;

    QStringList connectionStrings;
    QList<QStringList> table;
    for (int i = 1; i <= targets.length(); i++) {
        auto target = targets.at(i - 1);
        connectionStrings.append(target.connectionString);
        table.append({QString::number(i),
            target.connectionString,
            target.name});
    }
    d->console->outputTable(table);

    return connectionStrings;
}
