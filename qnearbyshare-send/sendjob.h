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

#ifndef QNEARBYSHARE_SENDJOB_H
#define QNEARBYSHARE_SENDJOB_H

#include <QFile>
#include <QObject>
#include <transferprogress.h>

struct SendJobPrivate;
class SendJob : public QObject {
        Q_OBJECT
    public:
        explicit SendJob(QObject* parent = nullptr);
        ~SendJob();

        bool send(const QString& connectionString, const QString& peerName, const QList<QFile*>& files);

    private slots:
        void sessionPropertiesChanged(QString interface, QVariantMap properties, QStringList changedProperties);
        void transfersChanged(QList<QNearbyShare::DBus::TransferProgress> transfers);

    private:
        SendJobPrivate* d;
        QList<QNearbyShare::DBus::TransferProgress> transfers();
};

#endif // QNEARBYSHARE_SENDJOB_H
