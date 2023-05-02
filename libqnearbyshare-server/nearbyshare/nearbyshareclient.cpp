

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

#include "nearbyshareclient.h"

#include "cryptography.h"
#include "nearbysocket.h"
#include "wire_format.pb.h"

#include <QDir>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTextStream>

struct NearbyShareClientPrivate {
        NearbySocket* socket = nullptr;
        QList<NearbyShareClient::TransferredFile> files;

        QMap<qint64, AbstractNearbyPayloadPtr> filePayloads;
        NearbyShareClient::State state = NearbyShareClient::State::NotReady;

        bool isServer;

        enum InternalState {
            WaitingForNearbyConnection,
            WaitingForPairedKeyEncryption
        };
        InternalState internalState = WaitingForNearbyConnection;
};

NearbyShareClient::NearbyShareClient(QObject* parent) :
    QObject(parent) {
    d = new NearbyShareClientPrivate();
}

NearbyShareClient::~NearbyShareClient() {
    delete d;
}

void NearbyShareClient::readyForEncryptedMessages() {
    //    if (d->isServer) {
    auto pke = new sharing::nearby::PairedKeyEncryptionFrame();
    pke->set_secret_id_hash(Cryptography::randomBytes(6).toStdString());
    pke->set_signed_data(Cryptography::randomBytes(72).toStdString());

    auto v1 = new sharing::nearby::V1Frame();
    v1->set_type(sharing::nearby::V1Frame_FrameType_PAIRED_KEY_ENCRYPTION);
    v1->set_allocated_paired_key_encryption(pke);

    sharing::nearby::Frame nearbyFrame;
    nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
    nearbyFrame.set_allocated_v1(v1);

    d->socket->sendPayloadPacket(nearbyFrame);
    //    }
    d->internalState = NearbyShareClientPrivate::WaitingForPairedKeyEncryption;
}

void NearbyShareClient::messageReceived(const AbstractNearbyPayloadPtr& payload) {
    if (auto dataPayload = payload.objectCast<NearbyPayload>()) {
        sharing::nearby::Frame nearbyFrame;
        auto success = nearbyFrame.ParseFromString(dataPayload->data().toStdString());
        if (!success) {
            QTextStream(stderr) << "Could not parse nearby frame\n";
            return;
        }

        if (nearbyFrame.version() != sharing::nearby::Frame_Version_V1) {
            QTextStream(stderr) << "Received nearby frame version != 1\n";
            return;
        }

        auto v1 = nearbyFrame.v1();
        switch (v1.type()) {
            case sharing::nearby::V1Frame_FrameType_UNKNOWN_FRAME_TYPE:
                break;
            case sharing::nearby::V1Frame_FrameType_INTRODUCTION:
                {
                    const auto& introduction = v1.introduction();

                    QTextStream(stdout) << "Ready for transfer from remote device:\n";
                    QTextStream(stdout) << "PIN: " << pinCodeFromAuthString(d->socket->authString()) << "\n";
                    QTextStream(stdout) << "Incoming files:\n";

                    QDir downloads(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
                    for (const auto& meta : introduction.file_metadata()) {
                        TransferredFile tf;
                        tf.id = meta.payload_id();
                        tf.fileName = QString::fromStdString(meta.name());
                        tf.size = meta.size();

                        // TODO: Check for conflicts
                        tf.destination = downloads.absoluteFilePath(tf.fileName);
                        d->files.append(tf);

                        QTextStream(stdout) << "  " << QString::fromStdString(meta.name()) << "   len: " << meta.size() << "   mime: " << QString::fromStdString(meta.mime_type()) << "\n";
                    }

                    setState(State::WaitingForUserAccept);
                    emit negotiationCompleted();

                    break;
                }
            case sharing::nearby::V1Frame_FrameType_RESPONSE:
                break;
            case sharing::nearby::V1Frame_FrameType_PAIRED_KEY_ENCRYPTION:
                {
                    auto pke = v1.paired_key_encryption();

                    // ???
                    sendPairedKeyEncryptionResponse();

                    break;
                }
            case sharing::nearby::V1Frame_FrameType_PAIRED_KEY_RESULT:
                {
                    if (!d->isServer) {
                        // Introduce the files to be sent
                        auto introduction = new sharing::nearby::IntroductionFrame();
                        auto f = introduction->add_file_metadata();
                        f->set_name("MY_FILE.png");
                        f->set_mime_type("image/png");
                        f->set_id(10);
                        f->set_size(3000);
                        f->set_payload_id(6000);

                        auto v1 = new sharing::nearby::V1Frame();
                        v1->set_type(sharing::nearby::V1Frame_FrameType_INTRODUCTION);
                        v1->set_allocated_introduction(introduction);

                        sharing::nearby::Frame nearbyFrame;
                        nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
                        nearbyFrame.set_allocated_v1(v1);

                        d->socket->sendPayloadPacket(nearbyFrame);
                    }
                    break;
                }
            case sharing::nearby::V1Frame_FrameType_CERTIFICATE_INFO:
                break;
            case sharing::nearby::V1Frame_FrameType_CANCEL:
                break;
        }
    }
}

void NearbyShareClient::sendPairedKeyEncryptionResponse() {
    auto pkr = new sharing::nearby::PairedKeyResultFrame();
    pkr->set_status(sharing::nearby::PairedKeyResultFrame_Status_UNABLE);

    auto v1 = new sharing::nearby::V1Frame();
    v1->set_type(sharing::nearby::V1Frame_FrameType_PAIRED_KEY_RESULT);
    v1->set_allocated_paired_key_result(pkr);

    sharing::nearby::Frame nearbyFrame;
    nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
    nearbyFrame.set_allocated_v1(v1);

    d->socket->sendPayloadPacket(nearbyFrame);
}

QString NearbyShareClient::pinCodeFromAuthString(const QByteArray& authString) {
    int hash = 0;
    int multiplier = 1;
    for (auto c : authString) {
        uchar uc = *reinterpret_cast<uchar*>(&c);
        hash = (hash + static_cast<char>(uc) * multiplier) % 9973;
        multiplier = (multiplier * 31) % 9973;
    }

    return QString::number(abs(hash)).rightJustified(4, '0');
}

void NearbyShareClient::acceptTransfer() {
    // Create all the files to transfer
    for (const auto& tf : d->files) {
        auto outputFile = new QFile(tf.destination);
        outputFile->open(QFile::WriteOnly);

        auto payload = AbstractNearbyPayloadPtr(new AbstractNearbyPayload(tf.id, false));
        payload->setOutput(outputFile);
        connect(payload.data(), &AbstractNearbyPayload::transferredChanged, this, &NearbyShareClient::filesToTransferChanged);
        connect(payload.data(), &AbstractNearbyPayload::complete, this, [this] {
            emit filesToTransferChanged();
            emit checkIfComplete();
        });

        d->filePayloads.insert(tf.id, payload);
        d->socket->insertPendingPayload(tf.id, payload);
    }

    auto rsp = new sharing::nearby::ConnectionResponseFrame();
    rsp->set_status(sharing::nearby::ConnectionResponseFrame_Status_ACCEPT);

    auto v1 = new sharing::nearby::V1Frame();
    v1->set_type(sharing::nearby::V1Frame_FrameType_RESPONSE);
    v1->set_allocated_connection_response(rsp);

    sharing::nearby::Frame nearbyFrame;
    nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
    nearbyFrame.set_allocated_v1(v1);

    QTextStream(stdout) << "Accepting transfer from remote device\n";
    d->socket->sendPayloadPacket(nearbyFrame);

    setState(State::Transferring);
}

void NearbyShareClient::rejectTransfer() {
    auto rsp = new sharing::nearby::ConnectionResponseFrame();
    rsp->set_status(sharing::nearby::ConnectionResponseFrame_Status_REJECT);

    auto v1 = new sharing::nearby::V1Frame();
    v1->set_type(sharing::nearby::V1Frame_FrameType_RESPONSE);
    v1->set_allocated_connection_response(rsp);

    sharing::nearby::Frame nearbyFrame;
    nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
    nearbyFrame.set_allocated_v1(v1);

    QTextStream(stdout) << "Rejecting transfer from remote device\n";
    d->socket->sendPayloadPacket(nearbyFrame);

    setState(State::Failed);
}

QList<NearbyShareClient::TransferredFile> NearbyShareClient::filesToTransfer() {
    auto files = d->files;

    for (auto& file : files) {
        if (d->filePayloads.contains(file.id)) {
            auto payload = d->filePayloads.value(file.id);
            file.complete = payload->completed();
            file.transferred = payload->bytesTransferred();
        }
    }

    return files;
}

QString NearbyShareClient::pin() {
    return pinCodeFromAuthString(d->socket->authString());
}

QString NearbyShareClient::peerName() {
    return d->socket->peerName();
}

NearbyShareClient::State NearbyShareClient::state() {
    return d->state;
}

void NearbyShareClient::setState(NearbyShareClient::State state) {
    d->state = state;
    emit stateChanged(state);
}

void NearbyShareClient::checkIfComplete() {
    if (d->state != State::Transferring) return;
    for (const auto& file : this->filesToTransfer()) {
        if (!file.complete) return;
    }

    setState(State::Complete);
}

NearbyShareClient* NearbyShareClient::clientForReceive(QIODevice* device) {
    auto client = new NearbyShareClient();
    client->d->isServer = true;

    client->d->socket = new NearbySocket(device, true, client);

    connect(client->d->socket, &NearbySocket::readyForEncryptedMessages, client, &NearbyShareClient::readyForEncryptedMessages);
    connect(client->d->socket, &NearbySocket::messageReceived, client, &NearbyShareClient::messageReceived);
    connect(client->d->socket, &NearbySocket::disconnected, client, [client] {
        if (client->d->state != State::Complete && client->d->state != State::Failed) {
            client->setState(State::Failed);
        }
    });

    return client;
}

NearbyShareClient* NearbyShareClient::clientForSend(QIODevice* device, QList<LocalFile> files) {
    auto client = new NearbyShareClient();
    client->d->isServer = false;

    client->d->socket = new NearbySocket(device, false, client);

    connect(client->d->socket, &NearbySocket::readyForEncryptedMessages, client, &NearbyShareClient::readyForEncryptedMessages);
    connect(client->d->socket, &NearbySocket::messageReceived, client, &NearbyShareClient::messageReceived);
    connect(client->d->socket, &NearbySocket::disconnected, client, [client] {
        if (client->d->state != State::Complete && client->d->state != State::Failed) {
            client->setState(State::Failed);
        }
    });

    return client;
}

QIODevice* NearbyShareClient::resolveConnectionString(const QString& connectionString) {
    auto parts = connectionString.split(":");
    if (parts.first() == "tcp") {
        const auto& host = parts.at(1);
        const auto& portStr = parts.at(2);

        bool ok;
        auto port = portStr.toUInt(&ok);

        if (!ok) return nullptr;

        auto* socket = new QTcpSocket();
        socket->connectToHost(host, port);
        return socket;
    }

    return nullptr;
}
