

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

#ifndef QNEARBYSHARE_DBUSNEARBYSHARESESSION_H
#define QNEARBYSHARE_DBUSNEARBYSHARESESSION_H

#include <QDBusArgument>
#include <QDBusMessage>
#include <QObject>
#include <transferprogress.h>

class NearbyShareClient;
struct DBusNearbyShareSessionPrivate;
class DBusNearbyShareSession : public QObject {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", QNEARBYSHARE_DBUS_SERVICE ".Session")
        Q_SCRIPTABLE Q_PROPERTY(QString PeerName READ peerName)
            Q_SCRIPTABLE Q_PROPERTY(QString Pin READ pin)
                Q_SCRIPTABLE Q_PROPERTY(QString State READ state)
                    Q_SCRIPTABLE Q_PROPERTY(QString FailedReason READ failedReason)
                        Q_SCRIPTABLE Q_PROPERTY(bool IsSending READ isSending)

                            public : explicit DBusNearbyShareSession(NearbyShareClient* client, const QString& path, QObject* parent = nullptr);
        ~DBusNearbyShareSession();

        QString peerName();
        QString pin();
        QString state();
        QString failedReason();
        bool isSending();

        Q_SCRIPTABLE [[maybe_unused]] QList<QNearbyShare::DBus::TransferProgress> Transfers();

    public slots:
        Q_SCRIPTABLE [[maybe_unused]] void AcceptTransfer(const QDBusMessage& message);
        Q_SCRIPTABLE [[maybe_unused]] void RejectTransfer(const QDBusMessage& message);

    signals:
        Q_SCRIPTABLE void TransfersChanged(QList<QNearbyShare::DBus::TransferProgress> transfers);

    private:
        DBusNearbyShareSessionPrivate* d;
};

#endif // QNEARBYSHARE_DBUSNEARBYSHARESESSION_H
