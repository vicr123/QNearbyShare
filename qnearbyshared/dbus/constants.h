

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

#ifndef QNEARBYSHARE_CONSTANTS_H
#define QNEARBYSHARE_CONSTANTS_H

#include <QString>

namespace QNearbyShare {
    const QString DBUS_SERVICE = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE);
    const QString DBUS_ROOT_PATH = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH);

    const QString DBUS_ERROR_INVALID_STATE = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE ".InvalidState");
    const QString DBUS_ERROR_INVALID_DIRECTION = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE ".InvalidDirection");
    const QString DBUS_ERROR_INVALID_CONNECTION_STRING = QStringLiteral(QNEARBYSHARE_DBUS_SERVICE ".InvalidConnectionString");
} // namespace QNearbyShare

#endif // QNEARBYSHARE_CONSTANTS_H
