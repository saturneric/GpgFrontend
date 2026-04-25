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

#include "KeyStorage.h"

#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto GetKeyPtrGnuPGImpl(OpenPGPContext& ctx, const QString& key_id, bool secret)
    -> GpgKeyPtr {
  auto& g_ctx = GpgCtx(ctx);
  gpgme_key_t p_key = nullptr;
  gpgme_get_key(g_ctx.DefaultContext(), key_id.toUtf8(), &p_key,
                secret ? 1 : 0);

  if (p_key == nullptr) {
    return GetKeyPtrGnuPGImpl(ctx, key_id, false);
  }
  return SecureCreateSharedObject<GpgKey>(p_key);
}

auto FlushKeyDatabaseGnuPGImpl(OpenPGPContext& ctx) -> bool {
  // init
  auto& g_ctx = GpgCtx(ctx);
  GpgError err = gpgme_op_keylist_start(g_ctx.DefaultContext(), nullptr, 0);

  // for debug
  assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);

  // return when error
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;

  gpgme_key_t key;
  while ((err = gpgme_op_keylist_next(g_ctx.DefaultContext(), &key)) ==
         GPG_ERR_NO_ERROR) {
    SecureCreateSharedObject<GpgKey>(key);
  }

  // for debug
  assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_EOF);

  err = gpgme_op_keylist_end(g_ctx.DefaultContext());
  assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_NO_ERROR);

  return true;
}

auto FlushKeyCacheGnuPGImpl(
    OpenPGPContext& ctx, const QSharedPointer<GpgKeyPtrList>& keys_cache,
    const QSharedPointer<QMap<QString, GpgAbstractKeyPtr>>& keys_search_cache)
    -> bool {
  // init
  auto& g_ctx = GpgCtx(ctx);
  GpgError err = gpgme_op_keylist_start(g_ctx.DefaultContext(), nullptr, 0);

  // for debug
  assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);

  // return when error
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;

  gpgme_key_t key;
  while ((err = gpgme_op_keylist_next(g_ctx.DefaultContext(), &key)) ==
         GPG_ERR_NO_ERROR) {
    auto g_key = SecureCreateSharedObject<GpgKey>(key);

    // detect if the key is in a smartcard
    // if so, try to get full information using gpgme_get_key()
    // this maybe a bug in gpgme
    if (g_key->IsHasCardKey()) {
      g_key = GetKeyPtrGnuPGImpl(ctx, g_key->ID(), true);
    }

    keys_cache->push_back(g_key);
    keys_search_cache->insert(g_key->ID(), g_key);
    keys_search_cache->insert(g_key->Fingerprint(), g_key);

    for (const auto& s_key : g_key->SubKeys()) {
      if (s_key.ID() == g_key->ID()) continue;

      // don't add adsk key or it will cause bugs
      if (s_key.IsADSK()) continue;

      // subkeys should be weaker than primary key
      if (keys_search_cache->contains(s_key.ID())) continue;

      auto p_s_key = SecureCreateSharedObject<GpgSubKey>(s_key);
      keys_search_cache->insert(s_key.ID(), p_s_key);
      keys_search_cache->insert(s_key.Fingerprint(), p_s_key);
    }
  }

  // for debug
  assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_EOF);

  err = gpgme_op_keylist_end(g_ctx.DefaultContext());
  assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_NO_ERROR);

  return true;
}

}  // namespace GpgFrontend