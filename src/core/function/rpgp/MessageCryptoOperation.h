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

namespace GpgFrontend {

/**
 * @brief
 *
 * @param ctx_
 * @param keys
 * @param in_buffer
 * @param ascii
 * @param data_object
 * @return GpgError
 */
auto EncryptRpgpImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& keys,
                     const GFBuffer& in_buffer, bool ascii,
                     const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx_
 * @param in_buffer
 * @param data_object
 * @return GpgError
 */
auto DecryptRpgpImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                     const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx
 * @param signers
 * @param in_buffer
 * @param mode
 * @param ascii
 * @param data_object
 * @return GpgError
 */
auto SignRpgpImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& signers,
                  const GFBuffer& in_buffer, GpgSignMode mode, bool ascii,
                  const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx_
 * @param in_buffer
 * @param sig_buffer
 * @param data_object
 * @return GpgError
 */
auto VerifyRpgpImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                    const GFBuffer& sig_buffer,
                    const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx
 * @param keys
 * @param signers
 * @param in_buffer
 * @param ascii
 * @param data_object
 * @return GpgError
 */
auto EncryptSignRpgpImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                         const GpgAbstractKeyPtrList& signers,
                         const GFBuffer& in_buffer, bool ascii,
                         const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx_
 * @param in_buffer
 * @param data_object
 * @return GpgError
 */
auto DecryptVerifyRpgpImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                           const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx_
 * @param in_buffer
 * @param ascii
 * @param data_object
 * @return GpgError
 */
auto EncryptSymmetricRpgpImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                              bool ascii, const DataObjectPtr& data_object)
    -> GpgError;
}  // namespace GpgFrontend