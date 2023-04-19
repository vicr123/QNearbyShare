//
// Created by victor on 16/04/23.
//

#include "nearbyshareclient.h"

#include "cryptography.h"
#include "wire_format.pb.h"
#include "nearbysocket.h"

#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

struct NearbyShareClientPrivate {
    NearbySocket* socket = nullptr;
    QList<NearbyShareClient::TransferredFile> files;

    QMap<qint64, AbstractNearbyPayloadPtr> filePayloads;
    NearbyShareClient::State state = NearbyShareClient::State::NotReady;
};

NearbyShareClient::NearbyShareClient(QIODevice *ioDevice, bool receive, QObject *parent) : QObject(parent) {
    d = new NearbyShareClientPrivate();
    d->socket = new NearbySocket(ioDevice, this);

    connect(d->socket, &NearbySocket::readyForEncryptedMessages, this, &NearbyShareClient::readyForEncryptedMessages);
    connect(d->socket, &NearbySocket::messageReceived, this, &NearbyShareClient::messageReceived);
    connect(d->socket, &NearbySocket::disconnected, this, [this] {
        if (d->state != State::Complete && d->state != State::Failed) {
            setState(State::Failed);
        }
    });
}

NearbyShareClient::~NearbyShareClient() {
    delete d;
}

void NearbyShareClient::readyForEncryptedMessages() {
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

void NearbyShareClient::messageReceived(const AbstractNearbyPayloadPtr & payload) {
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
            case sharing::nearby::V1Frame_FrameType_INTRODUCTION: {
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
            case sharing::nearby::V1Frame_FrameType_PAIRED_KEY_ENCRYPTION: {
                auto pke = v1.paired_key_encryption();

                // ???
                sendPairedKeyEncryptionResponse();

                break;
            }
            case sharing::nearby::V1Frame_FrameType_PAIRED_KEY_RESULT:
                break;
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
