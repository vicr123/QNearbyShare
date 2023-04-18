//
// Created by victor on 16/04/23.
//

#include "cryptography.h"
#include <QRandomGenerator>
#include <QMessageAuthenticationCode>
#include <QTextStream>

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
QByteArray Cryptography::aes256cbcDecrypt(const QByteArray &ciphertext, const QByteArray &key, const QByteArray &iv) {
    return aes256cbc(ciphertext, key, iv, false);
}

QByteArray Cryptography::aes256cbcEncrypt(const QByteArray &plaintext, const QByteArray &key, const QByteArray &iv) {
    return aes256cbc(plaintext, key, iv, true);
}
