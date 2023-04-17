//
// Created by victor on 16/04/23.
//

#include "nearbyshareclient.h"

#include "cryptography.h"
#include "wire_format.pb.h"
#include "offline_wire_formats.pb.h"
#include "nearbysocket.h"

#include <QTextStream>

struct NearbyShareClientPrivate {
    NearbySocket* socket;
};

NearbyShareClient::NearbyShareClient(QIODevice *ioDevice, bool receive, QObject *parent) : QObject(parent) {
    d = new NearbyShareClientPrivate();
    d->socket = new NearbySocket(ioDevice, this);

    connect(d->socket, &NearbySocket::readyForEncryptedMessages, this, &NearbyShareClient::readyForEncryptedMessages);
    connect(d->socket, &NearbySocket::messageReceived, this, &NearbyShareClient::messageReceived);
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

void NearbyShareClient::messageReceived(const NearbyPayloadPtr& payload) {
    sharing::nearby::Frame nearbyFrame;
    auto success = nearbyFrame.ParseFromString(payload->data().toStdString());
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
            QTextStream(stderr) << "Got introduction frame\n";
            break;
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
