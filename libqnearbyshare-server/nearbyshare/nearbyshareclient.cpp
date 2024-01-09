

// Copyright (c) 2023 Victor Tran
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nearbyshareclient.h"

#include "cryptography.h"
#include "nearbysocket.h"
#include "wire_format.pb.h"

#include <QDir>
#include <QMimeDatabase>
#include <QRandomGenerator64>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTextStream>
#include <QTimer>
#include <utility>

struct NearbyShareClientPrivate {
        NearbySocket* socket = nullptr;
        QList<NearbyShareClient::TransferredFile> files;

        QMap<qint64, AbstractNearbyPayloadPtr> filePayloads;
        NearbyShareClient::State state = NearbyShareClient::State::NotReady;
        NearbyShareClient::FailedReason failedReason = NearbyShareClient::FailedReason::Unknown;

        struct LocalFileStats {
                quint64 progress = 0;
                qint64 payloadId = 0;
        };

        bool isServer = false;
        QList<NearbyShareClient::LocalFile> filesToSend;
        QList<LocalFileStats> filesToSendStats;
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
                {
                    if (d->state == State::WaitingForUserAccept) {
                        const auto& response = v1.connection_response();

                        if (response.status() == sharing::nearby::ConnectionResponseFrame_Status_ACCEPT) {
                            // Start sending files!
                            setState(State::Transferring);

                            connect(d->socket, &NearbySocket::readyForNextPacket, this, &NearbyShareClient::writeNextSendPackets);
                            this->writeNextSendPackets();
                        } else {
                            switch (response.status()) {
                                case sharing::nearby::ConnectionResponseFrame_Status_REJECT:
                                    d->failedReason = FailedReason::RemoteDeclined;
                                    break;
                                case sharing::nearby::ConnectionResponseFrame_Status_NOT_ENOUGH_SPACE:
                                    d->failedReason = FailedReason::RemoteOutOfSpace;
                                    break;
                                case sharing::nearby::ConnectionResponseFrame_Status_UNSUPPORTED_ATTACHMENT_TYPE:
                                    d->failedReason = FailedReason::RemoteUnsupported;
                                    break;
                                case sharing::nearby::ConnectionResponseFrame_Status_TIMED_OUT:
                                    d->failedReason = FailedReason::RemoteTimedOut;
                                    break;
                                default:
                                    break;
                            }

                            setState(State::Failed);
                        }
                    }
                    break;
                }
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
                        QMimeDatabase mimeDb;

                        auto introduction = new sharing::nearby::IntroductionFrame();
                        for (auto i = 0; i < d->filesToSend.length(); i++) {
                            auto file = d->filesToSend.at(i);
                            auto stat = d->filesToSendStats.at(i);
                            auto f = introduction->add_file_metadata();
                            f->set_name(file.fileName.toStdString());
                            f->set_mime_type(mimeDb.mimeTypeForFile(file.fileName).name().toStdString());
                            f->set_id(QRandomGenerator64::global()->generate());
                            f->set_size(file.size);
                            f->set_payload_id(stat.payloadId);
                        }

                        auto v1 = new sharing::nearby::V1Frame();
                        v1->set_type(sharing::nearby::V1Frame_FrameType_INTRODUCTION);
                        v1->set_allocated_introduction(introduction);

                        sharing::nearby::Frame nearbyFrame;
                        nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
                        nearbyFrame.set_allocated_v1(v1);

                        d->socket->sendPayloadPacket(nearbyFrame);
                        setState(State::WaitingForUserAccept);
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
    if (d->isServer) {
        auto files = d->files;
        for (auto& file : files) {
            if (d->filePayloads.contains(file.id)) {
                auto payload = d->filePayloads.value(file.id);
                file.complete = payload->completed();
                file.transferred = payload->bytesTransferred();
            }
        }
        return files;
    } else {
        QList<NearbyShareClient::TransferredFile> files;
        for (auto i = 0; i < d->filesToSend.length(); i++) {
            auto file = d->filesToSend.at(i);
            auto stat = d->filesToSendStats.at(i);
            NearbyShareClient::TransferredFile tf;
            tf.fileName = file.fileName;
            tf.size = file.size;
            tf.transferred = stat.progress;
            tf.complete = tf.size == tf.transferred;
            files.append(tf);
        }
        return files;
    }
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

void NearbyShareClient::setState(NearbyShareClient::State state, bool dontDisconnect) {
    // TODO: Disconnect on failure
    d->state = state;
    emit stateChanged(state);

    if ((state == State::Complete || state == State::Failed) && !dontDisconnect) {
        d->socket->disconnect();
    }
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

NearbyShareClient* NearbyShareClient::clientForSend(QIODevice* device, QString peerName, QList<LocalFile> files) {
    auto client = new NearbyShareClient();
    client->d->isServer = false;

    for (auto file : files) {
        client->d->filesToSendStats.append({0,
            static_cast<qint64>(QRandomGenerator64::global()->generate())});
    }
    client->d->filesToSend = std::move(files);

    client->d->socket = new NearbySocket(device, false, client);
    client->d->socket->setPeerName(std::move(peerName));

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

NearbyShareClient::FailedReason NearbyShareClient::failedReason() {
    return d->failedReason;
}

bool NearbyShareClient::isSending() {
    return !d->isServer;
}

void NearbyShareClient::writeNextSendPackets() {
    if (d->state != State::Transferring || d->isServer) return;
    bool complete = true;
    for (auto i = 0; i < d->filesToSend.length(); i++) {
        auto file = d->filesToSend.at(i);
        auto stat = d->filesToSendStats.at(i);
        if (stat.progress == file.size) continue;

        // auto buf = file.device->read(1048576);
        auto buf = file.device->read(512 * 1024);
        d->socket->sendPayloadPacket(buf, stat.payloadId, NearbySocket::File, stat.progress, stat.progress + buf.length() == file.size, file.size);
        stat.progress += buf.length();

        d->filesToSendStats.replace(i, stat);
        complete = false;
    }

    emit filesToTransferChanged();

    if (complete) {
        // Don't send the disconnect frame now because this causes Android to think that the file was not sent correctly for some reason
        setState(State::Complete, true);
    }
}
