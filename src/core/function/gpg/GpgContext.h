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

#include <map>
#include <mutex>
#include <thread>

#include "core/function/gpg/GnuPGHome.h"
#include "core/function/gpg/GpgAgentProcess.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GpgErrorTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GpgContext : public OpenPGPContext {
 public:
  /**
   * @brief Construct a new Gpg Context object
   *
   * @param channel
   */
  explicit GpgContext(int channel);

  /**
   * @brief Construct a new Gpg Context object
   *
   * @param args
   * @param channel
   */
  explicit GpgContext(const OpenPGPContextInitArgs &args, int channel);

  /**
   * @brief Destroy the Gpg Context object
   *
   */
  ~GpgContext() override;

  /**
   * @brief The calling thread's unarmored gpgme context.
   *
   * A gpgme context may be used by one thread at a time, and an operation's
   * result is read from it after the call returns. A context shared by the
   * GPG runner, the GUI and module threads let one thread's key listing run
   * into another's lookup, and a result be read after another operation
   * replaced it. So every thread gets contexts of its own, configured like
   * the first ones; they live as long as this object.
   *
   * @return gpgme_ctx_t never shared with another thread
   */
  [[nodiscard]] auto BinaryContext() const -> gpgme_ctx_t;

  /**
   * @brief The calling thread's armored gpgme context; see BinaryContext().
   *
   * @return gpgme_ctx_t never shared with another thread
   */
  [[nodiscard]] auto DefaultContext() const -> gpgme_ctx_t;

  /**
   * @brief Create a fresh, fully-initialised armored OpenPGP context.
   *
   * The returned context is configured identically to DefaultContext() (same
   * engine, homedir / key database, key-list mode, armor) but is NOT shared:
   * the caller owns it and must gpgme_release() it. Intended for operations
   * that may wedge and prove un-cancellable (e.g. interactive edit-key
   * sessions), so a stuck operation can be abandoned without poisoning the
   * shared default context.
   *
   * @return a new gpgme context, or nullptr on failure.
   */
  [[nodiscard]] auto CreateDisposableContext() -> gpgme_ctx_t;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto RestartGpgAgent() -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto KillGpgAgent() -> bool;

  /**
   * @brief Asynchronously cancel the GPG operation currently running on this
   * context, if any.
   *
   * Uses gpgme_cancel_async(), which is thread-safe and intended to be called
   * from a thread other than the one blocked in the synchronous operation. The
   * blocked gpgme_op_* call returns GPG_ERR_CANCELED. Cancels both the default
   * (armored) and binary contexts; harmless no-op when nothing is running.
   */
  void CancelCurrentOperation();

  /**
   * @brief
   *
   * @param type
   * @return QString
   */
  [[nodiscard]] auto ComponentDirectory(GpgComponentType type) const -> QString;

  /**
   * @brief The home directory this channel hands to GnuPG.
   *
   * Usually KeyDBPath() itself. It differs only when that path is too long for
   * gpg-agent to put its sockets in, in which case this is a short link to it
   * and the real files are still where KeyDBPath() says.
   *
   * @return the home directory to pass to gnupg
   */
  [[nodiscard]] auto EngineHomePath() const -> QString override {
    return home_.engine_path;
  }

  /**
   * @brief Where this channel's key database is, what GnuPG was told about it,
   * and why that failed if it did.
   *
   * @return this channel's resolved home
   */
  [[nodiscard]] auto Home() const -> const GnuPGHome & { return home_; }

  /**
   * @brief The first key database on this machine whose GnuPG home cannot host
   * the agent sockets.
   *
   * For the interface surfaces that report on the installation as a whole
   * rather than on one channel. It walks the live contexts instead of reading a
   * cached answer, so it cannot report a database that has since been closed.
   *
   * @return the reason, or empty when every open GnuPG database is fine
   */
  static auto FirstUnusableHomeReason() -> QString;

  /**
   * @brief Ask for passphrases through @p cb (loopback pinentry).
   *
   * Given one of this object's contexts, it applies to every thread's
   * contexts, also those made later; see BinaryContext().
   *
   * @param ctx the context to set it on
   * @param cb the callback
   * @return whether it could be set on @p ctx
   */
  auto SetPassphraseCb(const gpgme_ctx_t &ctx, gpgme_passphrase_cb_t cb)
      -> bool;

 protected:
  /**
   * @brief
   *
   * @param args
   * @return true
   * @return false
   */
  auto init(const OpenPGPContextInitArgs &args) -> bool override;

 private:
  gpgme_ctx_t ctx_ref_ = nullptr;             ///<
  gpgme_ctx_t binary_ctx_ref_ = nullptr;      ///<
  gpgme_ctx_t cms_ctx_ref_ = nullptr;         ///<
  gpgme_ctx_t cms_binary_ctx_ref_ = nullptr;  ///<
  OpenPGPContextInitArgs
      init_args_{};  ///< args used to initialise this context
  /// The armored and unarmored contexts of each thread that asked for one,
  /// the first thread's being ctx_ref_ and binary_ctx_ref_.
  struct ThreadContexts {
    gpgme_ctx_t armored = nullptr;
    gpgme_ctx_t binary = nullptr;
  };
  mutable std::mutex thread_ctx_lock_;
  mutable std::map<std::thread::id, ThreadContexts> thread_ctxs_;
  /// Set by SetPassphraseCb() on one of this object's contexts; given to
  /// every thread's contexts.
  gpgme_passphrase_cb_t passphrase_cb_ = nullptr;

  /**
   * @brief Ask @p ctx for passphrases through @p cb (loopback pinentry).
   */
  auto apply_passphrase_cb(gpgme_ctx_t ctx, gpgme_passphrase_cb_t cb) -> bool;

  /**
   * @brief The calling thread's contexts, made on its first call.
   *
   * Falls back to the shared ones, with a warning, when a new context
   * cannot be made; that is what every thread used before.
   */
  [[nodiscard]] auto thread_contexts() const -> ThreadContexts;
  QString gpg_agent_path_;
  QString gpgconf_path_;
  GnuPGHome home_;  ///< see Home(); set once by init()
  QSharedPointer<GpgAgentProcess> agent_;
  /// Whether this context counts towards the agent session of its home. Only
  /// the first context on a home starts the agent and only the last stops it,
  /// so a Runtime replacing another on the same home never cuts the old one's
  /// agent from under it.
  bool agent_session_held_ = false;
  QString agent_session_home_;  ///< the key it joined its agent session with
  QMap<QString, QString> component_dirs_;

  static auto set_ctx_key_list_mode(const gpgme_ctx_t &ctx) -> bool;

  auto set_ctx_openpgp_engine_info(gpgme_ctx_t ctx) -> bool;

  auto common_ctx_initialize(const gpgme_ctx_t &ctx,
                             const OpenPGPContextInitArgs &args) -> bool;

  auto cms_default_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  auto cms_binary_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  auto binary_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  auto default_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  void get_gpg_conf_dirs();

  auto kill_gpg_agent() -> bool;

  auto launch_gpg_agent() -> bool;
};

auto GF_CORE_EXPORT GpgCtx(OpenPGPContext &parent) -> GpgContext &;

}  // namespace GpgFrontend