/*
Copyright (C) 2019  CurbShifter

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

#include "BurstCoupon.h"

BurstCoupon::BurstCoupon(String hostUrl) : BurstKit(hostUrl)
{
}

BurstCoupon::~BurstCoupon()
{
}

String BurstCoupon::CreateCoupon(String txSignedHex, String password)
{
	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	int retry = 10;
	String base64coupon;
	while (txSignedHex.length() >= (176 * 2) && base64coupon.isEmpty() && --retry > 0)
	{
		MemoryBlock txSigned;
		txSigned.loadFromHexString(txSignedHex);
				
		int length = 64;
		int newlength = 0; // 80
		byte *encBytes = bf.Encrypt_CBC(&((byte *)txSigned.getData())[96], length, &newlength);// only encrypt the 64 signature bytes. 96 to 160

		// cut out signature
		MemoryBlock txSignedEncrypted(&((char *)txSigned.getData())[0], 96); // pre sign
		txSignedEncrypted.append(&((char *)txSigned.getData())[160], txSigned.getSize() - 160); // post sign
		// add encrypted signature at back
		txSignedEncrypted.append(encBytes, newlength);

		base64coupon = toBase64Encoding(txSignedEncrypted);

	//	bool plainTestInvalid = true; // should fail
	//	String plainTest = ValidateCoupon(base64coupon, "WRONG_PASSWORD", plainTestInvalid);

		bool valid = false;
		String details = ValidateCoupon(base64coupon, password, valid);

		if (!valid) // erase if code is not valid
		{
			Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 500);
			base64coupon.clear();
		}

		delete[] encBytes;
	}
	return base64coupon;
}

String BurstCoupon::RedeemCoupon(String couponCode, String password)
{
	String txHex = DecryptCoupon(couponCode, password);
	if (txHex.length() >= 176 * 2 && txHex.containsOnly("ABCDEFabcdef0123456789"))
	{
		String result = broadcastTransaction(txHex);		
		String transactionID = GetJSONvalue(result, "transaction");// result["transaction"];
		return transactionID;
	}
	return String::empty;
}

String BurstCoupon::ValidateCoupon(String couponCode, String password, bool &valid)
{
	String txHex = DecryptCoupon(couponCode, password, true);
	if (txHex.length() >= 176 * 2 && txHex.containsOnly("ABCDEFabcdef0123456789"))
	{
		String txDetail = parseTransaction(txHex, "");
		String verify = GetJSONvalue(txDetail, "verify");
		valid = verify.getIntValue() > 0 || verify.compareIgnoreCase("true") == 0;
		return txDetail;
	}
	return String::empty;
}

String BurstCoupon::DecryptCoupon(String couponCode, String password, bool validate)
{
	String txHex;
	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	MemoryBlock	couponCodeBin = fromBase64Encoding(couponCode.toStdString());
	MemoryBlock	txBin(couponCodeBin.getData(), couponCodeBin.getSize() - 80);
	if (couponCodeBin.getSize() >= 176)
	{
		int length = 80;// couponCodeBin.getSize() - (176 - 64);
		int newlength = 0;
		byte *decBytes = bf.Decrypt_CBC(&((byte *)couponCodeBin.getData())[couponCodeBin.getSize() - 80], length, &newlength);

		if (newlength >= 64) // size of signature
		{
			txBin.insert(decBytes, 64, 96); // insert decrypted signature
			txHex = String::toHexString(txBin.getData(), txBin.getSize(), 0);
		}
		else if (!validate)
		{ // for data validation w/o sign
			MemoryBlock emptySign(64, true);
			txBin.insert(emptySign.getData(), 64, 96);
			txHex = String::toHexString(txBin.getData(), txBin.getSize(), 0);
		}
		delete[] decBytes;
	}
	return txHex;
}
