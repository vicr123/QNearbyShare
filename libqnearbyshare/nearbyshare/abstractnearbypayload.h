//
// Created by victor on 18/04/23.
//

#ifndef QNEARBYSHARE_ABSTRACTNEARBYPAYLOAD_H
#define QNEARBYSHARE_ABSTRACTNEARBYPAYLOAD_H

#include <QObject>
#include <QByteArray>
#include <QSharedPointer>

struct AbstractNearbyPayloadPrivate;
class AbstractNearbyPayload : public QObject {
    Q_OBJECT

public:
    explicit AbstractNearbyPayload(qint64 id, bool isBytes);
    ~AbstractNearbyPayload();

    void setOutput(QIODevice* output);
    void loadChunk(quint64 offset, const QByteArray& body);

    void setCompleted();
    bool completed();
    bool isBytes();
    qint64 id();
    quint64 bytesTransferred();

    signals:
        void complete();
        void transferredChanged();

private:
    AbstractNearbyPayloadPrivate* d;
};

typedef QSharedPointer<AbstractNearbyPayload> AbstractNearbyPayloadPtr;


#endif//QNEARBYSHARE_ABSTRACTNEARBYPAYLOAD_H
