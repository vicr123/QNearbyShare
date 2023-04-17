//
// Created by victor on 16/04/23.
//

#ifndef QNEARBYSHARE_NEARBYSHARECLIENT_H
#define QNEARBYSHARE_NEARBYSHARECLIENT_H

#include <QObject>

struct NearbyShareClientPrivate;
class NearbyShareClient : public QObject {
    Q_OBJECT
public:
    explicit NearbyShareClient(QIODevice* ioDevice, bool receive, QObject* parent = nullptr);
    ~NearbyShareClient();

private:
    NearbyShareClientPrivate* d;

    void readyForEncryptedMessages();
};


#endif//QNEARBYSHARE_NEARBYSHARECLIENT_H
