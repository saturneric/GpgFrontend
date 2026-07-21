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

#include "core/function/SystemSecretStore.h"

#include "core/function/SecureRandomGenerator.h"

namespace {

/// Account used only to prove the store works. Never holds real key material.
constexpr auto kProbeAccount = "probe";

std::unique_ptr<GpgFrontend::SystemSecretStore> g_store;

}  // namespace

namespace GpgFrontend {

void RegisterSystemSecretStore(std::unique_ptr<SystemSecretStore> store) {
  if (store != nullptr) {
    LOG_I() << "system secret store backend registered:" << store->Name();
  }
  g_store = std::move(store);
}

auto GetSystemSecretStore() -> SystemSecretStore* { return g_store.get(); }

auto ProbeSystemSecretStore(SystemSecretStore& store) -> bool {
  auto probe = SecureRandomGenerator::Generate(32);
  if (!probe) {
    LOG_W() << "cannot probe secret store: no random source";
    return false;
  }

  if (!store.Write(kProbeAccount, *probe)) {
    LOG_W() << "secret store probe write failed, backend:" << store.Name();
    return false;
  }

  auto read_back = store.Read(kProbeAccount);
  store.Remove(kProbeAccount);

  if (!read_back || *read_back != *probe) {
    LOG_W() << "secret store probe read-back mismatched, backend:"
            << store.Name();
    return false;
  }

  return true;
}

}  // namespace GpgFrontend
