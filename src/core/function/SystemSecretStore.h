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
   * @brief Explain the most recent failure, when the backend can.
   *
   * The interface reports failure as a bare false or an empty result, which
   * cannot tell a locked keyring from an absent service from a rejected write.
   * Backends that get a message from the platform surface it here so a user
   * facing a store that loaded but does not work is not left guessing.
   *
   * Untranslated: it is the platform's own wording, and it is meant to be read
   * back verbatim. Not pure, because a backend with nothing to add is normal.
   *
   * @return the message, or empty when the last call succeeded or said nothing
   */
  [[nodiscard]] virtual auto LastError() const -> QString { return {}; }

  /**
   * @brief Ask the platform to unlock its store, interactively if it must.
   *
   * Exists because a lookup that finds nothing cannot be told apart from a
   * lookup against a store that never came online. On the Secret Service the
   * two are the same call: it raises an unlock prompt only when the daemon
   * reports the wanted item as locked, and a collection that has not been
   * opened in this session reports nothing at all -- no item, no prompt, and
   * no error either. Asking to unlock explicitly is the only way to reach the
   * prompt in that state.
   *
   * Blocks on a human, so it may only be called where the user has already
   * asked for the thing that needs the secret. There is no timeout: a prompter
   * that never answers holds the caller, exactly as a lookup already does.
   *
   * The default does nothing on purpose, rather than being pure: macOS raises
   * its own unlock panel from inside SecItemCopyMatching, and Credential
   * Manager entries have no lock state for anything to raise. Only the Secret
   * Service needs to be asked.
   *
   * @return true when the store is now unlocked, i.e. when a read that just
   * came back empty is worth repeating; false leaves the caller's earlier
   * result standing
   */
  [[nodiscard]] virtual auto Unlock() -> bool { return false; }

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
 * @brief Explain the credential store's current trouble, whatever its stage.
 *
 * Two failures reach every display path and only one of them has a reason at
 * any given time: a backend that never loaded leaves it in the registry, and
 * one that loaded and then refused leaves it on the store. Picking between
 * them is the same three lines everywhere it is needed, so it lives here
 * rather than in each dialog.
 *
 * Untranslated, for the same reason
 * SystemSecretStoreUnavailableReason() is.
 *
 * @return the reason, or empty when nothing has anything to add
 */
auto GF_CORE_EXPORT SystemSecretStoreReason() -> QString;

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
