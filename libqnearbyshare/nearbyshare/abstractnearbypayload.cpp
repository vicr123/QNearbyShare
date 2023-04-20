

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
