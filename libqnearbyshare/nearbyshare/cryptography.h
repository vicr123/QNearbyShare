//
// Created by victor on 16/04/23.
//

#ifndef QNEARBYSHARE_CRYPTOGRAPHY_H
#define QNEARBYSHARE_CRYPTOGRAPHY_H

#include <QString>
#include <openssl/evp.h>

struct EcKey;
namespace Cryptography {
    QByteArray randomBytes(qint64 length);

    EcKey *generateEcdsaKeyPair();
    void deleteEcdsaKeyPair(EcKey* key);

    QByteArray ecdsaBignumParam(EcKey *key, const char *paramName, int degree);
    QByteArray ecdsaX(EcKey* key);
    QByteArray ecdsaY(EcKey* key);

    QByteArray diffieHellman(EcKey* ourKey, QByteArray peerX, QByteArray peerY);
    QByteArray hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length);

    QByteArray aes256cbc(const QByteArray &input, const QByteArray &key, const QByteArray &iv, bool isEncrypt);
    QByteArray aes256cbcDecrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& iv);
    QByteArray aes256cbcEncrypt(const QByteArray& plaintext, const QByteArray& key, const QByteArray& iv);
    QByteArray hmacSha256Signature(const QByteArray& data, const QByteArray& key);
};


#endif//QNEARBYSHARE_CRYPTOGRAPHY_H
