//
// Created by victor on 17/04/23.
//

#ifndef QNEARBYSHARE_NEARBYPAYLOAD_H
#define QNEARBYSHARE_NEARBYPAYLOAD_H

#include "abstractnearbypayload.h"
#include <QByteArray>
#include <QSharedPointer>

struct NearbyPayloadPrivate;
class NearbyPayload : public AbstractNearbyPayload {
    Q_OBJECT
public:
    explicit NearbyPayload(qint64 id, bool isBytes);
    ~NearbyPayload();

    QByteArray data();

private:
    NearbyPayloadPrivate* d;
};

#endif//QNEARBYSHARE_NEARBYPAYLOAD_H
