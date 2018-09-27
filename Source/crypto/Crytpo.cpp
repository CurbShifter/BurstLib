//
//  Crytpo.cpp
//  Burstcoin
/*
Copyright (C) 2018  CurbShifter

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
//  
//  based on .m version by Andy Prock
//  https://github.com/aprock/BurstKit/blob/master/BurstKit/Crypto/Crytpo.m
//
//  https://github.com/PoC-Consortium/burstcoin/blob/master/src/brs/crypto/Crypto.java

#include "Crypto.hpp"

extern "C" {
#include "curve25519_i64.h"
}
#include "sha256.h"
#include "aes.hpp" // https://github.com/kokke/tiny-AES-c

#define ECCKeyLength 32
#define ECCSignatureLength 64

void Crypto::getPublicKey(MemoryBlock passPhrase, MemoryBlock &publicKey)
{
	publicKey.ensureSize(ECCKeyLength, true);
	const MemoryBlock hash = sha256(passPhrase);
	keygen25519((unsigned char *)publicKey.getData(), nullptr, (unsigned char *)hash.getData());
}

void Crypto::getPrivateKey(MemoryBlock passPhrase, MemoryBlock &privateKey)
{
	privateKey = sha256(passPhrase);

	if (privateKey.getSize() > 31)
	{
		((char*)privateKey.getData())[31] &= 0x7F;
		((char*)privateKey.getData())[31] |= 0x40;
		((char*)privateKey.getData())[0] &= 0xF8;
	}
}

MemoryBlock Crypto::sha256(MemoryBlock data1, MemoryBlock data2)
{
	//	SHA256 sha1(passPhrase.getData(), passPhrase.getSize());
	//	MemoryBlock k = sha1.getRawData();
	SHA256_CTX ctx0;
	sha256_init(&ctx0);
	if (data1.getSize() > 0)
		sha256_update(&ctx0, (BYTE*)data1.getData(), data1.getSize());
	if (data2.getSize() > 0)
		sha256_update(&ctx0, (BYTE*)data2.getData(), data2.getSize());
	MemoryBlock k(ECCKeyLength, true);
	sha256_final(&ctx0, (BYTE*)k.getData());
	return k;
}

// deterministic EC-KCDSA
MemoryBlock Crypto::sign(MemoryBlock data, MemoryBlock passPhrase)
{ 
	MemoryBlock P(ECCKeyLength, true);
	MemoryBlock s(ECCKeyLength, true);

	MemoryBlock k = sha256(passPhrase);
	keygen25519((unsigned char *)P.getData(), (unsigned char *)s.getData(), (unsigned char *)k.getData());

	const MemoryBlock m = sha256(data);
	const MemoryBlock x = sha256(m, s);

	MemoryBlock Y(ECCKeyLength, true);
	keygen25519((unsigned char *)Y.getData(), nullptr, (unsigned char *)x.getData());

	const MemoryBlock h = sha256(m, Y);

	MemoryBlock v(ECCKeyLength, true);
	bool success = sign25519((unsigned char *)v.getData(), (unsigned char *)h.getData(), (unsigned char *)x.getData(), (unsigned char *)s.getData()) > 0;

	MemoryBlock signature;
	signature.append(v.getData(), ECCKeyLength);
	signature.append(h.getData(), ECCKeyLength);

	return signature;
}

bool Crypto::verify(MemoryBlock signature, MemoryBlock pubKey, MemoryBlock data)
{
	MemoryBlock Y(ECCKeyLength, true); 
	MemoryBlock v(signature.getData(), ECCKeyLength);
	MemoryBlock h(&((char*)signature.getData())[ECCKeyLength], ECCKeyLength);

	verify25519((unsigned char *)Y.getData(), (unsigned char *)v.getData(), (unsigned char *)h.getData(), (unsigned char *)pubKey.getData());
	
	const MemoryBlock datahash = sha256(data);
	const MemoryBlock h2 = sha256(datahash, Y);

	return h2.matches(h.getData(), h.getSize());
}



/* The RFC that contains the PKCS#7 standard is the same except that it allows block sizes up to 255 bytes in size (10.3 note 2):
For such algorithms, the method shall be to pad the input at the trailing end with k - (l mod k) octets all having value k - (l mod k), where l is the length of the input.
PKCS#5 and PKCS#7 - PKCS#7 is described in RFC 5652.
Padding is in whole bytes. The value of each added byte is the number of bytes that are added, i.e. N bytes, each of value N are added. 
The number of bytes added will depend on the block boundary to which the message needs to be extended.	The padding will be one of:
01
02 02
03 03 03
...	
 */
#define IV_SIZE 16

MemoryBlock Crypto::getSharedSecret(MemoryBlock myPrivateKey, MemoryBlock theirPublicKey)
{
	MemoryBlock sharedSecret(ECCKeyLength, true);
	curve25519((unsigned char *)sharedSecret.getData(), (unsigned char *)myPrivateKey.getData(), (unsigned char *)theirPublicKey.getData());
	return sharedSecret;
}

MemoryBlock Crypto::aesEncrypt(MemoryBlock plainText, MemoryBlock myPrivateKey, MemoryBlock theirPublicKey, MemoryBlock &nonce)
{
	nonce.ensureSize(ECCKeyLength, true);
	juce::Random random;
	random.fillBitsRandomly(nonce.getData(), ECCKeyLength);

	MemoryBlock sharedSecret = getSharedSecret(myPrivateKey, theirPublicKey);
	unsigned char *sharedSecretPtr = (unsigned char *)sharedSecret.getData();
	unsigned char *noncePtr = (unsigned char *)nonce.getData();
	for (int i = 0; i < ECCKeyLength && i < sharedSecret.getSize(); i++)
		sharedSecretPtr[i] ^= noncePtr[i];
	
	MemoryBlock iv(IV_SIZE);
	random.fillBitsRandomly(iv.getData(), IV_SIZE);
	
	// pad the plain to a multiple of AES_BLOCKLEN
	unsigned char addedBytes = (AES_BLOCKLEN - (plainText.getSize() % AES_BLOCKLEN));
	if (addedBytes > 0)
		plainText.ensureSize(plainText.getSize() + addedBytes, true);
	for (int i = 0; i < addedBytes; i++) // store the amount of padding in each padded byte
		plainText.copyFrom(&addedBytes, (plainText.getSize()-1) - i, 1);
	
	MemoryBlock key = sha256(sharedSecret);

	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, (uint8_t *)key.getData(), (uint8_t *)iv.getData());
	AES_CBC_encrypt_buffer(&ctx, (uint8_t *)plainText.getData(), plainText.getSize());

	MemoryBlock result;
	result.append(iv.getData(), iv.getSize());
	result.append(plainText.getData(), plainText.getSize());
	
	return result;
}

MemoryBlock Crypto::aesDecrypt(MemoryBlock ivCiphertext, MemoryBlock myPrivateKey, MemoryBlock theirPublicKey, MemoryBlock nonce)
{
	const int ivSize = IV_SIZE;
	MemoryBlock iv(ivCiphertext.getData(), ivSize);
	ivCiphertext.removeSection(0, ivSize);
	
	MemoryBlock dhSharedSecret = getSharedSecret(myPrivateKey, theirPublicKey);
	nonce.ensureSize(ECCKeyLength, true); // no mem overrun
	unsigned char *sharedSecretPtr = (unsigned char *)dhSharedSecret.getData();
	unsigned char *noncePtr = (unsigned char *)nonce.getData();
	for (int i = 0; i < ECCKeyLength && i < dhSharedSecret.getSize(); i++)
		sharedSecretPtr[i] ^= noncePtr[i];
	
	MemoryBlock key = sha256(dhSharedSecret);

	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, (uint8_t *)key.getData(), (uint8_t *)iv.getData());
	AES_CBC_decrypt_buffer(&ctx, (uint8_t *)ivCiphertext.getData(), ivCiphertext.getSize());

	MemoryBlock plainData(ivCiphertext);
	if (plainData.getSize() > 0)
	{ // remove padding if needed
		const unsigned char padSize = ((unsigned char*)plainData.getData())[plainData.getSize() - 1];	
		if (padSize < jmin<int>(plainData.getSize(), 0xff))
		{
			bool padCheck = true;
			for (int i = 1; i < padSize; i++)
			{ // check padding bytes
				const unsigned char padFound = ((unsigned char*)plainData.getData())[(plainData.getSize() - 1) - i];
				if (padSize != padFound)
					padCheck = false;
			}
			if (padCheck)
				plainData.setSize(plainData.getSize() - padSize);
		}
	}
	return plainData;
}
