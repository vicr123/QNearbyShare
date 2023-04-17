//
// Created by victor on 15/04/23.
//

#include "endpointinfo.h"
#include <QHostInfo>
#include <QRandomGenerator>

EndpointInfo EndpointInfo::fromByteArray(const QByteArray& data) {
    if (data.length() < 17) return {};

    EndpointInfo info;

    uchar byte1 = data.at(0);
    info.version = (byte1 & 0b11100000) >> 5;
    info.visible = (byte1 & 0b00010000) == 0;
    info.deviceType = (byte1 & 0b00001110) >> 1;

    auto deviceNameLength = data.at(17);
    if (data.length() < deviceNameLength + 17) return {};
    info.deviceName = data.mid(18, deviceNameLength);

    return info;

}
QByteArray EndpointInfo::toByteArray() {
    uchar byte1 = version << 5 |
            (visible ? 0b00000000 : 0b00010000) |
                  (deviceType & 0b00000111) << 1;

    QByteArray endpointInfo;
    endpointInfo.append(byte1);

    quint32 array[4];
    QRandomGenerator::global()->fillRange(array);
    endpointInfo.append(reinterpret_cast<char*>(array), 16);

    endpointInfo.append(static_cast<uchar>(this->deviceName.length()));
    endpointInfo.append(this->deviceName.toUtf8());

    return endpointInfo;
}
