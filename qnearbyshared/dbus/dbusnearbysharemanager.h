

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

#ifndef QNEARBYSHARE_DBUSNEARBYSHAREMANAGER_H
#define QNEARBYSHARE_DBUSNEARBYSHAREMANAGER_H

#include "constants.h"
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QObject>

#include <sendingfile.h>

struct DBusNearbyShareManagerPrivate;
class DBusNearbyShareManager : public QObject {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", QNEARBYSHARE_DBUS_SERVICE ".Manager")
        Q_SCRIPTABLE Q_PROPERTY(QString ServerName READ serverName);
        Q_SCRIPTABLE Q_PROPERTY(bool IsRunning READ isRunning NOTIFY isRunningChanged)

            public : explicit DBusNearbyShareManager(QObject* parent = nullptr);
        ~DBusNearbyShareManager();

        QString serverName();

        bool isRunning();
        void setRunning(bool running);

    public slots:
        Q_SCRIPTABLE [[maybe_unused]] QList<QDBusObjectPath> Sessions();
        Q_SCRIPTABLE QDBusObjectPath StartListening(const QDBusMessage& message);
        Q_SCRIPTABLE QDBusObjectPath DiscoverTargets(const QDBusMessage& message);
        Q_SCRIPTABLE QDBusObjectPath SendToTarget(QString connectionString, QList<QNearbyShare::DBus::SendingFile> files, const QDBusMessage& message);

    signals:
        void isRunningChanged(bool running);

        Q_SCRIPTABLE void NewSession(QDBusObjectPath sessionPath);

    private:
        DBusNearbyShareManagerPrivate* d;
};


#endif//QNEARBYSHARE_DBUSNEARBYSHAREMANAGER_H
