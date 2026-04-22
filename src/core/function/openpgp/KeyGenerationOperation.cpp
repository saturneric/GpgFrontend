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

#include "KeyGenerationOperation.h"

#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/KeyGenerationTraits.h"
#include "core/model/DataObject.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

KeyGenerationOperation::KeyGenerationOperation(int channel)
    : SingletonFunctionObject<KeyGenerationOperation>(channel) {}

void KeyGenerationOperation::GenerateKey(
    const QSharedPointer<KeyGenerateInfo>& params,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<GenerateKeyTag>(ctx_, cb, params);
}

auto KeyGenerationOperation::GenerateKeySync(
    const QSharedPointer<KeyGenerateInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<GenerateKeyTag>(ctx_, params);
}

void KeyGenerationOperation::GenerateSubkey(
    const GpgKeyPtr& key, const QSharedPointer<KeyGenerateInfo>& params,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<GenerateSubKeyTag>(ctx_, cb, key, params);
}

auto KeyGenerationOperation::GenerateSubkeySync(
    const GpgKeyPtr& key, const QSharedPointer<KeyGenerateInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<GenerateSubKeyTag>(ctx_, key, params);
}

void KeyGenerationOperation::GenerateKeyWithSubkey(
    const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<GenerateKeyWithSubKeyTag>(ctx_, cb, p_params, s_params);
}

auto KeyGenerationOperation::GenerateKeyWithSubkeySync(
    const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<GenerateKeyWithSubKeyTag>(ctx_, p_params, s_params);
}

}  // namespace GpgFrontend
