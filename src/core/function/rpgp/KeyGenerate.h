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

#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgKeyGenerateInfo.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @param kie
 * @param p_params
 * @param s_params
 * @param data_object
 * @return GpgError
 */
auto GenerateKeyWithSubkeyRpgpImpl(
    OpenPGPContext& ctx, const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param kie
 * @param params
 * @param data_object
 * @return GpgError
 */
auto GenerateKeyRpgpImpl(OpenPGPContext& ctx,
                         const QSharedPointer<KeyGenerateInfo>& params,
                         const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param kie
 * @param primary_key_id
 * @param params
 * @param data_object
 * @return GpgError
 */
auto GenerateSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                            const QSharedPointer<KeyGenerateInfo>& params,
                            const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief
 *
 * @param ctx
 * @param key
 * @return QContainer<KeyAlgo>
 */
auto FilterKeyAlgoByKeyRpgpImpl(OpenPGPContext& ctx, const GpgKey& key,
                                const QContainer<KeyAlgo>& algos)
    -> QContainer<KeyAlgo>;
}  // namespace GpgFrontend