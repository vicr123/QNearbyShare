//
// Created by victor on 16/04/23.
//

#ifndef QNEARBYSHARE_CRYPTOGRAPHY_H
#define QNEARBYSHARE_CRYPTOGRAPHY_H

#include <QString>
#include <openssl/evp.h>

namespace Cryptography {
    QByteArray randomBytes(qint64 length);

    evp_pkey_st *generateEcdsaKeyPair();
    QString ecdsaGroupName(evp_pkey_st* key);
    int ecdsaDegree(const QString& group);

    QByteArray bignumToBytes(BIGNUM* bn);
    BIGNUM* bytesToBignum(QByteArray ba);

    QByteArray ecdsaBignumParam(evp_pkey_st *key, const char *paramName, int degree);
    QByteArray ecdsaX(evp_pkey_st* key, int degree);
    QByteArray ecdsaY(evp_pkey_st* key, int degree);

    QByteArray diffieHellman(evp_pkey_st* ourKey, QByteArray peerX, QByteArray peerY);
    QByteArray hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length);

    QByteArray aes256cbcDecrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& iv);
    QByteArray hmacSha256Signature(const QByteArray& data, const QByteArray& key);
};


#endif//QNEARBYSHARE_CRYPTOGRAPHY_H
