

/*
 * Copyright (c) 2023 Victor Tran
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
