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
 * @brief
 *
 * @param fingerprint
 * @return QString
 */
auto GF_CORE_EXPORT BeautifyFingerprint(QString fingerprint) -> QString;

/**
 * @brief
 *
 * @param a
 * @param b
 * @return int
 */
auto GF_CORE_EXPORT GFCompareSoftwareVersion(const QString &a, const QString &b)
    -> int;

/**
 * @brief
 *
 * @return char*
 */
auto GF_CORE_EXPORT GFStrDup(const QString &) -> char *;

/**
 * @brief
 *
 * @return char*
 */
auto GF_CORE_EXPORT GFBufferDup(const GFBuffer &s) -> char *;

/**
 * @brief
 *
 * @return QString
 */
auto GF_CORE_EXPORT GFUnStrDup(const char *) -> QString;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsFlatpakENV() -> bool;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsAppImageENV() -> bool;

/**
 * @brief The program that starts this installation again: the .AppImage file
 * inside an AppImage, @p argv0 everywhere else.
 *
 * Inside an AppImage argv[0] names the image's temporary mount, which goes
 * away with this process: a successor started from it would find nothing
 * there. Falls back to @p argv0 when $APPIMAGE names no existing file.
 */
auto GF_CORE_EXPORT RelaunchProgram(const QString &argv0) -> QString;

/**
 * @brief
 *
 * @param s
 * @return int
 */
auto GF_CORE_EXPORT ParseHexEncodedVersionTuple(const QString &s) -> int;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsEmailAddress(const QString &) -> bool;

/**
 * @brief Whether @p data holds anything but whitespace outside one signed or
 * message armor block.
 *
 * gpg verifies the armored block it finds and skips whatever surrounds it,
 * so text added before or after a genuine clear-signed message, or a second
 * block, would be shown next to a verdict that does not cover it. Input with
 * no armor at all is left to gpg.
 */
auto GF_CORE_EXPORT HasTextOutsideSignedArmor(const QByteArray &data) -> bool;

/**
 * @brief HasTextOutsideSignedArmor() for a file, read a line at a time.
 *
 * @param path the file about to be verified; false when it cannot be read,
 *        which the verification itself then reports
 */
auto GF_CORE_EXPORT HasTextOutsideSignedArmorInFile(const QString &path)
    -> bool;

/**
 * @brief if a > b
 *
 * @param a
 * @param b
 * @return auto
 */
auto GF_CORE_EXPORT GFSoftwareVersionGreaterThan(const QString &a,
                                                 const QString &b) -> bool;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsCoreEnvInitialized() -> bool;

/**
 * @brief Return whether the application is running inside any sandbox
 * environment.
 *
 * Covers Flatpak, AppImage, macOS sandbox, and similar.
 *
 * @return true if any sandbox is detected
 */
auto GF_CORE_EXPORT IsRunningInSandBox() -> bool;

}  // namespace GpgFrontend