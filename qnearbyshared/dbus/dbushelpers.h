//
// Created by victor on 19/04/23.
//

#ifndef QNEARBYSHARE_DBUSHELPERS_H
#define QNEARBYSHARE_DBUSHELPERS_H


#include <QVariant>
namespace DBusHelpers {
    void emitPropertiesChangedSignal(QString path, QString interface, QString property, QVariant newValue);
};


#endif//QNEARBYSHARE_DBUSHELPERS_H
