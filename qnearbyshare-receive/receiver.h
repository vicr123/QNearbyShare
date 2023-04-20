//
// Created by victor on 19/04/23.
//

#ifndef QNEARBYSHARE_RECEIVER_H
#define QNEARBYSHARE_RECEIVER_H

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QObject>
#include <functional>

struct TransferProgress {
    QString fileName;
    QString destination;
    quint64 size;

    quint64 transferred = 0;
    bool complete = false;
};

QDBusArgument& operator<<(QDBusArgument& argument, const TransferProgress& transferProgress);
const QDBusArgument& operator>>(const QDBusArgument& argument, TransferProgress& transferProgress);

struct ReceiverPrivate;
class Receiver : public QObject {
    Q_OBJECT
public:
    explicit Receiver(QObject* parent = nullptr);
    ~Receiver();

    bool startListening();
    void acceptTransfer();
    void rejectTransfer();

private slots:
    void newSession(QDBusObjectPath path);
    void sessionPropertiesChanged(QString interface, QVariantMap properties, QStringList changedProperties);
    void transfersChanged(QList<TransferProgress> transfers);

private:
    ReceiverPrivate* d;

    void question(QString question, std::function<void()> yes, std::function<void()> no);
    QList<TransferProgress> transfers();
};

Q_DECLARE_METATYPE(TransferProgress)
Q_DECLARE_METATYPE(QList<TransferProgress>)

#endif//QNEARBYSHARE_RECEIVER_H
