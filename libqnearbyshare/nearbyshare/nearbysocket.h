//
// Created by victor on 15/04/23.
//

#ifndef QNEARBYSHARE_NEARBYSOCKET_H
#define QNEARBYSHARE_NEARBYSOCKET_H

#include <QObject>
#include "nearbypayload.h"
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
    NearbySocketPrivate * d;

    void readBuffer();
    void processOfflineFrame(const QByteArray& frame);
    void processUkey2Frame(const QByteArray& frame);
    void processSecureFrame(const QByteArray& frame);
    void sendKeepalive(bool isAck);

};


#endif//QNEARBYSHARE_NEARBYSOCKET_H
