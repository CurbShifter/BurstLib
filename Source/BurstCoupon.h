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

#ifndef BurstCoupon_H_INCLUDED
#define BurstCoupon_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "BurstKit.h"
#include "crypto/blowfish.h"

class BurstCoupon : public BurstKit
{
public:
	BurstCoupon(String hostUrl = String::empty);
	~BurstCoupon();

	String CreateCoupon(String txSignedHex, String password);
	String RedeemCoupon(String couponHex, String password);
	String ValidateCoupon(String couponCode, String password, bool &valid);
	String DecryptCoupon(String couponHex, String password, bool validate = false);
	
private:
};

#endif
