

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

#include "../cryptography.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/filters.h>
#include <cryptopp/hkdf.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/rijndael.h>

using namespace CryptoPP;

namespace CryptoPPSupport {

    QByteArray transform(StreamTransformationFilter* stf, const QByteArray& input);
}

struct EcKey {
        SecByteBlock sk, pk;
};

EcKey* Cryptography::generateEcdsaKeyPair() {
    AutoSeededRandomPool prng;
    ECDH<ECP>::Domain ecdh((ASN1::secp256r1()));

    SecByteBlock sk(ecdh.PrivateKeyLength());
    SecByteBlock pk(ecdh.PublicKeyLength());

    do {
        ecdh.GenerateKeyPair(prng, sk, pk);

        DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());
        auto element = params.DecodeElement(pk, false);

        // Android doesn't like it when the X and Y points are negative for some reason
        if (!element.x.IsNegative() && !element.y.IsNegative()) {
            return new EcKey{sk, pk};
        }

    } while (true);
}

QByteArray Cryptography::ecdsaX(EcKey* key) {
    DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());
    auto element = params.DecodeElement(key->pk, false);

    QByteArray xBa(element.x.MinEncodedSize(Integer::SIGNED), Qt::Uninitialized);
    element.x.Encode(reinterpret_cast<byte*>(xBa.data()), xBa.size(), Integer::SIGNED);

    return xBa;
}

QByteArray Cryptography::ecdsaY(EcKey* key) {
    DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());
    auto element = params.DecodeElement(key->pk, false);

    QByteArray yBa(element.y.MinEncodedSize(Integer::SIGNED), Qt::Uninitialized);
    element.y.Encode(reinterpret_cast<byte*>(yBa.data()), yBa.size(), Integer::SIGNED);

    return yBa;
}

QByteArray Cryptography::diffieHellman(EcKey* ourKey, QByteArray peerX, QByteArray peerY) {
    AutoSeededRandomPool prng;
    ECDH<ECP>::Domain ecdh((ASN1::secp256r1()));
    DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());

    Integer x, y;
    x.Decode(reinterpret_cast<const byte*>(peerX.data()), peerX.size(), Integer::SIGNED);
    y.Decode(reinterpret_cast<const byte*>(peerY.data()), peerY.size(), Integer::SIGNED);

    QByteArray otherPk(params.GetEncodedElementSize(true), Qt::Uninitialized);
    params.EncodeElement(true, ECP::Point(x, y), reinterpret_cast<byte*>(otherPk.data()));

    ECP::Point q(x, y);
    SecByteBlock output(ecdh.AgreedValueLength());
    ecdh.Agree(output, ourKey->sk, reinterpret_cast<const byte*>(otherPk.constData()));

    return {reinterpret_cast<const char*>(output.data()), static_cast<qsizetype>(output.size())};
}

QByteArray Cryptography::hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length) {
    HKDF<SHA256> hkdf;

    QByteArray output(length, Qt::Uninitialized);
    hkdf.DeriveKey(reinterpret_cast<byte*>(output.data()), output.size(), reinterpret_cast<const byte*>(ikm.constData()), ikm.size(), reinterpret_cast<const byte*>(salt.constData()), salt.size(), reinterpret_cast<const byte*>(info.constData()), info.length());

    return output;
}

QByteArray Cryptography::aes256cbc(const QByteArray& input, const QByteArray& key, const QByteArray& iv, bool isEncrypt) {
    if (isEncrypt) {
        CBC_Mode<AES>::Encryption e;
        e.SetKeyWithIV(reinterpret_cast<const byte*>(key.constData()), key.length(), reinterpret_cast<const byte*>(iv.constData()), iv.length());
        StreamTransformationFilter stf(e, nullptr, CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
        return CryptoPPSupport::transform(&stf, input);
    } else {
        CBC_Mode<AES>::Decryption d;
        d.SetKeyWithIV(reinterpret_cast<const byte*>(key.constData()), key.length(), reinterpret_cast<const byte*>(iv.constData()), iv.length());
        StreamTransformationFilter stf(d, nullptr, CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
        return CryptoPPSupport::transform(&stf, input);
    }
}

QByteArray CryptoPPSupport::transform(StreamTransformationFilter* stf, const QByteArray& input) {
    for (char i : input) {
        stf->Put(i);
    }
    stf->MessageEnd();

    QByteArray output(stf->MaxRetrievable(), Qt::Uninitialized);
    stf->Get(reinterpret_cast<byte*>(output.data()), output.size());

    return output;
}

void Cryptography::deleteEcdsaKeyPair(EcKey* key) {
    delete key;
}
