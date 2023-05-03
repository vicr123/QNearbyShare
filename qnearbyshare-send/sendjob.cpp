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

//
// Created by victor on 2/05/23.
//

#include "sendjob.h"
#include <QCoreApplication>
#include <QDBusInterface>
#include <QFileInfo>
#include <sendingfile.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct SendJobPrivate {
        QDBusInterface* manager{};
        QDBusInterface* session{};
};

SendJob::SendJob(QObject* parent) :
    QObject(parent) {
    d = new SendJobPrivate();
    d->manager = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager", QDBusConnection::sessionBus(), this);
}

SendJob::~SendJob() {
    delete d;
}

bool SendJob::send(const QString& connectionString, const QString& peerName, const QList<QFile*>& files) {
    QList<QNearbyShare::DBus::SendingFile> sendingFiles;
    for (auto file : files) {
        QFileInfo fileInfo(file->fileName());
        sendingFiles.append({QDBusUnixFileDescriptor(file->handle()),
            fileInfo.fileName()});
    }

    auto reply = d->manager->call("SendToTarget", connectionString, peerName, QVariant::fromValue(sendingFiles));
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return false;
    }

    auto sessionPath = reply.arguments().first().value<QDBusObjectPath>();
    d->session = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, sessionPath.path(), QNEARBYSHARE_DBUS_SERVICE ".Session", QDBusConnection::sessionBus(), this);
    QDBusConnection::sessionBus().connect(QNEARBYSHARE_DBUS_SERVICE, sessionPath.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(sessionPropertiesChanged(QString, QVariantMap, QStringList)));
    QDBusConnection::sessionBus().connect(d->session->service(), d->session->path(), d->session->interface(), "TransfersChanged", this, SLOT(transfersChanged(QList<QNearbyShare::DBus::TransferProgress>)));

    return true;
}

void SendJob::sessionPropertiesChanged(QString interface, QVariantMap properties, QStringList changedProperties) {
    if (properties.contains("State")) {
        auto state = properties.value("State").toString();
        if (state == "Failed") {
            QTextStream(stderr) << "\n";

            auto reason = d->session->property("FailedReason").toString();
            if (reason == QStringLiteral("RemoteDeclined")) {
                QTextStream(stderr) << "<!> " << tr("The peer device declined the transfer.") << "\n";
            } else if (reason == QStringLiteral("RemoteOutOfSpace")) {
                QTextStream(stderr) << "<!> " << tr("The peer device does not have enough space available to complete the transfer.") << "\n";
            } else {
                QTextStream(stderr) << "<!> " << tr("The transfer has failed.") << "\n";
            }
            QCoreApplication::exit();
        } else if (state == "WaitingForUserAccept") {
            QTextStream(stderr) << "\n";
            QTextStream(stderr) << tr("Connection established!") << "\n";
            QTextStream(stderr) << tr("Peer Device Name: %1").arg(d->session->property("PeerName").toString()) << "\n";
            QTextStream(stderr) << tr("PIN: %1").arg(d->session->property("Pin").toString()) << "\n";
            QTextStream(stderr) << "\n";
            QTextStream(stderr) << tr("Now awaiting acceptance on peer device...") << "\n";
        } else if (state == "Transferring") {
            auto transfers = this->transfers();
            for (const auto& transfer : transfers) {
                QTextStream(stderr) << transfer.fileName << "\n";
            }
        } else if (state == "Complete") {
            QTextStream(stderr) << "\n";
            QTextStream(stderr) << tr("Transfer job complete.") << "\n";
            QCoreApplication::exit();
        }
    }
}

void SendJob::transfersChanged(QList<QNearbyShare::DBus::TransferProgress> transfers) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    qint64 filenameWidth = 0;
    for (const auto& transfer : transfers) {
        filenameWidth = qMax(filenameWidth, transfer.fileName.length());
    }

    QTextStream(stderr) << "\033[" << transfers.length() << "A";
    for (const auto& transfer : transfers) {
        auto progressLength = w.ws_col - filenameWidth - 10;
        auto progress = transfer.transferred / static_cast<double>(transfer.size);
        QString progressBar(progressLength, ' ');
        int filledProgress = progressLength * progress;
        progressBar.replace(0, filledProgress, QString(filledProgress, '#'));

        auto percentage = QString::number(static_cast<int>(round(progress * 100))).rightJustified(3, ' ');

        QTextStream(stderr) << transfer.fileName.leftJustified(filenameWidth, ' ') << "   [" << progressBar << "] " << percentage << "%\033[K\n";
    }
}

QList<QNearbyShare::DBus::TransferProgress> SendJob::transfers() {
    auto transfersMessage = d->session->call("Transfers");
    QList<QNearbyShare::DBus::TransferProgress> transfers;
    auto transfersArg = transfersMessage.arguments().first().value<QDBusArgument>();
    transfersArg >> transfers;
    return transfers;
}
