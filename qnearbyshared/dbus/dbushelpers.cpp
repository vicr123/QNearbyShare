//
// Created by victor on 19/04/23.
//

#include "dbushelpers.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include "constants.h"

void DBusHelpers::emitPropertiesChangedSignal(QString path, QString interface, QString property, QVariant newValue) {
    auto signal = QDBusMessage::createSignal(path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"));
    signal.setArguments({
            interface,
            QVariantMap({{property, newValue}}),
            QStringList({property})
    });
    QDBusConnection::sessionBus().send(signal);
}
