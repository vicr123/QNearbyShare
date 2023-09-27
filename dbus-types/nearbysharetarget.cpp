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
// Created by victor on 1/05/23.
//

#include "nearbysharetarget.h"

QDBusArgument& QNearbyShare::DBus::operator<<(QDBusArgument& argument, const QNearbyShare::DBus::NearbyShareTarget& target) {
    argument.beginStructure();
    argument << target.connectionString << target.name << target.deviceType;
    argument.endStructure();
    return argument;
}

const QDBusArgument& QNearbyShare::DBus::operator>>(const QDBusArgument& argument, QNearbyShare::DBus::NearbyShareTarget& target) {
    argument.beginStructure();
    argument >> target.connectionString >> target.name >> target.deviceType;
    argument.endStructure();
    return argument;
}
