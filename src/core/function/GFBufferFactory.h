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

#include "core/function/PassphraseGenerator.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GFBufferFactory
    : public SingletonFunctionObject<GFBufferFactory> {
 public:
  explicit GFBufferFactory(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  static auto FromFile(const QString& path) -> GFBufferOrNone;

  static auto ToFile(const QString& path, const GFBuffer& buffer) -> bool;

  static auto ToBase64(const GFBuffer& buffer) -> GFBufferOrNone;

  static auto FromBase64(const GFBuffer& buffer) -> GFBufferOrNone;

  auto Compress(const GFBuffer& buffer) -> GFBufferOrNone;

  auto Decompress(const GFBuffer& buffer) -> GFBufferOrNone;

  static auto Encrypt(const GFBuffer& passphase, const GFBuffer& buffer)
      -> GFBufferOrNone;

  static auto Decrypt(const GFBuffer& passphase, const GFBuffer& buffer)
      -> GFBufferOrNone;

  static auto RandomOpenSSLPassphase(int len) -> GFBufferOrNone;

  auto RandomGpgPassphase(int len) -> GFBufferOrNone;

  auto RandomGpgZBasePassphase(int len) -> GFBufferOrNone;

  auto QString(const GFBuffer& buffer) -> QString;

  auto QByteArray(const GFBuffer& buffer) -> QByteArray;

 private:
  PassphraseGenerator& ph_gen_ = PassphraseGenerator::GetInstance(GetChannel());
};

}  // namespace GpgFrontend