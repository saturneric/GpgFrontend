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

#include "GpgKeyOpera.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/KeyManagementTraits.h"
#include "core/function/rpgp/KeyManagement.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyOpera::GpgKeyOpera(int channel)
    : SingletonFunctionObject<GpgKeyOpera>(channel) {}

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgKeyOpera::DeleteKeys(const GpgAbstractKeyPtrList& keys) {
  RunRegisteredForward<DeleteKeysOpTag>(ctx_, keys);
}

/**
 * Set the expire date and time of a key pair(actually the primary key) or
 * subkey
 * @param key target key pair
 * @param subkey null if primary key
 * @param expires date and time
 * @return if successful
 */
auto GpgKeyOpera::SetExpire(const GpgKeyPtr& key, const SubkeyId& skey_fpr,
                            const std::optional<QDateTime>& expires)
    -> GpgError {
  return RunRegisteredForward<SetExpireOpTag>(ctx_, key, skey_fpr, expires);
}

/**
 * Generate revoke cert of a key pair
 * @param key target key pair
 * @param outputFileName out file name(path)
 * @return the process doing this job
 */
void GpgKeyOpera::GenerateRevokeCert(const GpgKeyPtr& key,
                                     const QString& output_path,
                                     int reason_code,
                                     const QString& reason_text) {
  RunRegisteredForward<GenerateRevCertOpTag>(ctx_, key, output_path,
                                             reason_code, reason_text);
}

void GpgKeyOpera::ModifyPassword(const GpgKeyPtr& key,
                                 const GpgOperationCallback& cb) {
  RunRegisteredAsync<ModifyKeyPassphraseOpTag>(ctx_, cb, key);
}

void GpgKeyOpera::DeleteKey(const GpgAbstractKeyPtr& key) { DeleteKeys({key}); }

void GpgKeyOpera::AddADSK(const GpgKeyPtr& key, const GpgSubKey& adsk,
                          const GpgOperationCallback& cb) {
  RunRegisteredAsync<AddADSKOpTag>(ctx_, cb, key, adsk);
}

auto GpgKeyOpera::AddADSKSync(const GpgKeyPtr& key, const GpgSubKey& adsk)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunRegisteredSync<AddADSKOpTag>(ctx_, key, adsk);
}
}  // namespace GpgFrontend
