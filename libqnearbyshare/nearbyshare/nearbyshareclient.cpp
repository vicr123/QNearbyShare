//
// Created by victor on 16/04/23.
//

#include "nearbyshareclient.h"

#include "cryptography.h"
#include "wire_format.pb.h"
#include "nearbysocket.h"

struct NearbyShareClientPrivate {
    NearbySocket* socket;
};

NearbyShareClient::NearbyShareClient(QIODevice *ioDevice, bool receive, QObject *parent) : QObject(parent) {
    d = new NearbyShareClientPrivate();
    d->socket = new NearbySocket(ioDevice, this);

    connect(d->socket, &NearbySocket::readyForEncryptedMessages, this, &NearbyShareClient::readyForEncryptedMessages);
}

NearbyShareClient::~NearbyShareClient() {
    delete d;
}

void NearbyShareClient::readyForEncryptedMessages() {
    auto pke = new sharing::nearby::PairedKeyEncryptionFrame();
    pke->set_secret_id_hash(Cryptography::randomBytes(6));
    pke->set_signed_data(Cryptography::randomBytes(72));

    auto v1 = new sharing::nearby::V1Frame();
    v1->set_type(sharing::nearby::V1Frame_FrameType_PAIRED_KEY_ENCRYPTION);
    v1->set_allocated_paired_key_encryption(pke);

    sharing::nearby::Frame nearbyFrame;
    nearbyFrame.set_version(sharing::nearby::Frame_Version_V1);
    nearbyFrame.set_allocated_v1(v1);
}
