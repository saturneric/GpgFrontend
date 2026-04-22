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

#include "GpgKeyGenerateOpera.h"

#include "core/function/openpgp/Async.h"
#include "core/function/openpgp/traits/KeyGenerateTraits.h"
#include "core/model/DataObject.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

GpgKeyGenerateOpera::GpgKeyGenerateOpera(int channel)
    : SingletonFunctionObject<GpgKeyGenerateOpera>(channel) {}

void GpgKeyGenerateOpera::GenerateKey(
    const QSharedPointer<KeyGenerateInfo>& params,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<GenerateKeyTag>(ctx_, cb, params);
}

auto GpgKeyGenerateOpera::GenerateKeySync(
    const QSharedPointer<KeyGenerateInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<GenerateKeyTag>(ctx_, params);
}

void GpgKeyGenerateOpera::GenerateSubkey(
    const GpgKeyPtr& key, const QSharedPointer<KeyGenerateInfo>& params,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<GenerateSubKeyTag>(ctx_, cb, key, params);
}

auto GpgKeyGenerateOpera::GenerateSubkeySync(
    const GpgKeyPtr& key, const QSharedPointer<KeyGenerateInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<GenerateSubKeyTag>(ctx_, key, params);
}

void GpgKeyGenerateOpera::GenerateKeyWithSubkey(
    const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const GpgOperationCallback& cb) {
  RunRegisteredAsync<GenerateKeyWithSubKeyTag>(ctx_, cb, p_params, s_params);
}

auto GpgKeyGenerateOpera::GenerateKeyWithSubkeySync(
    const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<GenerateKeyWithSubKeyTag>(ctx_, p_params, s_params);
}

}  // namespace GpgFrontend
