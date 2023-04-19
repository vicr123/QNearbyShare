//
// Created by victor on 19/04/23.
//

#include "receiver.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QLocale>
#include <QSocketNotifier>

struct ReceiverPrivate {
    QDBusInterface* manager{};
    QDBusInterface* session{};
};

QDBusArgument& operator<<(QDBusArgument& argument, const TransferProgress& transferProgress) {
    argument.beginStructure();
    argument << transferProgress.fileName << transferProgress.destination << transferProgress.transferred << transferProgress.size << transferProgress.complete;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, TransferProgress& transferProgress) {
    argument.beginStructure();
    argument >> transferProgress.fileName >> transferProgress.destination >> transferProgress.transferred >> transferProgress.size >> transferProgress.complete;
    argument.endStructure();
    return argument;
}

Receiver::Receiver(QObject *parent) : QObject(parent) {
    d = new ReceiverPrivate();
    d->manager = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager");
}

Receiver::~Receiver() {
    delete d;
}

bool Receiver::startListening() {
    auto reply = d->manager->call("StartListening");
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return false;
    }

    QDBusConnection::sessionBus().connect(QNEARBYSHARE_DBUS_SERVICE, QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH, QNEARBYSHARE_DBUS_SERVICE ".Manager", "NewSession", this, SLOT(newSession(QDBusObjectPath)));

    QTextStream(stderr) << tr("Awaiting a Nearby Share connection.") << "\n";
    QTextStream(stderr) << tr("Use Nearby Share on another device to connect to this device to share files.") << "\n";
    QTextStream(stderr) << "\n";
    QTextStream(stderr) << tr("This Device Name: %1").arg(d->manager->property("ServerName").toString()) << "\n";
    return true;
}

void Receiver::newSession(QDBusObjectPath path) {
    d->session = new QDBusInterface(QNEARBYSHARE_DBUS_SERVICE, path.path(), QNEARBYSHARE_DBUS_SERVICE ".Session");
    QDBusConnection::sessionBus().connect(QNEARBYSHARE_DBUS_SERVICE, path.path(), "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(sessionPropertiesChanged(QString,QVariantMap,QStringList)));

    auto transfersMessage = d->session->call("Transfers");
    QList<TransferProgress> transfers;
    auto transfersArg = transfersMessage.arguments().first().value<QDBusArgument>();
    transfersArg >> transfers;

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

    this->question(tr("Proceed with transfer?"), [this] {
        acceptTransfer();
    }, [this] {
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
    connect(notifier, &QSocketNotifier::activated, this, [yes, no, question, this] {
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
}

void Receiver::rejectTransfer() {
    d->session->call("RejectTransfer");

    QTextStream(stderr) << "<!> " << tr("Rejected incoming transfer.") << "\n";
}
