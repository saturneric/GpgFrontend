/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

namespace GpgFrontend {

constexpr int kNonRestartCode = 0;
constexpr int kRestartCode = 1000;      ///< only refresh ui
constexpr int kDeepRestartCode = 1001;  // refresh core and ui
constexpr int kCrashCode = ~0;          ///< application crash

constexpr int kSecBufferSize = 4 * 1024;                // 4KB
constexpr int kSecBufferSizeForFile = 4 * 1024 * 1024;  // 4MB

// Channels
constexpr int kGpgFrontendDefaultChannel = 0;   ///<
constexpr int kGpgFrontendNonAsciiChannel = 2;  ///<

// HEADER
constexpr const char* PGP_CRYPT_BEGIN = "-----BEGIN PGP MESSAGE-----";  ///<
constexpr const char* PGP_CRYPT_END = "-----END PGP MESSAGE-----";      ///<
constexpr const char* PGP_SIGNED_BEGIN =
    "-----BEGIN PGP SIGNED MESSAGE-----";                              ///<
constexpr const char* PGP_SIGNED_END = "-----END PGP SIGNATURE-----";  ///<
constexpr const char* PGP_SIGNATURE_BEGIN =
    "-----BEGIN PGP SIGNATURE-----";                                      ///<
constexpr const char* PGP_SIGNATURE_END = "-----END PGP SIGNATURE-----";  ///<
constexpr const char* PGP_PUBLIC_KEY_BEGIN =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----";  ///<
constexpr const char* PGP_PRIVATE_KEY_BEGIN =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----";  ///<

// MODULE ID
constexpr const char* kEmailModuleID = "com.bktus.gpgfrontend.module.email";

}  // namespace GpgFrontend
