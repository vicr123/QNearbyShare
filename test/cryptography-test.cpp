

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
#include "gtest/gtest.h"

#include <openssl/ec.h>

TEST(crypto, aes256decrypt) {
    QByteArray expected("HELLO WORLD");
    QByteArray iv("AABBCCDDEEFFGGHH");
    QByteArray key("SECRETKEY1234567SECRETKEY1234567");
    QByteArray ciphertext = QByteArray::fromHex("240252C8656EED9FD468E75ECBD202CA");

    auto decrypted = Cryptography::aes256cbcDecrypt(ciphertext, key, iv);
    EXPECT_EQ(decrypted, expected);
}

TEST(crypto, aes256encrypt) {
    QByteArray expected = QByteArray::fromHex("240252C8656EED9FD468E75ECBD202CA");
    QByteArray iv("AABBCCDDEEFFGGHH");
    QByteArray key("SECRETKEY1234567SECRETKEY1234567");
    QByteArray plaintext("HELLO WORLD");

    auto decrypted = Cryptography::aes256cbcEncrypt(plaintext, key, iv);
    EXPECT_EQ(decrypted, expected);
}

// TEST(crypto, diffiehellman) {
//     auto derEncodedPrivateKey = QByteArray::fromBase64("MHcCAQEEIIE6X8aWH3uBtT6B9UJwyiaYu/AXa9TjzJ8yoekVqQSxoAoGCCqGSM49AwEHoUQDQgAEnhO4cWozhGQwjZ4Zhi1xAL06gyhN9qvpwt6ClFESTux+XW2/aL8iKl8Qug7mwTCIK76tEECLjWxxnbxYL22tmA==");
//     auto clientX = QByteArray::fromBase64("T8fj9vVlKUvtQXWW+jkW2e9feqbw6IUumnFYq7eo1Pw=");
//     auto clientY = QByteArray::fromBase64("Tc193jvBB2IAv4OBaxRBTrQPbzjq/Gi8e52xDKZOdNY=");
//     auto expected = QByteArray::fromBase64("aXRe7hj54EfvU9/yEUDZcrDeh3afEDFZSWg6tK84Glo=");
//
//     auto key = EC_KEY_new();
//     auto derEncodedData = derEncodedPrivateKey.constData();
//     d2i_ECPrivateKey(&key, reinterpret_cast<const unsigned char**>(&derEncodedData), derEncodedPrivateKey.length());
//
//     auto evpKey = EVP_PKEY_new();
//     EVP_PKEY_set1_EC_KEY(evpKey, key);
//
//     auto dhs = Cryptography::diffieHellman(evpKey, clientX, clientY);
//     EXPECT_EQ(dhs, expected);
// }

TEST(crypto, random) {
    auto bytes = Cryptography::randomBytes(6);
    EXPECT_EQ(bytes.length(), 6);
}
