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

//
// Created by victor on 3/05/23.
//

#ifndef QNEARBYSHARE_TRANSFERPROGRESS_H
#define QNEARBYSHARE_TRANSFERPROGRESS_H

#include <QDBusArgument>
#include <QString>

namespace QNearbyShare::DBus {
    struct TransferProgress {
            QString fileName;
            QString destination;
            quint64 size;

            quint64 transferred = 0;
            bool complete = false;
    };

    QDBusArgument& operator<<(QDBusArgument& argument, const TransferProgress& transferProgress);
    const QDBusArgument& operator>>(const QDBusArgument& argument, TransferProgress& transferProgress);
} // namespace QNearbyShare::DBus

Q_DECLARE_METATYPE(QNearbyShare::DBus::TransferProgress)

#endif // QNEARBYSHARE_TRANSFERPROGRESS_H
