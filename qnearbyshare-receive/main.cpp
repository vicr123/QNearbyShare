//
// Created by victor on 19/04/23.
//

#include <QCoreApplication>
#include <QTextStream>
#include <QCommandLineParser>
#include <QDBusConnection>
#include "receiver.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    a.setApplicationName("qnearbyshare-receive");

    QCommandLineParser parser;
    parser.setApplicationDescription("Nearby Share");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(a);

    Receiver receiver;

    if (!receiver.startListening()) {
        QTextStream(stderr) << "Could not register listener. Is the DBus service running?\n";
        return 1;
    }

    return a.exec();
}
