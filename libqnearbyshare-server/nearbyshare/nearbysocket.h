

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
        explicit NearbySocket(QIODevice* ioDevice, bool isServer, QObject* parent = nullptr);
        ~NearbySocket();

        bool active();

        void sendPacket(const QByteArray& packet);
        void sendPacket(const google::protobuf::MessageLite& message);

        enum PayloadType {
            Bytes,
            File
        };

        void sendPayloadPacket(const QByteArray& packet);
        void sendPayloadPacket(const google::protobuf::MessageLite& message);
        void sendPayloadPacket(const QByteArray& packet, qint64 id, PayloadType payloadType = Bytes, qint64 offset = 0, bool lastChunk = true);
        void sendPayloadPacket(const google::protobuf::MessageLite& message, qint64 id);

        void insertPendingPayload(qint64 id, const AbstractNearbyPayloadPtr& payload);

        QByteArray authString();

        void setPeerName(QString peerName);
        QString peerName();

        void disconnect();

    signals:
        void readyForEncryptedMessages();
        void messageReceived(AbstractNearbyPayloadPtr payload);
        void errorOccurred();
        void disconnected();
        void readyForNextPacket();

    private:
        NearbySocketPrivate* d;

        void readBuffer();
        void processOfflineFrame(const QByteArray& frame);
        void processUkey2Frame(const QByteArray& frame);
        void processSecureFrame(const QByteArray& frame);
        void sendKeepalive(bool isAck);

        void sendConnectionRequest();
        void setupDiffieHellman(const QByteArray& x, const QByteArray& y);
        void sendConnectionResponse();
        void writeNextPacket();
};

#endif // QNEARBYSHARE_NEARBYSOCKET_H
