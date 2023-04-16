//
// Created by victor on 15/04/23.
//

#ifndef QNEARBYSHARE_ENDPOINTINFO_H
#define QNEARBYSHARE_ENDPOINTINFO_H


#include <QByteArray>
#include <QString>

struct EndpointInfo {
    uint version = 0;
    bool visible = true;
    uint deviceType = 3;
    QString deviceName;

    static EndpointInfo fromByteArray(const QByteArray& data);
    QByteArray toByteArray();
};


#endif//QNEARBYSHARE_ENDPOINTINFO_H
