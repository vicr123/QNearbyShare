#include <QCoreApplication>
#include <QTextStream>

#include "nearbyshare/nearbyshareserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    NearbyShareServer server;
    server.start();

    QObject::connect(&server, &NearbyShareServer::newShare, [](NearbyShareClient* client) {
        client->rejectTransfer();
    });

    QTextStream(stdout) << "Server Running\n";

    return QCoreApplication::exec();
}
