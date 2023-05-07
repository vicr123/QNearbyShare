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

#ifndef QNEARBYSHARE_DBUSNEARBYSHAREDISCOVERY_H
#define QNEARBYSHARE_DBUSNEARBYSHAREDISCOVERY_H

#include "dbusnearbysharemanager.h"
#include <QObject>

#include <nearbysharetarget.h>

class NearbyShareDiscovery;
struct DBusNearbyShareDiscoveryPrivate;
class DBusNearbyShareDiscovery : public QObject {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", QNEARBYSHARE_DBUS_SERVICE ".TargetDiscovery")
    public:
        ~DBusNearbyShareDiscovery() override;

        static DBusNearbyShareDiscovery* construct(const QString& service, const QString& path, QObject* parent);

    public slots:
        Q_SCRIPTABLE QList<QNearbyShare::DBus::NearbyShareTarget> DiscoveredTargets(const QDBusMessage& message);
        Q_SCRIPTABLE void StopDiscovery(const QDBusMessage& message);

    signals:
        Q_SCRIPTABLE void DiscoveredNewTarget(QNearbyShare::DBus::NearbyShareTarget target);
        Q_SCRIPTABLE void DiscoveredTargetGone(QString connectionString);
        void stoppedDiscovery();

    private:
        explicit DBusNearbyShareDiscovery(NearbyShareDiscovery* discovery, const QString& service, const QString& path, QObject* parent);
        DBusNearbyShareDiscoveryPrivate* d;

        void stopDiscovery();
};


#endif // QNEARBYSHARE_DBUSNEARBYSHAREDISCOVERY_H
