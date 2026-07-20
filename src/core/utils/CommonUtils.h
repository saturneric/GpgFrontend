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

#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Format a fingerprint string with spaces every five characters.
 *
 * @param fingerprint raw fingerprint string (hex, no spaces)
 * @return formatted fingerprint with spaces for readability
 */
auto GF_CORE_EXPORT BeautifyFingerprint(QString fingerprint) -> QString;

/**
 * @brief Compare two software version strings.
 *
 * Strips a leading 'v' prefix if present, then compares dot-separated
 * numeric components.
 *
 * @param a first version string
 * @param b second version string
 * @return 1 if a > b, -1 if a < b, 0 if equal
 */
auto GF_CORE_EXPORT GFCompareSoftwareVersion(const QString& a, const QString& b)
    -> int;

/**
 * @brief Duplicate a QString as a heap-allocated C string via SMAMalloc.
 *
 * The caller is responsible for freeing the returned pointer with SMAFree.
 *
 * @param s string to duplicate
 * @return null-terminated C string copy
 */
auto GF_CORE_EXPORT GFStrDup(const QString& s) -> char*;

/**
 * @brief Duplicate a GFBuffer as a heap-allocated byte array via SMAMalloc.
 *
 * The caller is responsible for freeing the returned pointer with SMAFree.
 *
 * @param s buffer to duplicate
 * @return heap-allocated copy of the buffer contents
 */
auto GF_CORE_EXPORT GFBufferDup(const GFBuffer& s) -> char*;

/**
 * @brief Convert a C string to a QString.
 *
 * @param s null-terminated C string; may be nullptr (returns empty string)
 * @return resulting QString
 */
auto GF_CORE_EXPORT GFUnStrDup(const char* s) -> QString;

/**
 * @brief Return whether the application is running inside a Flatpak sandbox.
 * @return true if the FLATPAK_ID environment variable is set
 */
auto GF_CORE_EXPORT IsFlatpakENV() -> bool;

/**
 * @brief Return whether the application is running as an AppImage.
 * @return true if the APPIMAGE environment variable is set
 */
auto GF_CORE_EXPORT IsAppImageENV() -> bool;

/**
 * @brief Return whether the application is running inside an app-level sandbox
 * (e.g. macOS sandbox).
 * @return true if a platform-level app sandbox is detected
 */
auto GF_CORE_EXPORT IsRunningInAppSandbox() -> bool;

/**
 * @brief Return whether the application is running inside any sandbox
 * environment.
 *
 * Covers Flatpak, AppImage, macOS sandbox, and similar.
 *
 * @return true if any sandbox is detected
 */
auto GF_CORE_EXPORT IsRunningInSandBox() -> bool;

/**
 * @brief Parse a hex-encoded version tuple string into a comparable integer.
 *
 * @param s hex-encoded version string
 * @return integer representation of the version tuple
 */
auto GF_CORE_EXPORT ParseHexEncodedVersionTuple(const QString& s) -> int;

/**
 * @brief Return whether the given string looks like a valid email address.
 *
 * @param s string to test
 * @return true if the string matches a basic email address pattern
 */
auto GF_CORE_EXPORT IsEmailAddress(const QString& s) -> bool;

/**
 * @brief Return whether version string @p a is strictly greater than @p b.
 *
 * @param a first version string
 * @param b second version string
 * @return true if a > b, false if a <= b or @p a is empty
 */
auto GF_CORE_EXPORT GFSoftwareVersionGreaterThan(const QString& a,
                                                 const QString& b) -> bool;

/**
 * @brief Return whether the core environment has been fully initialised.
 * @return true if core initialisation is complete
 */
auto GF_CORE_EXPORT IsCoreEnvInitialized() -> bool;

/**
 * @brief Ensure that the Sodium library is initialised.
 *
 * @return true if Sodium is initialised successfully or was already
 * initialised, false if
 * @return false if Sodium initialization fails
 */
auto GF_CORE_EXPORT EnsureSodiumInit() -> bool;

/**
 * @brief Get the secure level of the application from the "GFSecureLevel"
 * property of the QApplication instance.
 *
 * @return int secure level, or 0 if the property is not set or qApp is null
 */
auto GF_CORE_EXPORT SecureLevelFromApp() -> int;

/**
 * @brief Resolve a startup setting across its three layers.
 *
 * Precedence is ENV.ini override, then the user's stored value, then the
 * built-in default. An invalid QVariant means "this layer has no value", which
 * is exactly what QSettings::value() returns for a missing key — so a layer
 * that is merely absent falls through instead of overriding with an empty
 * value.
 *
 * @param env_value value from ENV.ini, or an invalid QVariant if unset
 * @param user_value value from the user settings, or invalid if unset
 * @param fallback built-in default, used when neither layer has a value
 * @return the winning value
 */
auto GF_CORE_EXPORT ResolveLayeredValue(const QVariant& env_value,
                                        const QVariant& user_value,
                                        const QVariant& fallback) -> QVariant;

}  // namespace GpgFrontend
