//
// Created by victor on 16/04/23.
//

#include "cryptography.h"
#include <QRandomGenerator>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <QMessageAuthenticationCode>


EVP_PKEY *Cryptography::generateEcdsaKeyPair() {
    auto ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (ctx == nullptr) {
        return nullptr;
    }

    // Initialize the EVP context for key generation
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    // Set the EC curve for key generation
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    // Generate the ECDSA key pair
    EVP_PKEY* clientKey;
    if (EVP_PKEY_keygen(ctx, &clientKey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    EVP_PKEY_CTX_free(ctx);
    return clientKey;
}

int Cryptography::ecdsaDegree(const QString& group) {
    auto ecGroup = EC_GROUP_new_by_curve_name(OBJ_sn2nid(group.toLatin1().data()));
    auto degree = EC_GROUP_get_degree(ecGroup);
    EC_GROUP_free(ecGroup);
    return degree;
}

QString Cryptography::ecdsaGroupName(evp_pkey_st *key) {
    char groupNameBytes[64];
    size_t groupNameLength;
    auto groupOk = EVP_PKEY_get_utf8_string_param(key, OSSL_PKEY_PARAM_GROUP_NAME, groupNameBytes, 64, &groupNameLength);
    if (!groupOk) {
        return "";
    }
    return QString::fromLatin1(groupNameBytes, groupNameLength);
}

QByteArray Cryptography::ecdsaBignumParam(evp_pkey_st *key, const char *paramName, int degree) {
    auto n = BN_new();
    auto ok = EVP_PKEY_get_bn_param(key, paramName, &n);
    if (!ok) {
        BN_free(n);
        return {};
    }

    QByteArray nBytes(degree / 8, Qt::Uninitialized);

    BN_bn2binpad(n, reinterpret_cast<unsigned char *>(nBytes.data()), degree / 8);
    BN_free(n);
    return nBytes;
}

QByteArray Cryptography::ecdsaX(evp_pkey_st *key, int degree) {
    return ecdsaBignumParam(key, OSSL_PKEY_PARAM_EC_PUB_X, degree);
}

QByteArray Cryptography::ecdsaY(evp_pkey_st *key, int degree) {
    return ecdsaBignumParam(key, OSSL_PKEY_PARAM_EC_PUB_Y, degree);
}

QByteArray Cryptography::diffieHellman(evp_pkey_st *ourKey, QByteArray peerX, QByteArray peerY) {
    QByteArray peerEncodedKey;
    peerEncodedKey.append(0x04);
    peerEncodedKey.append(peerX);
    peerEncodedKey.append(peerY);

    auto bnX = BN_new();
    auto bnY = BN_new();

    BN_bin2bn(reinterpret_cast<const unsigned char *>(peerX.constData()), peerX.length(), bnX);
    BN_bin2bn(reinterpret_cast<const unsigned char *>(peerY.constData()), peerY.length(), bnY);

    EC_KEY* ecPeerKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!EC_KEY_set_public_key_affine_coordinates(ecPeerKey, bnX, bnY))
    {
        BN_free(bnX);
        BN_free(bnY);
    }
    BN_free(bnX);
    BN_free(bnY);

    auto peerKey = EVP_PKEY_new();
    EVP_PKEY_assign_EC_KEY(peerKey, ecPeerKey);

    /* Create the context for the shared secret derivation */
    auto ctx = EVP_PKEY_CTX_new(ourKey, nullptr);
    if (ctx == nullptr) {
        EVP_PKEY_free(peerKey);
        return {};
    }

    /* Initialise */
    if (EVP_PKEY_derive_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return {};
    }

    /* Provide the peer public key */
    if (EVP_PKEY_derive_set_peer(ctx, peerKey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return {};
    }

    /* Determine buffer length for shared secret */
    size_t secret_len;
    if (EVP_PKEY_derive(ctx, nullptr, &secret_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return {};
    }

    /* Create the buffer */
    QByteArray secret(secret_len, Qt::Uninitialized);

    /* Derive the shared secret */
    if(EVP_PKEY_derive(ctx, reinterpret_cast<unsigned char *>(secret.data()), &secret_len) != 1){
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return {};
    }

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peerKey);

    return secret;
}

QByteArray Cryptography::hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!ctx) {
        // Clean up
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    // Set HKDF parameters
    if (EVP_PKEY_derive_init(ctx) <= 0 ||
        EVP_PKEY_CTX_hkdf_mode(ctx, EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND) <= 0 ||
        EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set1_hkdf_salt(ctx, reinterpret_cast<const unsigned char *>(salt.constData()), salt.length()) <= 0 ||
        EVP_PKEY_CTX_set1_hkdf_key(ctx, reinterpret_cast<const unsigned char *>(ikm.constData()), ikm.length()) <= 0 ||
        EVP_PKEY_CTX_add1_hkdf_info(ctx, reinterpret_cast<const unsigned char *>(info.constData()), info.length()) <= 0) {
        // Clean up
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    // Perform HKDF extraction
    QByteArray outputKey(length, Qt::Uninitialized);
    if (EVP_PKEY_derive(ctx, reinterpret_cast<unsigned char *>(outputKey.data()), &length) <= 0) {
        // Clean up
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    EVP_PKEY_CTX_free(ctx);
    return outputKey;
}

QByteArray Cryptography::aes256cbcDecrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return {};
    }

    EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);

    if (EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.constData()), reinterpret_cast<const unsigned char *>(iv.constData()), 0) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    QByteArray plaintext(ciphertext.length() + 32, Qt::Uninitialized);
    int plaintextLength;
    if (EVP_CipherUpdate(ctx, reinterpret_cast<unsigned char *>(plaintext.data()), &plaintextLength, reinterpret_cast<const unsigned char *>(ciphertext.constData()), ciphertext.length()) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    auto fullPlaintextLength = plaintextLength;
    if(EVP_CipherFinal_ex(ctx, reinterpret_cast<unsigned char *>(plaintext.data() + plaintextLength), &plaintextLength) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    fullPlaintextLength += plaintextLength;

    plaintext.truncate(fullPlaintextLength);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}

QByteArray Cryptography::randomBytes(qint64 length) {
    qint64 baLength = ceil(length / 4.0) * 4;

    QByteArray bytes(baLength, Qt::Uninitialized);
    QRandomGenerator::securelySeeded().fillRange(reinterpret_cast<quint32*>(bytes.data()), baLength / 4);
    bytes.truncate(length);
    return bytes;
}

QByteArray Cryptography::hmacSha256Signature(const QByteArray &data, const QByteArray &key) {
    QMessageAuthenticationCode code(QCryptographicHash::Sha256, key);
    code.addData(data);
    return code.result();
}
