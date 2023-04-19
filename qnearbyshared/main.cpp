#include <QCoreApplication>
#include <QTextStream>

#include <QDBusConnection>

#include "dbus/dbusnearbysharemanager.h"
#include "dbus/constants.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    auto manager = new DBusNearbyShareManager();

    QDBusConnection::sessionBus().registerObject(QNearbyShare::DBUS_ROOT_PATH, manager, QDBusConnection::ExportScriptableContents);
    QDBusConnection::sessionBus().registerService(QNearbyShare::DBUS_SERVICE);

    QTextStream(stdout) << "Server Running\n";

    return QCoreApplication::exec();
}
