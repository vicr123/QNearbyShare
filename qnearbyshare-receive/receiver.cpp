

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

#include "receiver.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QLocale>
#include <QSocketNotifier>
#include <dbusconstants.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct ReceiverPrivate {
        QDBusInterface* manager{};
        QDBusInterface* session{};
};

Receiver::Receiver(QObject* parent) :
    QObject(parent) {
    d = new ReceiverPrivate();
    d->manager = new QDBusInterface(QNearbyShare::DBus::DBUS_SERVICE, QNearbyShare::DBus::DBUS_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager");
}

Receiver::~Receiver() {
    delete d;
}

bool Receiver::startListening() {
    auto reply = d->manager->call("StartListening");
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return false;
    }

    QDBusConnection::sessionBus().connect(QNearbyShare::DBus::DBUS_SERVICE, QNearbyShare::DBus::DBUS_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager", "NewSession", this, SLOT(newSession(QDBusObjectPath)));

    QTextStream(stderr) << tr("Awaiting a Nearby Share connection.") << "\n";
    QTextStream(stderr) << tr("Use Nearby Share on another device to connect to this device to share files.") << "\n";
    QTextStream(stderr) << "\n";
    QTextStream(stderr) << tr("This Device Name: %1").arg(d->manager->property("ServerName").toString()) << "\n";
    return true;
}

void Receiver::newSession(QDBusObjectPath path) { // NOLINT(performance-unnecessary-value-param)
    d->session = new QDBusInterface(QNearbyShare::DBus::DBUS_SERVICE, path.path(), QNEARBYSHARE_DBUS_SERVICE ".Session");
    QDBusConnection::sessionBus().connect(QNearbyShare::DBus::DBUS_SERVICE, path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(sessionPropertiesChanged(QString, QVariantMap, QStringList)));

    auto transfers = this->transfers();

    QTextStream(stderr) << "\n";
    QTextStream(stderr) << tr("Connection established!") << "\n";
    QTextStream(stderr) << tr("Peer Device Name: %1").arg(d->session->property("PeerName").toString()) << "\n";
    QTextStream(stderr) << tr("PIN: %1").arg(d->session->property("Pin").toString()) << "\n";
    QTextStream(stderr) << "\n";

    QTextStream(stderr) << tr("Files to transfer:") << "\n";

    qint64 filenameWidth = 0;
    for (const auto& transfer : transfers) {
        filenameWidth = qMax(filenameWidth, transfer.fileName.length());
    }

    QTextStream(stderr) << tr("FILE NAME").leftJustified(filenameWidth, ' ') << "   " << tr("SIZE") << "\n";
    for (const auto& transfer : transfers) {
        QTextStream(stderr) << transfer.fileName.leftJustified(filenameWidth, ' ') << "   " << QLocale().formattedDataSize(transfer.size, 2) << "\n";
    }

    QTextStream(stderr) << "\n";

    this->question(
        tr("Proceed with transfer?"), [this] {
            acceptTransfer();
        },
        [this] {
        rejectTransfer();
        });
}

void Receiver::sessionPropertiesChanged(QString interface, QVariantMap properties, QStringList changedProperties) {
    if (properties.contains("State")) {
        auto state = properties.value("State").toString();
        if (state == "Failed") {
            QTextStream(stderr) << "\n";
            QTextStream(stderr) << "<!> " << tr("The transfer has failed.") << "\n";
            QCoreApplication::exit();
        } else if (state == "Complete") {
            QTextStream(stderr) << "\n";
            QTextStream(stderr) << tr("Transfer job complete.") << "\n";
            QCoreApplication::exit();
        }
    }
}

void Receiver::question(QString question, std::function<void()> yes, std::function<void()> no) {
    QTextStream(stderr) << ":: " << question << " " << tr("[Y/n]") << " ";
    auto notifier = new QSocketNotifier(stdin->_fileno, QSocketNotifier::Read);
    connect(notifier, &QSocketNotifier::activated, this, [yes, no, question, this, notifier] {
        notifier->deleteLater();

        QTextStream s(stdin, QTextStream::ReadOnly);
        auto response = s.readLine();
        if (response.isEmpty()) {
            yes();
            return;
        }

        if (response.startsWith("y") || response.startsWith("Y")) {
            yes();
            return;
        }

        if (response.startsWith("n") || response.startsWith("N")) {
            no();
            return;
        }

        // Invalid answer: ask again
        this->question(question, yes, no);
    });
}

void Receiver::acceptTransfer() {
    d->session->call("AcceptTransfer");

    QTextStream(stderr) << tr("Starting transfer...") << "\n";

    auto transfers = this->transfers();
    for (auto transfer : transfers) {
        QTextStream(stderr) << transfer.fileName << "\n";
    }

    QDBusConnection::sessionBus().connect(d->session->service(), d->session->path(), d->session->interface(), "TransfersChanged", this, SLOT(transfersChanged(QList<QNearbyShare::DBus::TransferProgress>)));
}

void Receiver::rejectTransfer() {
    d->session->call("RejectTransfer");

    QTextStream(stderr) << "<!> " << tr("Rejected incoming transfer.") << "\n";
}

QList<QNearbyShare::DBus::TransferProgress> Receiver::transfers() {
    auto transfersMessage = d->session->call("Transfers");
    QList<QNearbyShare::DBus::TransferProgress> transfers;
    auto transfersArg = transfersMessage.arguments().first().value<QDBusArgument>();
    transfersArg >> transfers;
    return transfers;
}

void Receiver::transfersChanged(QList<QNearbyShare::DBus::TransferProgress> transfers) {
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
