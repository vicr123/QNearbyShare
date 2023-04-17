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
    explicit NearbyPayload(quint64 id, bool isBytes);
    ~NearbyPayload();

    void loadChunk(quint64 offset, const QByteArray& body);
    void setCompleted();

    quint64 id();
    QByteArray data();
    bool completed();
    bool isBytes();

private:
    NearbyPayloadPrivate* d;
};

typedef QSharedPointer<NearbyPayload> NearbyPayloadPtr;

#endif//QNEARBYSHARE_NEARBYPAYLOAD_H
