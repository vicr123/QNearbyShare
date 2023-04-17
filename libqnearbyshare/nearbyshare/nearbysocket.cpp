//
// Created by victor on 15/04/23.
//

#include "nearbysocket.h"

#include "offline_wire_formats.pb.h"
#include "ukey.pb.h"
#include "securemessage.pb.h"
#include "device_to_device_messages.pb.h"
#include <QBuffer>
#include <QIODevice>
#include <QTextStream>
#include <QtEndian>
#include <QCryptographicHash>

#include <openssl/ec.h>
#include <openssl/evp.h>

#include "cryptography.h"
#include "endpointinfo.h"

struct NearbySocketPrivate {
    QIODevice* io = nullptr;

    QBuffer buffer;

    quint32 packetLength = 0;
    QByteArray packetData;

    enum State {
        WaitingForConnectionRequest,
        WaitingForUkey2ClientInit,
        WaitingForUkey2ClientFinish,
        WaitingForConnectionResponse,
        Ready
    };
    State state = WaitingForConnectionRequest;

    QString remoteDeviceName;
    EVP_PKEY* clientKey = nullptr;
    QByteArray clientInitMessage;
    QByteArray serverInitMessage;
    QByteArray clientHash;

    bool isServer = true;
    QByteArray decryptKey;
    QByteArray receiveHmacKey;
    QByteArray encryptKey;
    QByteArray sendHmacKey;
};

NearbySocket::NearbySocket(QIODevice *ioDevice, QObject *parent) : QObject(parent) {
    d = new NearbySocketPrivate();
    d->io = ioDevice;

    connect(d->io, &QIODevice::readyRead, this, &NearbySocket::readBuffer);
}

NearbySocket::~NearbySocket() {
    if (d->clientKey != nullptr) {
        EVP_PKEY_free(d->clientKey);
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

        while (d->packetLength > 0) {
            auto buf = d->buffer.read(d->packetLength);
            d->packetData.append(buf);
            d->packetLength -= buf.length();
        }

        if (d->packetLength == 0) {
            //We have the entire packet now

            switch (d->state) {
                case NearbySocketPrivate::WaitingForConnectionRequest:
                case NearbySocketPrivate::WaitingForConnectionResponse:
                    processOfflineFrame(d->packetData);
                    break;
                case NearbySocketPrivate::WaitingForUkey2ClientInit:
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

void NearbySocket::processOfflineFrame(QByteArray frame) {
    location::nearby::connections::OfflineFrame offlineFrame;
    auto success = offlineFrame.ParseFromString(frame.toStdString());
    if (success) {
        switch (offlineFrame.version()) {
            case location::nearby::connections::OfflineFrame_Version_V1: {
                auto v1 = offlineFrame.v1();

                switch (v1.type()) {

                    case location::nearby::connections::V1Frame_FrameType_UNKNOWN_FRAME_TYPE:
                        break;
                    case location::nearby::connections::V1Frame_FrameType_CONNECTION_REQUEST:
                        if (d->state == NearbySocketPrivate::WaitingForConnectionRequest) {
                            const auto& connectionRequest = v1.connection_request();
                            auto info = EndpointInfo::fromByteArray(QByteArray::fromStdString(connectionRequest.endpoint_info()));
                            d->remoteDeviceName = info.deviceName;
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

                            //Send Connection Response
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

                            d->state = NearbySocketPrivate::Ready;

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

void NearbySocket::processUkey2Frame(QByteArray frame) {
    securegcm::Ukey2Alert::AlertType alertType = securegcm::Ukey2Alert_AlertType_BAD_MESSAGE;

    securegcm::Ukey2Message ukey2Message;
    auto success = ukey2Message.ParseFromString(frame.toStdString());
    if (success) {
        switch (ukey2Message.message_type()) {
            case securegcm::Ukey2Message_Type_UNKNOWN_DO_NOT_USE:
                break;
            case securegcm::Ukey2Message_Type_ALERT: {
                securegcm::Ukey2Alert alert;
                alert.ParseFromString(frame.toStdString());
                break;
            }
            case securegcm::Ukey2Message_Type_CLIENT_INIT: {
                if (d->state == NearbySocketPrivate::WaitingForUkey2ClientInit) {
                    securegcm::Ukey2ClientInit clientInit;
                    auto success = clientInit.ParseFromString(ukey2Message.message_data());
                    if (success) {
                        d->clientInitMessage = frame;

                        if (clientInit.version() != 1) {
                            alertType = securegcm::Ukey2Alert_AlertType_BAD_VERSION;
                            QTextStream(stderr) << "Handshake failed due to bad version\n";
                            break;
                        }

                        if (clientInit.random().length() != 32) {
                            alertType = securegcm::Ukey2Alert_AlertType_BAD_RANDOM;
                            QTextStream(stderr) << "Handshake failed due to bad random\n";
                            break;
                        }

                        if (QString::fromStdString(clientInit.next_protocol()) != "AES_256_CBC-HMAC_SHA256") {
                            alertType = securegcm::Ukey2Alert_AlertType_BAD_NEXT_PROTOCOL;
                            QTextStream(stderr) << "Handshake failed due to bad next protocol\n";
                            break;
                        }

                        QByteArray commitmentHash;
                        for (auto commitment : clientInit.cipher_commitments()) {
                            if (commitment.handshake_cipher() == securegcm::P256_SHA512) {
                                commitmentHash = QByteArray::fromStdString(commitment.commitment());
                            }
                        }
                        if (commitmentHash.isEmpty()) {
                            alertType = securegcm::Ukey2Alert_AlertType_BAD_HANDSHAKE_CIPHER;
                            QTextStream(stderr) << "Handshake failed due to unsupported handshake commitments\n";
                            break;
                        }
                        d->clientHash = commitmentHash;


                        auto ecP256PublicKey = new securemessage::EcP256PublicKey();
                        d->clientKey = Cryptography::generateEcdsaKeyPair();
                        auto degree = Cryptography::ecdsaDegree(Cryptography::ecdsaGroupName(d->clientKey));

                        ecP256PublicKey->set_x(Cryptography::ecdsaX(d->clientKey, degree).toStdString());
                        ecP256PublicKey->set_y(Cryptography::ecdsaY(d->clientKey, degree).toStdString());

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
                break;
            case securegcm::Ukey2Message_Type_CLIENT_FINISH: {
                if (d->state == NearbySocketPrivate::WaitingForUkey2ClientFinish) {
                    securegcm::Ukey2ClientFinished clientFinish;
                    auto success = clientFinish.ParseFromString(ukey2Message.message_data());
                    if (success) {
                        // https://github.com/google/ukey2#deriving-the-authentication-string-and-the-next-protocol-secret
                        securemessage::GenericPublicKey publickey;
                        publickey.ParseFromString(clientFinish.public_key());

                        if (publickey.type() != securemessage::EC_P256) {
                            // TODO: close connection
                            return;
                        }

                        auto ecp256 = publickey.ec_p256_public_key();

                        auto dhs = QCryptographicHash::hash(Cryptography::diffieHellman(d->clientKey, QByteArray::fromStdString(ecp256.x()), QByteArray::fromStdString(ecp256.y())), QCryptographicHash::Sha256);
                        auto m1 = d->clientInitMessage;
                        auto m2 = d->serverInitMessage;
                        const auto lAuth = 32;
                        const auto lNext = 32;

                        QByteArray m1m2;
                        m1m2.append(m1);
                        m1m2.append(m2);

                        auto authString = Cryptography::hkdfExtractExpand("UKEY2 v1 auth", dhs, m1m2, lAuth);
                        auto nextSecret = Cryptography::hkdfExtractExpand(QByteArray("UKEY2 v1 next", 13), dhs, m1m2, lNext);

                        auto d2dClient = Cryptography::hkdfExtractExpand(QByteArray::fromHex("82AA55A0D397F88346CA1CEE8D3909B95F13FA7DEB1D4AB38376B8256DA85510"), nextSecret, QByteArray("client", 6), 32);
                        auto d2dServer = Cryptography::hkdfExtractExpand(QByteArray::fromHex("82AA55A0D397F88346CA1CEE8D3909B95F13FA7DEB1D4AB38376B8256DA85510"), nextSecret, QByteArray("server", 6), 32);

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
    quint32 packetLength = packet.length();
    auto bePacketLength = qToBigEndian(packetLength);

    d->io->write(reinterpret_cast<char*>(&bePacketLength), 4);
    d->io->write(packet);
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

    auto seq = d2dm.sequence_number();
    emit messageReceived(QByteArray::fromStdString(d2dm.message()));
}
