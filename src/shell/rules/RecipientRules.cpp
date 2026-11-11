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

#include "shell/rules/RecipientRules.h"

#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/model/GpgKeyGroup.h"

namespace GpgFrontend {

auto MixesPostQuantumAndClassical(const QList<QList<bool>>& recipients)
    -> bool {
  bool any_pq = false;
  bool any_classical = false;
  for (const auto& subkeys : recipients) {
    if (subkeys.isEmpty()) continue;
    if (std::all_of(subkeys.cbegin(), subkeys.cend(),
                    [](bool pq) { return pq; })) {
      any_pq = true;
    } else {
      any_classical = true;
    }
  }
  return any_pq && any_classical;
}

auto RecipientsMixPostQuantumAndClassical(const GpgAbstractKeyPtrList& keys)
    -> bool {
  QList<QList<bool>> recipients;
  for (const auto& key : keys) {
    if (key == nullptr || key->KeyType() != GpgAbstractKeyType::kGPG_KEY) {
      continue;
    }
    auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
    if (gpg_key == nullptr) continue;

    // A pinned subkey is the one used; otherwise any usable one may be.
    QList<bool> usable;
    QList<bool> pinned;
    for (const auto& s_key : gpg_key->SubKeys()) {
      if (!s_key.IsHasEncrCap() || s_key.IsRevoked() || s_key.IsExpired()) {
        continue;
      }
      const auto algo = KeyGenerateInfo::SearchKeyAlgoByAlgoId(s_key.AlgoId());
      const bool pq = algo.has_value() && algo->IsPostQuantum();
      usable.append(pq);
      if (s_key.IsMarked()) pinned.append(pq);
    }
    recipients.append(pinned.isEmpty() ? usable : pinned);
  }
  return MixesPostQuantumAndClassical(recipients);
}

namespace {

template <typename Capability>
auto UsableSubkeys(const GpgAbstractKeyPtr& key, Capability&& capability)
    -> QList<GpgSubKey> {
  QList<GpgSubKey> usable;
  if (key == nullptr || key->KeyType() != GpgAbstractKeyType::kGPG_KEY) {
    return usable;
  }
  const auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
  if (gpg_key == nullptr) return usable;
  for (const auto& s_key : gpg_key->SubKeys()) {
    if (capability(s_key) && !s_key.IsRevoked() && !s_key.IsExpired()) {
      usable << s_key;
    }
  }
  return usable;
}

}  // namespace

auto UsableSigningSubkeys(const GpgAbstractKeyPtr& key) -> QList<GpgSubKey> {
  return UsableSubkeys(
      key, [](const GpgSubKey& s_key) { return s_key.IsHasSignCap(); });
}

auto UsableEncryptionSubkeys(const GpgAbstractKeyPtr& key)
    -> QList<GpgSubKey> {
  return UsableSubkeys(
      key, [](const GpgSubKey& s_key) { return s_key.IsHasEncrCap(); });
}

auto HasAmbiguousSigningSubkey(const GpgAbstractKeyPtrList& keys) -> bool {
  return std::any_of(keys.begin(), keys.end(), [](const auto& key) {
    return UsableSigningSubkeys(key).size() > 1;
  });
}

auto HasAmbiguousEncryptionSubkey(const GpgAbstractKeyPtrList& keys) -> bool {
  return std::any_of(keys.begin(), keys.end(), [](const auto& key) {
    return UsableEncryptionSubkeys(key).size() > 1;
  });
}

}  // namespace GpgFrontend
