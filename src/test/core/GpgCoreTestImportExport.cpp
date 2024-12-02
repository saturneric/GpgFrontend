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

#include "GpgCoreTest.h"
#include "core/GpgConstants.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreExportSubkeyTestA) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "F89C95A05088CC93", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestB) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "F89C95A05088CC93!", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestC) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestD) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93!", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

}  // namespace GpgFrontend::Test