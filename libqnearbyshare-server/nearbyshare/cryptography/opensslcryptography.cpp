

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

#include "nearbyshare/cryptography.h"

#include <QTextStream>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

namespace OpenSSLSupport {
    QByteArray bignumToBytes(BIGNUM* bn);
    BIGNUM* bytesToBignum(QByteArray ba);
    QByteArray ecdsaBignumParam(EcKey* key, const char* paramName);
} // namespace OpenSSLSupport

struct EcKey {
        EVP_PKEY* key;
};

EcKey* Cryptography::generateEcdsaKeyPair() {
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
    return new EcKey{clientKey};
}

QByteArray OpenSSLSupport::ecdsaBignumParam(EcKey* key, const char* paramName) {
    auto n = BN_new();
    auto ok = EVP_PKEY_get_bn_param(key->key, paramName, &n);
    if (!ok) {
        BN_free(n);
        return {};
    }

    //    QByteArray nBytes(degree / 8, Qt::Uninitialized);

    auto nBytes = OpenSSLSupport::bignumToBytes(n);
    BN_free(n);
    return nBytes;
}

QByteArray Cryptography::ecdsaX(EcKey* key) {
    return OpenSSLSupport::ecdsaBignumParam(key, OSSL_PKEY_PARAM_EC_PUB_X);
}

QByteArray Cryptography::ecdsaY(EcKey* key) {
    return OpenSSLSupport::ecdsaBignumParam(key, OSSL_PKEY_PARAM_EC_PUB_Y);
}

QByteArray Cryptography::diffieHellman(EcKey* ourKey, QByteArray peerX, QByteArray peerY) {
    auto bnX = OpenSSLSupport::bytesToBignum(std::move(peerX));
    auto bnY = OpenSSLSupport::bytesToBignum(std::move(peerY));

    EC_KEY* ecPeerKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!EC_KEY_set_public_key_affine_coordinates(ecPeerKey, bnX, bnY)) {
        BN_free(bnX);
        BN_free(bnY);
        return {};
    }
    BN_free(bnX);
    BN_free(bnY);

    auto peerKey = EVP_PKEY_new();
    EVP_PKEY_assign_EC_KEY(peerKey, ecPeerKey);

    /* Create the context for the shared secret derivation */
    auto ctx = EVP_PKEY_CTX_new(ourKey->key, nullptr);
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
    size_t secretLen;
    if (EVP_PKEY_derive(ctx, nullptr, &secretLen) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return {};
    }

    /* Create the buffer */
    QByteArray secret(secretLen, Qt::Uninitialized);

    /* Derive the shared secret */
    if (EVP_PKEY_derive(ctx, reinterpret_cast<unsigned char*>(secret.data()), &secretLen) != 1) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return {};
    }

    secret.truncate(secretLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peerKey);

    return secret;
}

QByteArray Cryptography::hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!ctx) {
        // Clean up
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    // Set HKDF parameters
    if (EVP_PKEY_derive_init(ctx) <= 0 ||
        EVP_PKEY_CTX_hkdf_mode(ctx, EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND) <= 0 ||
        EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set1_hkdf_salt(ctx, reinterpret_cast<const unsigned char*>(salt.constData()), salt.length()) <= 0 ||
        EVP_PKEY_CTX_set1_hkdf_key(ctx, reinterpret_cast<const unsigned char*>(ikm.constData()), ikm.length()) <= 0 ||
        EVP_PKEY_CTX_add1_hkdf_info(ctx, reinterpret_cast<const unsigned char*>(info.constData()), info.length()) <= 0) {
        // Clean up
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    // Perform HKDF extraction
    QByteArray outputKey(length, Qt::Uninitialized);
    if (EVP_PKEY_derive(ctx, reinterpret_cast<unsigned char*>(outputKey.data()), &length) <= 0) {
        // Clean up
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    EVP_PKEY_CTX_free(ctx);
    return outputKey;
}

QByteArray Cryptography::aes256cbc(const QByteArray& input, const QByteArray& key, const QByteArray& iv, bool isEncrypt) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return {};
    }

    EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);

    if (EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key.constData()), reinterpret_cast<const unsigned char*>(iv.constData()), isEncrypt) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    QByteArray output(input.length() + 32, Qt::Uninitialized);
    int outputLength;
    if (EVP_CipherUpdate(ctx, reinterpret_cast<unsigned char*>(output.data()), &outputLength, reinterpret_cast<const unsigned char*>(input.constData()), input.length()) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    auto fullOutputLength = outputLength;
    if (EVP_CipherFinal_ex(ctx, reinterpret_cast<unsigned char*>(output.data() + outputLength), &outputLength) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    fullOutputLength += outputLength;

    output.truncate(fullOutputLength);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return output;
}

void Cryptography::deleteEcdsaKeyPair(EcKey* key) {
    delete key;
}

QByteArray OpenSSLSupport::bignumToBytes(BIGNUM* bn) {
    auto bnBytes = BN_num_bytes(bn);
    QByteArray numData(bnBytes, Qt::Uninitialized);
    BN_bn2bin(bn, reinterpret_cast<unsigned char*>(numData.data()));

    auto hexRep = BN_bn2hex(bn);
    auto isNegative = hexRep[0] == '-';
    if (isNegative) {
        QTextStream(stderr) << "Conversion to bytes: BIGNUM was negative";
    }

    /*
     * 0000 0000 -> 0
     * 0000 0001 -> 1
     * 0000 0010 -> 2
     * 0000 0011 -> 3
     * 0000 0100 -> 4
     * 0000 0101 -> 5
     * 0000 0110 -> 6
     * 0000 0111 -> 7
     * 0000 1000 -> 8
     * 1001 -> 1000 -> 0111 -> -7
     */

    // Pad with 0 if not already padded
    numData.prepend('\0');
    //    if (numData.at(0) & 80) {
    //    }

    return numData;
}

BIGNUM* OpenSSLSupport::bytesToBignum(QByteArray ba) {
    if (ba[0] & 0x80) {
        QTextStream(stderr) << "Conversion to BIGNUM: BIGNUM was negative";
    }

    auto bn = BN_bin2bn(reinterpret_cast<const unsigned char*>(ba.constData()), ba.length(), nullptr);
    return bn;
}
