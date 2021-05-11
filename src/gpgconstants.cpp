/*
 *      gpgconstants.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "gpgconstants.h"
#include <QString>

const char* GpgConstants::PGP_CRYPT_BEGIN = "-----BEGIN PGP MESSAGE-----";
const char* GpgConstants::PGP_CRYPT_END = "-----END PGP MESSAGE-----";
const char* GpgConstants::PGP_SIGNED_BEGIN = "-----BEGIN PGP SIGNED MESSAGE-----";
const char* GpgConstants::PGP_SIGNED_END = "-----END PGP SIGNATURE-----";
const char* GpgConstants::PGP_SIGNATURE_BEGIN = "-----BEGIN PGP SIGNATURE-----";
const char* GpgConstants::PGP_SIGNATURE_END = "-----END PGP SIGNATURE-----";

