

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
