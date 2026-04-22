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

#include "core/function/gpg/GpgContext.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgImportInformation.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @param ctx
 * @param in_buffer
 * @return QSharedPointer<GpgImportInformation>
 */
auto ImportKeyGnuPGImpl(GpgContext& ctx, const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation>;

/**
 * @brief
 *
 * @param ctx
 * @param keys
 * @param secret
 * @param ascii
 * @param shortest
 * @param ssh_mode
 * @return std::tuple<GpgError, GFBuffer>
 */
auto ExportKeysGnuPGImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                         bool secret, bool ascii, bool shortest, bool ssh_mode)
    -> std::tuple<GpgError, GFBuffer>;

/**
 * @brief
 *
 * @param ctx
 * @param keys
 * @param secret
 * @param ascii
 * @param shortest
 * @param ssh_mode
 * @param data_object
 * @return GpgError
 */
auto ExportKeysAsyncGnuPGImpl(GpgContext& ctx,
                              const GpgAbstractKeyPtrList& keys, bool secret,
                              bool ascii, bool shortest, bool ssh_mode,
                              const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx
 * @param keys
 * @param secret
 * @param ascii
 * @param data_object
 * @return GpgError
 */
auto ExportAllKeysGnuPGImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                            bool secret, bool ascii,
                            const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx
 * @param fpr
 * @param ascii
 * @return std::tuple<GpgError, GFBuffer>
 */
auto ExportSubkeyGnuPGImpl(GpgContext& ctx, const QString& fpr, bool ascii)
    -> std::tuple<GpgError, GFBuffer>;
}  // namespace GpgFrontend
