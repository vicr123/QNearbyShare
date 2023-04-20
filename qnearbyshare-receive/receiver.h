

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
