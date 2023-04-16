//
// Created by victor on 15/04/23.
//

#ifndef QNEARBYSHARE_NEARBYSOCKET_H
#define QNEARBYSHARE_NEARBYSOCKET_H

#include <QObject>
#include <google/protobuf/message_lite.h>

class QIODevice;
struct NearbySocketPrivate;

class NearbySocket : public QObject {
    Q_OBJECT
public:
    explicit NearbySocket(QIODevice* ioDevice, QObject* parent = nullptr);
    ~NearbySocket();

    void sendPacket(const QByteArray& packet);
    void sendPacket(const google::protobuf::MessageLite& message);

    signals:
        void readyForEncryptedMessages();
        void messageReceived(QByteArray packet);

private:
    NearbySocketPrivate * d;

    void readBuffer();
    void processOfflineFrame(QByteArray frame);
    void processUkey2Frame(QByteArray frame);
    void processSecureFrame(QByteArray frame);

};


#endif//QNEARBYSHARE_NEARBYSOCKET_H
