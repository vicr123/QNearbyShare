#include <QCoreApplication>
#include <QTextStream>

#include "nearbyshare/nearbyshareserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    NearbyShareServer server;
    server.start();

    QTextStream(stdout) << "Server Running\n";

    return QCoreApplication::exec();
}
