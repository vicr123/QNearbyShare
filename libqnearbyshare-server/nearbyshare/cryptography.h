

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

#ifndef QNEARBYSHARE_CRYPTOGRAPHY_H
#define QNEARBYSHARE_CRYPTOGRAPHY_H

#include <QString>

struct EcKey;
namespace Cryptography {
    QByteArray randomBytes(qint64 length);

    EcKey* generateEcdsaKeyPair();
    void deleteEcdsaKeyPair(EcKey* key);

    QByteArray ecdsaBignumParam(EcKey* key, const char* paramName, int degree);
    QByteArray ecdsaX(EcKey* key);
    QByteArray ecdsaY(EcKey* key);

    QByteArray diffieHellman(EcKey* ourKey, const QByteArray& peerX, const QByteArray& peerY);
    QByteArray hkdfExtractExpand(const QByteArray& salt, const QByteArray& ikm, const QByteArray& info, size_t length);

    QByteArray aes256cbc(const QByteArray& input, const QByteArray& key, const QByteArray& iv, bool isEncrypt);
    QByteArray aes256cbcDecrypt(const QByteArray& ciphertext, const QByteArray& key, const QByteArray& iv);
    QByteArray aes256cbcEncrypt(const QByteArray& plaintext, const QByteArray& key, const QByteArray& iv);
    QByteArray hmacSha256Signature(const QByteArray& data, const QByteArray& key);
}; // namespace Cryptography

#endif // QNEARBYSHARE_CRYPTOGRAPHY_H
