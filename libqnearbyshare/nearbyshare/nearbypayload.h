//
// Created by victor on 17/04/23.
//

#ifndef QNEARBYSHARE_NEARBYPAYLOAD_H
#define QNEARBYSHARE_NEARBYPAYLOAD_H

#include <QByteArray>
#include <QSharedPointer>

struct NearbyPayloadPrivate;
class NearbyPayload {
public:
    explicit NearbyPayload(bool isBytes);
    ~NearbyPayload();

    void loadChunk(quint64 offset, const QByteArray& body);
    void setCompleted();

    QByteArray data();
    bool completed();
    bool isBytes();

private:
    NearbyPayloadPrivate* d;
};

typedef QSharedPointer<NearbyPayload> NearbyPayloadPtr;

#endif//QNEARBYSHARE_NEARBYPAYLOAD_H
