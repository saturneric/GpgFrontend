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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgContext.h"
#include "core/model/GFBuffer.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GpgImportInformation;

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyImportExporter
    : public SingletonFunctionObject<GpgKeyImportExporter> {
 public:
  /**
   * @brief Construct a new Gpg Key Import Exporter object
   *
   * @param channel
   */
  explicit GpgKeyImportExporter(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @param inBuffer
   * @return GpgImportInformation
   */
  auto ImportKey(const GFBuffer&) -> QSharedPointer<GpgImportInformation>;

  /**
   * @brief
   *
   * @param key
   * @param secret
   * @param ascii
   * @return std::tuple<GpgError, GFBuffer>
   */
  [[nodiscard]] auto ExportKey(
      const GpgAbstractKeyPtr& key, bool secret, bool ascii, bool shortest,
      bool ssh_mode = false) const -> std::tuple<GpgError, GFBuffer>;

  /**
   * @brief
   *
   * @param fpr
   * @param ascii
   * @return std::tuple<GpgError, GFBuffer>
   */
  [[nodiscard]] auto ExportSubkey(const QString& fpr, bool ascii) const
      -> std::tuple<GpgError, GFBuffer>;

  /**
   * @brief
   *
   * @param keys
   * @param outBuffer
   * @param secret
   * @return true
   * @return false
   */
  void ExportKeys(const GpgAbstractKeyPtrList& keys, bool secret, bool ascii,
                  bool shortest, bool ssh_mode,
                  const GpgOperationCallback& cb) const;

  /**
   * @brief
   *
   * @param keys
   * @param secret
   * @param ascii
   * @param cb
   */
  void ExportAllKeys(const GpgAbstractKeyPtrList& keys, bool secret, bool ascii,
                     const GpgOperationCallback& cb) const;

 private:
  GpgContext& ctx_;
};

}  // namespace GpgFrontend
