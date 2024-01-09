

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

#ifndef QNEARBYSHARE_NEARBYSHARECLIENT_H
#define QNEARBYSHARE_NEARBYSHARECLIENT_H

#include "abstractnearbypayload.h"
#include <QObject>

struct NearbyShareClientPrivate;
class NearbyShareClient : public QObject {
        Q_OBJECT
    public:
        ~NearbyShareClient();

        struct LocalFile {
                QIODevice* device;
                QString fileName;
                quint64 size;
        };

        static NearbyShareClient* clientForReceive(QIODevice* device);
        static NearbyShareClient* clientForSend(QIODevice* device, QString peerName, QList<LocalFile> files);
        static QIODevice* resolveConnectionString(const QString& connectionString);

        enum class State {
            NotReady,
            WaitingForUserAccept,
            Transferring,
            Complete,
            Failed
        };

        enum class FailedReason {
            Unknown,
            RemoteDeclined,
            RemoteOutOfSpace,
            RemoteUnsupported,
            RemoteTimedOut
        };

        struct TransferredFile {
                qint64 id;
                QString fileName;
                QString destination;
                quint64 size;

                quint64 transferred = 0;
                bool complete = 0;
        };

        static QString pinCodeFromAuthString(const QByteArray& authString);

        bool isSending();

        State state();
        FailedReason failedReason();
        QList<TransferredFile> filesToTransfer();
        QString peerName();
        QString pin();

        void acceptTransfer();
        void rejectTransfer();

    signals:
        void stateChanged(State state);
        void negotiationCompleted();
        void filesToTransferChanged();

    private:
        explicit NearbyShareClient(QObject* parent = nullptr);
        NearbyShareClientPrivate* d;

        void readyForEncryptedMessages();
        void messageReceived(const AbstractNearbyPayloadPtr& payload);
        void setState(State state, bool dontDisconnect = false);

        void sendPairedKeyEncryptionResponse();
        void checkIfComplete();

        void writeNextSendPackets();
};

#endif // QNEARBYSHARE_NEARBYSHARECLIENT_H
