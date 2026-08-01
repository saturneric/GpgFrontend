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

#include <memory>

#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Service name every backend files its entries under.
 *
 * Scoped to the build's profile rather than fixed, because the entry is shared
 * state the same way the data directory is: turning protection off calls
 * Remove() on it, so a nightly and an installed release filing under one name
 * means either can delete the secret the other's wrapped app.key depends on --
 * locking that installation out of its own key permanently. Isolating the
 * directories without isolating this would only move the failure.
 *
 * @return service name, stable for the lifetime of the process
 */
auto GF_CORE_EXPORT SystemSecretService() -> const char*;

/// Account name of the secret that wraps the application secure key.
constexpr auto kAppKeyWrapAccount = "app-key-wrap";

/**
 * @brief A credential store provided by the operating system.
 *
 * Implementations live in src/platform and register themselves at startup.
 * The interface takes and returns raw binary; encoding for string-oriented
 * platform APIs is the implementation's business.
 */
class GF_CORE_EXPORT SystemSecretStore {
 public:
  SystemSecretStore() = default;
  virtual ~SystemSecretStore() = default;

  SystemSecretStore(const SystemSecretStore&) = delete;
  auto operator=(const SystemSecretStore&) -> SystemSecretStore& = delete;
  SystemSecretStore(SystemSecretStore&&) = delete;
  auto operator=(SystemSecretStore&&) -> SystemSecretStore& = delete;

  /**
   * @brief Return a short name of the backing store, for logs and dialogs.
   *
   * @return backend name, e.g. "libsecret"
   */
  [[nodiscard]] virtual auto Name() const -> QString = 0;

  /**
   * @brief Report whether the store can actually be used right now.
   *
   * Implementations should answer by round-tripping a probe entry rather than
   * by checking that a library loaded: a present library with no running
   * daemon, a locked keyring, or a missing entitlement all have to read as
   * unavailable. The answer is expected to be cached for the process lifetime.
   *
   * @return true when secrets can be written and read back
   */
  [[nodiscard]] virtual auto IsAvailable() -> bool = 0;

  /**
   * @brief Read a secret.
   *
   * @param account account name within the service
   * @return the secret, or empty when absent or unreadable
   */
  virtual auto Read(const QString& account) -> GFBufferOrNone = 0;

  /**
   * @brief Write a secret, replacing any existing entry.
   *
   * @param account account name within the service
   * @param secret binary secret to store
   * @return true on success
   */
  virtual auto Write(const QString& account, const GFBuffer& secret)
      -> bool = 0;

  /**
   * @brief Delete a secret.
   *
   * @param account account name within the service
   * @return true when the entry is gone, including when it never existed
   */
  virtual auto Remove(const QString& account) -> bool = 0;
};

/**
 * @brief Install the backend for this platform. Called once during startup.
 *
 * @param store backend to install, or nullptr for none
 */
void GF_CORE_EXPORT
RegisterSystemSecretStore(std::unique_ptr<SystemSecretStore> store);

/**
 * @brief Record that this platform has no usable backend, and why.
 *
 * The reason exists because "System keychain" then greys out in the settings,
 * and a user who has already installed the credential library has no way left
 * to tell an unsupported platform from a library that failed to load. Logs do
 * not close that gap: the default log level filters warnings out entirely, so
 * the reason has to reach the interface.
 *
 * @param reason technical, untranslated, safe to show and to paste into a bug
 *               report; must never contain secrets or a user's file paths
 */
void GF_CORE_EXPORT RegisterSystemSecretStoreUnavailable(QString reason);

/**
 * @brief Return the installed backend.
 *
 * @return the backend, or nullptr when this platform has none
 */
auto GF_CORE_EXPORT GetSystemSecretStore() -> SystemSecretStore*;

/**
 * @brief Explain why no backend is installed.
 *
 * Untranslated on purpose: it carries a library name and the loader's own
 * message, which are what a maintainer needs to read back verbatim. Callers
 * pair it with translated prose rather than showing it alone.
 *
 * @return the reason, or empty when a backend is installed or none was given
 */
auto GF_CORE_EXPORT SystemSecretStoreUnavailableReason() -> QString;

/**
 * @brief Round-trip a throwaway entry to prove the store really works.
 *
 * Shared by the platform backends so they all answer IsAvailable() the same
 * way. Writing is part of the check on purpose: on some platforms a read of a
 * missing entry is indistinguishable from a broken or locked store.
 *
 * @param store backend to probe
 * @return true when a probe secret could be written, read back intact, and
 * deleted
 */
auto GF_CORE_EXPORT ProbeSystemSecretStore(SystemSecretStore& store) -> bool;

}  // namespace GpgFrontend
