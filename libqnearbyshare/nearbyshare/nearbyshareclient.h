//
// Created by victor on 16/04/23.
//

#ifndef QNEARBYSHARE_NEARBYSHARECLIENT_H
#define QNEARBYSHARE_NEARBYSHARECLIENT_H

#include "abstractnearbypayload.h"
#include <QObject>

struct NearbyShareClientPrivate;
class NearbyShareClient : public QObject {
    Q_OBJECT
public:
    explicit NearbyShareClient(QIODevice* ioDevice, bool receive, QObject* parent = nullptr);
    ~NearbyShareClient();

    struct TransferredFile {
        qint64 id;
        QString fileName;
        QString destination;
        quint64 size;

        quint64 transferred = 0;
        bool complete = 0;
    };

    static QString pinCodeFromAuthString(const QByteArray& authString);

    QList<TransferredFile> filesToTransfer();

    void acceptTransfer();
    void rejectTransfer();

    signals:
        void negotiationCompleted();

private:
    NearbyShareClientPrivate* d;

    void readyForEncryptedMessages();
    void messageReceived(const AbstractNearbyPayloadPtr & payload);

    void sendPairedKeyEncryptionResponse();
};


#endif//QNEARBYSHARE_NEARBYSHARECLIENT_H
