

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

#include "nearbysocket.h"

#include "device_to_device_messages.pb.h"
#include "offline_wire_formats.pb.h"
#include "securemessage.pb.h"
#include "ukey.pb.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QIODevice>
#include <QMap>
#include <QRandomGenerator64>
#include <QTcpSocket>
#include <QTextStream>
#include <QTimer>
#include <QtEndian>
#include <utility>

#include <QQueue>
#include <openssl/ec.h>
#include <openssl/evp.h>

#include "abstractnearbypayload.h"
#include "cryptography.h"
#include "endpointinfo.h"
#include "nearbypayload.h"
#include "securegcm.pb.h"

struct NearbySocketPrivate {
        QIODevice* io = nullptr;

        QBuffer buffer;

        quint32 packetLength = 0;
        QByteArray packetData;

        enum State {
            ConnectingToPeer,
            WaitingForConnectionRequest,
            WaitingForUkey2ClientInit,
            WaitingForUkey2ServerInit,
            WaitingForUkey2ClientFinish,
            WaitingForConnectionResponse,
            Ready,
            Closed,
            Error
        };
        State state = WaitingForConnectionRequest;

        QString peerName;
        EcKey* clientKey = nullptr;
        QByteArray clientInitMessage;
        QByteArray serverInitMessage;
        QByteArray clientHash;
        QByteArray clientFinishMessage;

        bool isServer;
        QByteArray decryptKey;
        QByteArray receiveHmacKey;
        QByteArray encryptKey;
        QByteArray sendHmacKey;
        QByteArray authString;

        qint32 peerSeq = 0;
        qint32 mySeq = 1;

        QTimer* keepaliveTimer;
        QMap<qint64, AbstractNearbyPayloadPtr> pendingPayloads;

        QQueue<QByteArray> pendingPackets;
        quint64 pendingWrite = 0;
};

NearbySocket::NearbySocket(QIODevice* ioDevice, bool isServer, QObject* parent) :
    QObject(parent) {
    d = new NearbySocketPrivate();
    d->io = ioDevice;
    d->isServer = isServer;

    d->keepaliveTimer = new QTimer(this);
    d->keepaliveTimer->setInterval(10000);
    connect(d->keepaliveTimer, &QTimer::timeout, this, [this] {
        this->sendKeepalive(false);
    });

    connect(d->io, &QIODevice::readyRead, this, &NearbySocket::readBuffer);
    connect(d->io, &QIODevice::aboutToClose, this, [this] {
        d->keepaliveTimer->stop();
        d->state = NearbySocketPrivate::Closed;
        emit disconnected();
    });
    connect(d->io, &QIODevice::bytesWritten, this, [this](qint64 bytes) {
        d->pendingWrite -= bytes;
        if (d->pendingWrite == 0) {
            this->writeNextPacket();
        }
    });

    if (isServer) {
        d->state = NearbySocketPrivate::WaitingForConnectionRequest;
    } else {
        d->state = NearbySocketPrivate::ConnectingToPeer;

        if (auto tcp = qobject_cast<QTcpSocket*>(d->io)) {
            connect(tcp, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError error) {
                emit errorOccurred();
                emit disconnected();
            });
            this->sendConnectionRequest();
        } else {
            this->sendConnectionRequest();
        }
    }
}

NearbySocket::~NearbySocket() {
    if (d->clientKey != nullptr) {
        Cryptography::deleteEcdsaKeyPair(d->clientKey);
    }
    delete d;
}

void NearbySocket::readBuffer() {
    d->buffer.open(QBuffer::ReadWrite);
    d->buffer.write(d->io->readAll());
    d->buffer.seek(0);

    while (!d->buffer.atEnd()) {
        if (d->packetLength == 0) {
            // Read in 4 bytes for the packet length
            auto packetLength = d->buffer.read(4);
            if (packetLength.length() != 4) {
                // If we don't have 4 bytes that means we've stopped reading in the middle of the packet length
                // Save it for next time and wait for more data
                d->buffer.close();
                d->buffer.setData(packetLength);
                return;
            }

            d->packetLength = qFromBigEndian(*reinterpret_cast<quint32*>(packetLength.data()));
        }

        while (d->packetLength > 0 && !d->buffer.atEnd()) {
            auto buf = d->buffer.read(d->packetLength);
            d->packetData.append(buf);
            d->packetLength -= buf.length();
        }

        if (d->packetLength == 0) {
            // We have the entire packet now

            switch (d->state) {
                case NearbySocketPrivate::WaitingForConnectionRequest:
                case NearbySocketPrivate::WaitingForConnectionResponse:
                    processOfflineFrame(d->packetData);
                    break;
                case NearbySocketPrivate::WaitingForUkey2ClientInit:
                case NearbySocketPrivate::WaitingForUkey2ServerInit:
                case NearbySocketPrivate::WaitingForUkey2ClientFinish:
                    processUkey2Frame(d->packetData);
                    break;
                case NearbySocketPrivate::Ready:
                    processSecureFrame(d->packetData);
                    break;
            }

            d->packetData.clear();
        }
    }

    d->buffer.close();
    d->buffer.setData(QByteArray());
}

void NearbySocket::processOfflineFrame(const QByteArray& frame) {
    location::nearby::connections::OfflineFrame offlineFrame;
    auto success = offlineFrame.ParseFromString(frame.toStdString());
    if (success) {
        switch (offlineFrame.version()) {
            case location::nearby::connections::OfflineFrame_Version_V1:
                {
                    auto v1 = offlineFrame.v1();

                    switch (v1.type()) {
                        case location::nearby::connections::V1Frame_FrameType_UNKNOWN_FRAME_TYPE:
                            break;
                        case location::nearby::connections::V1Frame_FrameType_CONNECTION_REQUEST:
                            if (d->state == NearbySocketPrivate::WaitingForConnectionRequest) {
                                const auto& connectionRequest = v1.connection_request();
                                auto info = EndpointInfo::fromByteArray(QByteArray::fromStdString(connectionRequest.endpoint_info()));
                                d->peerName = info.deviceName;
                                QTextStream(stdout) << "Accepted connection from " << info.deviceName << "\n";

                                d->state = NearbySocketPrivate::WaitingForUkey2ClientInit;
                                return;
                            }
                            break;
                        case location::nearby::connections::V1Frame_FrameType_CONNECTION_RESPONSE:
                            if (d->state == NearbySocketPrivate::WaitingForConnectionResponse) {
                                const auto& connectionResponse = v1.connection_response();
                                if (connectionResponse.response() != location::nearby::connections::ConnectionResponseFrame_ResponseStatus_ACCEPT) {
                                    QTextStream(stdout) << "Client did not accept the connection\n";
                                    return;
                                }

                                if (d->isServer) {
                                    this->sendConnectionResponse();
                                }

                                d->state = NearbySocketPrivate::Ready;
                                d->keepaliveTimer->start();

                                emit readyForEncryptedMessages();
                            }
                            break;
                        case location::nearby::connections::V1Frame_FrameType_PAYLOAD_TRANSFER:
                        case location::nearby::connections::V1Frame_FrameType_BANDWIDTH_UPGRADE_NEGOTIATION:
                        case location::nearby::connections::V1Frame_FrameType_KEEP_ALIVE:
                        case location::nearby::connections::V1Frame_FrameType_DISCONNECTION:
                        case location::nearby::connections::V1Frame_FrameType_PAIRED_KEY_ENCRYPTION:
                            break;
                    }
                    break;
                }
            case location::nearby::connections::OfflineFrame_Version_UNKNOWN_VERSION:
                break;
        }
    }

    // ???
    // End the connection here
}

void NearbySocket::processUkey2Frame(const QByteArray& frame) {
    securegcm::Ukey2Alert::AlertType alertType = securegcm::Ukey2Alert_AlertType_BAD_MESSAGE;

    securegcm::Ukey2Message ukey2Message;
    auto success = ukey2Message.ParseFromString(frame.toStdString());
    if (success) {
        switch (ukey2Message.message_type()) {
            case securegcm::Ukey2Message_Type_UNKNOWN_DO_NOT_USE:
                break;
            case securegcm::Ukey2Message_Type_ALERT:
                {
                    securegcm::Ukey2Alert alert;
                    alert.ParseFromString(frame.toStdString());
                    break;
                }
            case securegcm::Ukey2Message_Type_CLIENT_INIT:
                {
                    if (d->state == NearbySocketPrivate::WaitingForUkey2ClientInit) {
                        securegcm::Ukey2ClientInit clientInit;
                        auto success = clientInit.ParseFromString(ukey2Message.message_data());
                        if (success) {
                            d->clientInitMessage = frame;

                            if (clientInit.version() != 1) {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_VERSION;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to bad version\n";
                                break;
                            }

                            if (clientInit.random().length() != 32) {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_RANDOM;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to bad random\n";
                                break;
                            }

                            if (QString::fromStdString(clientInit.next_protocol()) != "AES_256_CBC-HMAC_SHA256") {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_NEXT_PROTOCOL;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to bad next protocol\n";
                                break;
                            }

                            QByteArray commitmentHash;
                            for (const auto& commitment : clientInit.cipher_commitments()) {
                                if (commitment.handshake_cipher() == securegcm::P256_SHA512) { // TODO: Support Curve25519
                                    commitmentHash = QByteArray::fromStdString(commitment.commitment());
                                }
                            }
                            if (commitmentHash.isEmpty()) {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_HANDSHAKE_CIPHER;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to unsupported handshake commitments\n";
                                break;
                            }
                            d->clientHash = commitmentHash;

                            auto ecP256PublicKey = new securemessage::EcP256PublicKey();
                            d->clientKey = Cryptography::generateEcdsaKeyPair();

                            ecP256PublicKey->set_x(Cryptography::ecdsaX(d->clientKey).toStdString());
                            ecP256PublicKey->set_y(Cryptography::ecdsaY(d->clientKey).toStdString());

                            securemessage::GenericPublicKey publickey;
                            publickey.set_type(securemessage::EC_P256);
                            publickey.set_allocated_ec_p256_public_key(ecP256PublicKey);

                            securegcm::Ukey2ServerInit serverInit;
                            serverInit.set_version(1);
                            serverInit.set_random(QByteArray(Cryptography::randomBytes(32)).toStdString());
                            serverInit.set_handshake_cipher(securegcm::P256_SHA512);
                            serverInit.set_public_key(publickey.SerializeAsString());

                            securegcm::Ukey2Message replyMessage;
                            replyMessage.set_message_type(securegcm::Ukey2Message_Type_SERVER_INIT);
                            replyMessage.set_message_data(serverInit.SerializeAsString());

                            d->serverInitMessage = QByteArray::fromStdString(replyMessage.SerializeAsString());
                            sendPacket(d->serverInitMessage);
                            d->state = NearbySocketPrivate::WaitingForUkey2ClientFinish;
                            return;
                        }
                    }
                    break;
                }
            case securegcm::Ukey2Message_Type_SERVER_INIT:
                {
                    if (d->state == NearbySocketPrivate::WaitingForUkey2ServerInit) {
                        securegcm::Ukey2ServerInit serverInit;
                        auto success = serverInit.ParseFromString(ukey2Message.message_data());
                        if (success) {
                            d->serverInitMessage = frame;

                            if (serverInit.version() != 1) {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_VERSION;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to bad version\n";
                                break;
                            }

                            if (serverInit.random().length() != 32) {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_RANDOM;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to bad random\n";
                                break;
                            }

                            if (serverInit.handshake_cipher() != securegcm::P256_SHA512) {
                                alertType = securegcm::Ukey2Alert_AlertType_BAD_HANDSHAKE_CIPHER;
                                d->state = NearbySocketPrivate::Error;
                                emit errorOccurred();
                                QTextStream(stderr) << "Handshake failed due to bad handshake cipher\n";
                                break;
                            }

                            securemessage::GenericPublicKey serverPublicKey;
                            serverPublicKey.ParseFromString(serverInit.public_key());

                            if (serverPublicKey.type() != securemessage::EC_P256) {
                                // TODO: close connection
                                return;
                            }

                            sendPacket(d->clientFinishMessage);

                            auto ecp256 = serverPublicKey.ec_p256_public_key();
                            this->setupDiffieHellman(QByteArray::fromStdString(ecp256.x()), QByteArray::fromStdString(ecp256.y()));
                            this->sendConnectionResponse();

                            d->state = NearbySocketPrivate::WaitingForConnectionResponse;
                            return;
                        }
                    }
                    break;
                }
            case securegcm::Ukey2Message_Type_CLIENT_FINISH:
                {
                    if (d->state == NearbySocketPrivate::WaitingForUkey2ClientFinish) {
                        securegcm::Ukey2ClientFinished clientFinish;
                        auto success = clientFinish.ParseFromString(ukey2Message.message_data());
                        if (success) {
                            // https://github.com/google/ukey2#deriving-the-authentication-string-and-the-next-protocol-secret
                            securemessage::GenericPublicKey publicKey;
                            publicKey.ParseFromString(clientFinish.public_key());

                            if (publicKey.type() != securemessage::EC_P256) {
                                // TODO: close connection
                                return;
                            }

                            // TODO: Verify the commitment hash

                            auto ecp256 = publicKey.ec_p256_public_key();
                            this->setupDiffieHellman(QByteArray::fromStdString(ecp256.x()), QByteArray::fromStdString(ecp256.y()));

                            d->state = NearbySocketPrivate::WaitingForConnectionResponse;
                            return;
                        }
                    }
                    break;
                }
        }
    }

    // ???
    // End the connection here
    securegcm::Ukey2Alert alert;
    alert.set_type(alertType);
    sendPacket(alert);
}

void NearbySocket::sendPacket(const QByteArray& packet) {
    QByteArray plainPacket = packet;
    if (d->state == NearbySocketPrivate::Ready) {
        // Encrypt the packet before sending it
        securegcm::DeviceToDeviceMessage d2dm;
        d2dm.set_sequence_number(d->mySeq);
        d2dm.set_message(packet.toStdString());

        d->mySeq++;

        auto d2dmBytes = QByteArray::fromStdString(d2dm.SerializeAsString());
        auto iv = Cryptography::randomBytes(16);
        auto encrypted = Cryptography::aes256cbcEncrypt(d2dmBytes, d->encryptKey, iv);

        securegcm::GcmMetadata metadata;
        metadata.set_type(securegcm::DEVICE_TO_DEVICE_MESSAGE);
        metadata.set_version(1);

        auto header = new securemessage::Header();
        header->set_encryption_scheme(securemessage::AES_256_CBC);
        header->set_signature_scheme(securemessage::HMAC_SHA256);
        header->set_public_metadata(metadata.SerializeAsString());
        header->set_iv(iv.toStdString());

        securemessage::HeaderAndBody headerAndBody;
        headerAndBody.set_allocated_header(header);
        headerAndBody.set_body(encrypted.toStdString());

        auto headerAndBodyBytes = QByteArray::fromStdString(headerAndBody.SerializeAsString());

        securemessage::SecureMessage message;
        message.set_signature(Cryptography::hmacSha256Signature(headerAndBodyBytes, d->sendHmacKey));
        message.set_header_and_body(headerAndBodyBytes.toStdString());

        plainPacket = QByteArray::fromStdString(message.SerializeAsString());
    }

    quint32 packetLength = plainPacket.length();
    auto bePacketLength = qToBigEndian(packetLength);

    plainPacket.prepend(reinterpret_cast<char*>(&bePacketLength), 4);
    if (!plainPacket.isEmpty()) {
        d->pendingPackets.enqueue(plainPacket);
    }
    this->writeNextPacket();
}

void NearbySocket::sendPacket(const google::protobuf::MessageLite& message) {
    sendPacket(QByteArray::fromStdString(message.SerializeAsString()));
}

void NearbySocket::processSecureFrame(const QByteArray& frame) {
    securemessage::SecureMessage message;
    auto success = message.ParseFromString(frame.toStdString());
    if (!success) return;

    auto signature = QByteArray::fromStdString(message.signature());
    auto calculatedSignature = Cryptography::hmacSha256Signature(QByteArray::fromStdString(message.header_and_body()), d->receiveHmacKey);
    if (signature != calculatedSignature) {
        QTextStream(stderr) << "Received secure packet with wrong signature\n";
        return;
    }

    securemessage::HeaderAndBody headerAndBody;
    success = headerAndBody.ParseFromString(message.header_and_body());
    if (!success) return;

    if (headerAndBody.header().encryption_scheme() != securemessage::AES_256_CBC) {
        QTextStream(stderr) << "Received secure packet with wrong encryption scheme\n";
        return;
    }

    if (headerAndBody.header().signature_scheme() != securemessage::HMAC_SHA256) {
        QTextStream(stderr) << "Received secure packet with wrong signature scheme\n";
        return;
    }

    auto iv = QByteArray::fromStdString(headerAndBody.header().iv());
    auto decrypted = Cryptography::aes256cbcDecrypt(QByteArray::fromStdString(headerAndBody.body()), d->decryptKey, iv);
    if (decrypted.isEmpty()) {
        QTextStream(stderr) << "Received undecryptable secure packet\n";
        return;
    }

    securegcm::DeviceToDeviceMessage d2dm;
    success = d2dm.ParseFromString(decrypted.toStdString());
    if (!success) {
        QTextStream(stderr) << "Could not parse secure packet\n";
        return;
    }

    // TODO: sequence number
    auto seq = d2dm.sequence_number();

    location::nearby::connections::OfflineFrame offlineFrame;
    success = offlineFrame.ParseFromString(d2dm.message());
    if (!success) {
        QTextStream(stderr) << "Could not parse decrypted packet as offline frame";
        return;
    }

    if (offlineFrame.version() != location::nearby::connections::OfflineFrame_Version_V1) {
        QTextStream(stderr) << "Received offline frame with version != 1";
        return;
    }

    auto v1 = offlineFrame.v1();

    switch (v1.type()) {
        case location::nearby::connections::V1Frame_FrameType_PAYLOAD_TRANSFER:
            {
                const auto& payloadTransfer = v1.payload_transfer();
                const auto& payloadHeader = payloadTransfer.payload_header();
                const auto& payloadChunk = payloadTransfer.payload_chunk();
                auto id = payloadHeader.id();

                AbstractNearbyPayloadPtr payload;
                if (d->pendingPayloads.contains(id)) {
                    payload = d->pendingPayloads.value(id);
                } else {
                    payload = AbstractNearbyPayloadPtr(new NearbyPayload(id, payloadHeader.type() == location::nearby::connections::PayloadTransferFrame_PayloadHeader_PayloadType_BYTES));
                    d->pendingPayloads.insert(id, payload);
                }

                payload->loadChunk(payloadChunk.offset(), QByteArray::fromStdString(payloadChunk.body()));
                if (payloadChunk.flags() & location::nearby::connections::PayloadTransferFrame_PayloadChunk_Flags_LAST_CHUNK) {
                    payload->setCompleted();
                    d->pendingPayloads.remove(id);

                    emit messageReceived(payload);
                }
                break;
            }
        case location::nearby::connections::V1Frame_FrameType_KEEP_ALIVE:
            {
                auto ka = v1.keep_alive();
                if (ka.ack()) {
                    QTextStream(stderr) << "Sent keepalive was ack'd\n";
                } else {
                    sendKeepalive(true);
                }
                break;
            }
        case location::nearby::connections::V1Frame_FrameType_DISCONNECTION:
            {
                QTextStream(stderr) << "Received DISCONNECTION frame\n";
                d->io->close();

                break;
            }
        default:
            QTextStream(stderr) << "Received decrypted offline frame not PAYLOAD_TRANSFER or KEEP_ALIVE\n";
            break;
    }
}

void NearbySocket::sendPayloadPacket(const QByteArray& packet) {
    auto id = QRandomGenerator64::global()->generate();

    sendPayloadPacket(packet, id);
}

void NearbySocket::sendPayloadPacket(const google::protobuf::MessageLite& message) {
    sendPayloadPacket(QByteArray::fromStdString(message.SerializeAsString()));
}

void NearbySocket::sendPayloadPacket(const QByteArray& packet, qint64 id, PayloadType payloadType, qint64 offset, bool lastChunk) {
    location::nearby::connections::PayloadTransferFrame_PayloadHeader_PayloadType pbPayloadType;
    switch (payloadType) {
        case Bytes:
            pbPayloadType = location::nearby::connections::PayloadTransferFrame_PayloadHeader_PayloadType_BYTES;
            break;
        case File:
            pbPayloadType = location::nearby::connections::PayloadTransferFrame_PayloadHeader_PayloadType_FILE;
            break;
    }

    auto payloadHeader1 = new location::nearby::connections::PayloadTransferFrame_PayloadHeader();
    payloadHeader1->set_id(id);
    payloadHeader1->set_type(pbPayloadType);
    payloadHeader1->set_total_size(packet.length());
    payloadHeader1->set_is_sensitive(false);

    auto payloadChunk1 = new location::nearby::connections::PayloadTransferFrame_PayloadChunk();
    payloadChunk1->set_offset(offset);
    payloadChunk1->set_flags(0);
    payloadChunk1->set_body(packet.toStdString());

    auto payloadTransfer1 = new location::nearby::connections::PayloadTransferFrame();
    payloadTransfer1->set_packet_type(location::nearby::connections::PayloadTransferFrame_PacketType_DATA);
    payloadTransfer1->set_allocated_payload_header(payloadHeader1);
    payloadTransfer1->set_allocated_payload_chunk(payloadChunk1);

    auto v1_1 = new location::nearby::connections::V1Frame();
    v1_1->set_type(location::nearby::connections::V1Frame_FrameType_PAYLOAD_TRANSFER);
    v1_1->set_allocated_payload_transfer(payloadTransfer1);

    location::nearby::connections::OfflineFrame offlineFrame1;
    offlineFrame1.set_version(location::nearby::connections::OfflineFrame_Version_V1);
    offlineFrame1.set_allocated_v1(v1_1);
    sendPacket(offlineFrame1);

    if (lastChunk) {
        auto payloadHeader2 = new location::nearby::connections::PayloadTransferFrame_PayloadHeader();
        payloadHeader2->set_id(id);
        payloadHeader2->set_type(pbPayloadType);
        payloadHeader2->set_total_size(packet.length());
        payloadHeader2->set_is_sensitive(false);

        auto payloadChunk2 = new location::nearby::connections::PayloadTransferFrame_PayloadChunk();
        payloadChunk2->set_offset(packet.length() + offset);
        payloadChunk2->set_flags(location::nearby::connections::PayloadTransferFrame_PayloadChunk_Flags_LAST_CHUNK);
        payloadChunk2->set_body(QByteArray().toStdString());

        auto payloadTransfer2 = new location::nearby::connections::PayloadTransferFrame();
        payloadTransfer2->set_packet_type(location::nearby::connections::PayloadTransferFrame_PacketType_DATA);
        payloadTransfer2->set_allocated_payload_header(payloadHeader2);
        payloadTransfer2->set_allocated_payload_chunk(payloadChunk2);

        auto v1_2 = new location::nearby::connections::V1Frame();
        v1_2->set_type(location::nearby::connections::V1Frame_FrameType_PAYLOAD_TRANSFER);
        v1_2->set_allocated_payload_transfer(payloadTransfer2);

        location::nearby::connections::OfflineFrame offlineFrame2;
        offlineFrame2.set_version(location::nearby::connections::OfflineFrame_Version_V1);
        offlineFrame2.set_allocated_v1(v1_2);
        sendPacket(offlineFrame2);
    }
}

void NearbySocket::sendPayloadPacket(const google::protobuf::MessageLite& message, qint64 id) {
    sendPayloadPacket(QByteArray::fromStdString(message.SerializeAsString()), id);
}

void NearbySocket::sendKeepalive(bool isAck) {
    auto ka = new location::nearby::connections::KeepAliveFrame();
    ka->set_ack(isAck);

    auto v1 = new location::nearby::connections::V1Frame();
    v1->set_type(location::nearby::connections::V1Frame_FrameType_KEEP_ALIVE);
    v1->set_allocated_keep_alive(ka);

    location::nearby::connections::OfflineFrame offlineFrame;
    offlineFrame.set_version(location::nearby::connections::OfflineFrame_Version_V1);
    offlineFrame.set_allocated_v1(v1);

    sendPacket(offlineFrame);
}

QByteArray NearbySocket::authString() {
    return d->authString;
}

void NearbySocket::insertPendingPayload(qint64 id, const AbstractNearbyPayloadPtr& payload) {
    d->pendingPayloads.insert(id, payload);
}

QString NearbySocket::peerName() {
    return d->peerName;
}

bool NearbySocket::active() {
    return d->state != NearbySocketPrivate::Closed && d->state != NearbySocketPrivate::Error;
}

void NearbySocket::sendConnectionRequest() {
    auto connectionRequest = new location::nearby::connections::ConnectionRequestFrame();
    connectionRequest->set_endpoint_info(EndpointInfo::system().toByteArray().toStdString());

    auto v1 = new location::nearby::connections::V1Frame();
    v1->set_type(location::nearby::connections::V1Frame_FrameType_CONNECTION_REQUEST);
    v1->set_allocated_connection_request(connectionRequest);

    location::nearby::connections::OfflineFrame offlineFrame;
    offlineFrame.set_version(location::nearby::connections::OfflineFrame_Version_V1);
    offlineFrame.set_allocated_v1(v1);

    sendPacket(offlineFrame);

    // Prepare the UKey2 Client Finish
    auto ecP256PublicKey = new securemessage::EcP256PublicKey();
    d->clientKey = Cryptography::generateEcdsaKeyPair();

    ecP256PublicKey->set_x(Cryptography::ecdsaX(d->clientKey).toStdString());
    ecP256PublicKey->set_y(Cryptography::ecdsaY(d->clientKey).toStdString());

    securemessage::GenericPublicKey publicKey;
    publicKey.set_type(securemessage::EC_P256);
    publicKey.set_allocated_ec_p256_public_key(ecP256PublicKey);

    securegcm::Ukey2ClientFinished clientFinished;
    clientFinished.set_public_key(publicKey.SerializeAsString());

    securegcm::Ukey2Message replyMessage;
    replyMessage.set_message_type(securegcm::Ukey2Message_Type_CLIENT_FINISH);
    replyMessage.set_message_data(clientFinished.SerializeAsString());
    d->clientFinishMessage = QByteArray::fromStdString(replyMessage.SerializeAsString());

    // Now send the UKey2 Client Init
    securegcm::Ukey2ClientInit clientInit;
    clientInit.set_version(1);
    clientInit.set_random(Cryptography::randomBytes(32).toStdString());
    clientInit.set_next_protocol("AES_256_CBC-HMAC_SHA256");

    auto commitment = clientInit.add_cipher_commitments();
    commitment->set_handshake_cipher(securegcm::P256_SHA512); // TODO: Support Curve25519
    commitment->set_commitment(QCryptographicHash::hash(d->clientFinishMessage, QCryptographicHash::Sha512).toStdString());

    securegcm::Ukey2Message initMessage;
    initMessage.set_message_type(securegcm::Ukey2Message_Type_CLIENT_INIT);
    initMessage.set_message_data(clientInit.SerializeAsString());

    d->clientInitMessage = QByteArray::fromStdString(initMessage.SerializeAsString());
    sendPacket(d->clientInitMessage);
    d->state = NearbySocketPrivate::WaitingForUkey2ServerInit;
}

void NearbySocket::setupDiffieHellman(const QByteArray& x, const QByteArray& y) {
    auto dhs = QCryptographicHash::hash(Cryptography::diffieHellman(d->clientKey, x, y), QCryptographicHash::Sha256);
    auto m1 = d->clientInitMessage;
    auto m2 = d->serverInitMessage;
    const auto lAuth = 32;
    const auto lNext = 32;

    QByteArray m1m2;
    m1m2.append(m1);
    m1m2.append(m2);

    d->authString = Cryptography::hkdfExtractExpand("UKEY2 v1 auth", dhs, m1m2, lAuth);
    auto nextSecret = Cryptography::hkdfExtractExpand("UKEY2 v1 next", dhs, m1m2, lNext);

    auto d2dClient = Cryptography::hkdfExtractExpand(QByteArray::fromHex("82AA55A0D397F88346CA1CEE8D3909B95F13FA7DEB1D4AB38376B8256DA85510"), nextSecret, "client", 32);
    auto d2dServer = Cryptography::hkdfExtractExpand(QByteArray::fromHex("82AA55A0D397F88346CA1CEE8D3909B95F13FA7DEB1D4AB38376B8256DA85510"), nextSecret, "server", 32);

    auto keySalt = QByteArray::fromHex("BF9D2A53C63616D75DB0A7165B91C1EF73E537F2427405FA23610A4BE657642E");
    auto clientKey = Cryptography::hkdfExtractExpand(keySalt, d2dClient, "ENC:2", 32);
    auto clientHmacKey = Cryptography::hkdfExtractExpand(keySalt, d2dClient, "SIG:1", 32);
    auto serverKey = Cryptography::hkdfExtractExpand(keySalt, d2dServer, "ENC:2", 32);
    auto serverHmacKey = Cryptography::hkdfExtractExpand(keySalt, d2dServer, "SIG:1", 32);

    if (d->isServer) {
        d->decryptKey = clientKey;
        d->receiveHmacKey = clientHmacKey;
        d->encryptKey = serverKey;
        d->sendHmacKey = serverHmacKey;
    } else {
        d->decryptKey = serverKey;
        d->receiveHmacKey = serverHmacKey;
        d->encryptKey = clientKey;
        d->sendHmacKey = clientHmacKey;
    }
}

void NearbySocket::sendConnectionResponse() {
    // Send Connection Response
    auto osInfo = new location::nearby::connections::OsInfo();
    osInfo->set_type(location::nearby::connections::OsInfo_OsType_LINUX); // TODO: Update to make this correct

    auto response = new location::nearby::connections::ConnectionResponseFrame();
    response->set_response(location::nearby::connections::ConnectionResponseFrame_ResponseStatus_ACCEPT);
    response->set_allocated_os_info(osInfo);

    auto v1Response = new location::nearby::connections::V1Frame();
    v1Response->set_type(location::nearby::connections::V1Frame_FrameType_CONNECTION_RESPONSE);
    v1Response->set_allocated_connection_response(response);

    location::nearby::connections::OfflineFrame offlineResponse;
    offlineResponse.set_version(location::nearby::connections::OfflineFrame_Version_V1);
    offlineResponse.set_allocated_v1(v1Response);
    sendPacket(offlineResponse);
}

void NearbySocket::setPeerName(QString peerName) {
    d->peerName = std::move(peerName);
}

void NearbySocket::writeNextPacket() {
    if (d->pendingWrite != 0) return;
    if (d->pendingPackets.isEmpty()) {
        emit readyForNextPacket();
        return;
    };

    auto packet = d->pendingPackets.dequeue();
    if (packet.isEmpty()) {
        // This is a disconnect instruction
        d->io->close();
    } else {
        d->pendingWrite += packet.length();
        d->io->write(packet);
    }
}

void NearbySocket::disconnect() {
    // Send Disconnect
    auto disconnection = new location::nearby::connections::DisconnectionFrame();
    disconnection->set_request_safe_to_disconnect(true);

    auto v1Response = new location::nearby::connections::V1Frame();
    v1Response->set_type(location::nearby::connections::V1Frame_FrameType_DISCONNECTION);
    v1Response->set_allocated_disconnection(disconnection);

    location::nearby::connections::OfflineFrame offlineResponse;
    offlineResponse.set_version(location::nearby::connections::OfflineFrame_Version_V1);
    offlineResponse.set_allocated_v1(v1Response);
    sendPacket(offlineResponse);

    d->pendingPackets.enqueue({});
    writeNextPacket();
}
