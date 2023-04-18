//
// Created by victor on 18/04/23.
//

#include "../cryptography.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/rijndael.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>
#include <cryptopp/hkdf.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>

using namespace CryptoPP;

namespace CryptoPPSupport {

    QByteArray transform(StreamTransformationFilter *stf, const QByteArray& input);
}

struct EcKey {
    SecByteBlock sk, pk;
};

EcKey *Cryptography::generateEcdsaKeyPair() {
    AutoSeededRandomPool prng;
    ECDH<ECP>::Domain ecdh((ASN1::secp256r1()));

    SecByteBlock sk(ecdh.PrivateKeyLength());
    SecByteBlock pk(ecdh.PublicKeyLength());
    ecdh.GenerateKeyPair(prng, sk, pk);

    return new EcKey{sk, pk};
}

QByteArray Cryptography::ecdsaX(EcKey *key) {
    DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());
    auto element = params.DecodeElement(key->pk, false);

    QByteArray xBa(element.x.MinEncodedSize(Integer::SIGNED), Qt::Uninitialized);
    element.x.Encode(reinterpret_cast<byte *>(xBa.data()), xBa.size(), Integer::SIGNED);

    return xBa;
}

QByteArray Cryptography::ecdsaY(EcKey *key) {
    DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());
    auto element = params.DecodeElement(key->pk, false);

    QByteArray yBa(element.y.MinEncodedSize(Integer::SIGNED), Qt::Uninitialized);
    element.y.Encode(reinterpret_cast<byte *>(yBa.data()), yBa.size(), Integer::SIGNED);

    return yBa;
}

QByteArray Cryptography::diffieHellman(EcKey *ourKey, QByteArray peerX, QByteArray peerY) {
    AutoSeededRandomPool prng;
    ECDH<ECP>::Domain ecdh((ASN1::secp256r1()));
    DL_GroupParameters_EC<ECP> params(ASN1::secp256r1());

    Integer x, y;
    x.Decode(reinterpret_cast<const byte *>(peerX.data()), peerX.size(), Integer::SIGNED);
    y.Decode(reinterpret_cast<const byte *>(peerY.data()), peerY.size(), Integer::SIGNED);

    QByteArray otherPk(params.GetEncodedElementSize(true), Qt::Uninitialized);
    params.EncodeElement(true, ECP::Point(x, y), reinterpret_cast<byte *>(otherPk.data()));

    ECP::Point q(x, y);
    SecByteBlock output(ecdh.AgreedValueLength());
    ecdh.Agree(output, ourKey->sk, reinterpret_cast<const byte *>(otherPk.constData()));

    return {reinterpret_cast<const char *>(output.data()), static_cast<qsizetype>(output.size())};
}

QByteArray Cryptography::hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length) {
    HKDF<SHA256> hkdf;

    QByteArray output(length, Qt::Uninitialized);
    hkdf.DeriveKey(reinterpret_cast<byte *>(output.data()), output.size(), reinterpret_cast<const byte *>(ikm.constData()), ikm.size(), reinterpret_cast<const byte *>(salt.constData()), salt.size(), reinterpret_cast<const byte *>(info.constData()), info.length());

    return output;
}

QByteArray Cryptography::aes256cbc(const QByteArray &input, const QByteArray &key, const QByteArray &iv, bool isEncrypt) {
    if (isEncrypt) {
        CBC_Mode<AES>::Encryption e;
        e.SetKeyWithIV(reinterpret_cast<const byte *>(key.constData()), key.length(), reinterpret_cast<const byte *>(iv.constData()), iv.length());
        StreamTransformationFilter stf(e, nullptr, CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
        return CryptoPPSupport::transform(&stf, input);
    } else {
        CBC_Mode<AES>::Decryption d;
        d.SetKeyWithIV(reinterpret_cast<const byte *>(key.constData()), key.length(), reinterpret_cast<const byte *>(iv.constData()), iv.length());
        StreamTransformationFilter stf(d, nullptr, CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
        return CryptoPPSupport::transform(&stf, input);
    }
}

QByteArray CryptoPPSupport::transform(StreamTransformationFilter*stf, const QByteArray& input) {
    for (char i : input) {
        stf->Put(i);
    }
    stf->MessageEnd();

    QByteArray output(stf->MaxRetrievable(), Qt::Uninitialized);
    stf->Get(reinterpret_cast<byte *>(output.data()), output.size());

    return output;
}

void Cryptography::deleteEcdsaKeyPair(EcKey *key) {
    delete key;
}

