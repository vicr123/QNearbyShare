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
// Created by victor on 2/05/23.
//

#include "sendjob.h"
#include <QDBusInterface>
#include <sendingfile.h>

struct SendJobPrivate {
        QDBusInterface* manager{};
        QDBusInterface* session{};
};

SendJob::SendJob(QObject* parent) :
    QObject(parent) {
    d = new SendJobPrivate();
    d->manager = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager", QDBusConnection::sessionBus(), this);
}

SendJob::~SendJob() {
    delete d;
}

bool SendJob::send(const QString& connectionString, const QList<QFile*>& files) {
    QList<QNearbyShare::DBus::SendingFile> sendingFiles;
    for (auto file : files) {
        sendingFiles.append({QDBusUnixFileDescriptor(file->handle()),
            file->fileName()});
    }

    auto reply = d->manager->call("SendToTarget", connectionString, QVariant::fromValue(sendingFiles));
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return false;
    }

    auto sessionPath = reply.arguments().first().value<QDBusObjectPath>();
    d->session = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, sessionPath.path(), QNEARBYSHARE_DBUS_SERVICE ".Session", QDBusConnection::sessionBus(), this);

    return false;
}
