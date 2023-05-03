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

#include "console.h"
#include "devicediscovery.h"
#include "qnearbysharedbus.h"
#include "sendjob.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);
    a.setApplicationName("qnearbyshare-send");

    QNearbyShare::DBus::registerDBusMetaTypes();

    QCommandLineParser parser;
    parser.setApplicationDescription("Nearby Share");
    parser.addOption({"connection-string", "Connection String to use", "connection-string"});
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(a);

    Console console;

    if (parser.positionalArguments().isEmpty()) {
        console.outputInvocationError("missing file operand");
        return 1;
    }

    QList<QFile*> files;
    for (const auto& arg : parser.positionalArguments()) {
        auto* f = new QFile(arg);
        if (!f->open(QFile::ReadOnly)) {
            console.outputInvocationError(QStringLiteral("cannot access '%1': %2").arg(arg, f->errorString()));

            delete f;
            qDeleteAll(files);
            return 1;
        }
        files.append(f);
    }

    QString connectionString;
    QString peerName = "Remote Device";
    if (parser.isSet("connection-string")) {
        connectionString = parser.value("connection-string");
        if (connectionString.isEmpty()) {
            console.outputInvocationError("option connection-string requires a value");
            return 1;
        }
    } else {
        DeviceDiscovery discovery(&console);
        connectionString = discovery.exec(&peerName);
        if (connectionString.isEmpty()) {
            return 1;
        }
    }

    console.outputErrorLine(QStringLiteral("Sending to %1").arg(connectionString));

    SendJob job;
    if (!job.send(connectionString, peerName, files)) {
        return 1;
    }

    return a.exec();
}
