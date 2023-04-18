//
// Created by victor on 17/04/23.
//

#include <QBuffer>
#include <QTextStream>
#include "nearbypayload.h"

struct NearbyPayloadPrivate {
    QBuffer* buffer;
};

NearbyPayload::NearbyPayload(qint64 id, bool isBytes) : AbstractNearbyPayload(id, isBytes) {
    d = new NearbyPayloadPrivate();

    d->buffer = new QBuffer();
    d->buffer->open(QBuffer::WriteOnly);
    this->setOutput(d->buffer);
}

NearbyPayload::~NearbyPayload() {
    delete d;
}

QByteArray NearbyPayload::data() {
    return d->buffer->data();
}
