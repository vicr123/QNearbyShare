

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

#ifndef QNEARBYSHARE_NEARBYSOCKET_H
#define QNEARBYSHARE_NEARBYSOCKET_H

#include "nearbypayload.h"
#include <QObject>
#include <google/protobuf/message_lite.h>

class QIODevice;
struct NearbySocketPrivate;

class NearbySocket : public QObject {
        Q_OBJECT
    public:
        explicit NearbySocket(QIODevice* ioDevice, QObject* parent = nullptr);
        ~NearbySocket();

        bool active();

        void sendPacket(const QByteArray& packet);
        void sendPacket(const google::protobuf::MessageLite& message);

        void sendPayloadPacket(const QByteArray& packet);
        void sendPayloadPacket(const google::protobuf::MessageLite& message);

        void insertPendingPayload(qint64 id, const AbstractNearbyPayloadPtr& payload);

        QByteArray authString();
        QString peerName();

    signals:
        void readyForEncryptedMessages();
        void messageReceived(AbstractNearbyPayloadPtr payload);
        void disconnected();

    private:
        NearbySocketPrivate* d;

        void readBuffer();
        void processOfflineFrame(const QByteArray& frame);
        void processUkey2Frame(const QByteArray& frame);
        void processSecureFrame(const QByteArray& frame);
        void sendKeepalive(bool isAck);
};

#endif // QNEARBYSHARE_NEARBYSOCKET_H
