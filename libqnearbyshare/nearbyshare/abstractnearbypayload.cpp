//
// Created by victor on 18/04/23.
//

#include "abstractnearbypayload.h"
#include "nearbypayload.h"
#include <QTextStream>
#include <QIODevice>

struct AbstractNearbyPayloadPrivate {
    qint64 id;
    bool isBytes;

    QIODevice* output;
    quint64 read = 0;
    bool completed = false;
};

AbstractNearbyPayload::AbstractNearbyPayload(qint64 id, bool isBytes) {
    d = new AbstractNearbyPayloadPrivate();
    d->id = id;
    d->isBytes = isBytes;
}

AbstractNearbyPayload::~AbstractNearbyPayload() {
    d->output->deleteLater();
    delete d;
}

void AbstractNearbyPayload::setCompleted() {
    d->completed = true;
    d->output->close();
    emit complete();
}

bool AbstractNearbyPayload::completed() {
    return d->completed;
}

bool AbstractNearbyPayload::isBytes() {
    return d->isBytes;
}

qint64 AbstractNearbyPayload::id() {
    return d->id;
}

void AbstractNearbyPayload::loadChunk(quint64 offset, const QByteArray &body) {
    if (d->read != offset) {
        // Stop!
        QTextStream(stderr) << "Nearby Payload offset jumped unexpectedly\n";
        return;
    }
    d->output->write(body);
    d->read += body.length();
    emit transferredChanged();
}

void AbstractNearbyPayload::setOutput(QIODevice *output) {
    d->output = output;
}

quint64 AbstractNearbyPayload::bytesTransferred() {
    return d->read;
}
