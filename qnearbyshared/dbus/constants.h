//
// Created by victor on 19/04/23.
//

#ifndef QNEARBYSHARE_CONSTANTS_H
#define QNEARBYSHARE_CONSTANTS_H

#include <QString>

namespace QNearbyShare {
    const QString DBUS_SERVICE = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE);
    const QString DBUS_ROOT_PATH = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH);

    const QString DBUS_ERROR_INVALID_STATE = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE ".InvalidState");
}

#endif//QNEARBYSHARE_CONSTANTS_H
