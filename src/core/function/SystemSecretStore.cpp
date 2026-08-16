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
#include "core/utils/BuildInfoUtils.h"

namespace {

/// Account used only to prove the store works. Never holds real key material.
constexpr auto kProbeAccount = "probe";

std::unique_ptr<GpgFrontend::SystemSecretStore> g_store;

/// Why g_store is null. Only meaningful while it is.
QString g_unavailable_reason;

}  // namespace

namespace GpgFrontend {

auto SystemSecretService() -> const char* {
  // Held in a function-local static so callers can pass the pointer straight
  // into C varargs APIs without worrying about its lifetime.
  static const QByteArray kService = GetAppProfileName().toUtf8();
  return kService.constData();
}

void RegisterSystemSecretStore(std::unique_ptr<SystemSecretStore> store) {
  if (store != nullptr) {
    LOG_I() << "system secret store backend registered:" << store->Name();
  }

  // Cleared whichever way this goes: registering carries no explanation, so
  // holding on to an earlier one would leave a stale reason describing a state
  // that no longer applies. Only RegisterSystemSecretStoreUnavailable sets it.
  g_unavailable_reason.clear();
  g_store = std::move(store);
}

void RegisterSystemSecretStoreUnavailable(QString reason) {
  LOG_W() << "no system secret store backend available:" << reason;
  g_store = nullptr;
  g_unavailable_reason = std::move(reason);
}

auto GetSystemSecretStore() -> SystemSecretStore* { return g_store.get(); }

auto SystemSecretStoreUnavailableReason() -> QString {
  // A backend that later registers makes any earlier reason stale, and a stale
  // explanation on screen is worse than none.
  return g_store != nullptr ? QString{} : g_unavailable_reason;
}

auto SystemSecretStoreReason() -> QString {
  auto reason = SystemSecretStoreUnavailableReason();

  // Only one of the two can be set at a time -- the reason above is suppressed
  // once a backend registers -- so this is a fallback rather than a choice.
  if (auto* store = GetSystemSecretStore();
      reason.isEmpty() && store != nullptr) {
    reason = store->LastError();
  }

  return reason;
}

auto ProbeSystemSecretStore(SystemSecretStore& store) -> bool {
  auto probe = SecureRandomGenerator::Generate(32);
  if (!probe) {
    LOG_W() << "cannot probe secret store: no random source";
    return false;
  }

  if (!store.Write(kProbeAccount, *probe)) {
    LOG_W() << "secret store probe write failed, backend:" << store.Name()
            << "reason:" << store.LastError();
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
