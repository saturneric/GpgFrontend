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

#include <QCborMap>
#include <QMutex>
#include <QRecursiveMutex>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include "core/model/GFBuffer.h"
#include "sdk/GFSDKCommand.hpp"

namespace GpgFrontend::UI {

/**
 * @file CommandRegistry.h
 * @brief The one registry every command goes through.
 *
 * Host menus, modules in C++ and modules' Lua UI scripts all invoke here,
 * through Invoke(), and nowhere else. That is what makes the policy below
 * the policy: there is no second path on which it could be forgotten.
 *
 * Checked on every invocation, in this order:
 *   1. the command exists;
 *   2. a Host-only command is refused to a module;
 *   3. the caller holds every capability the command declares;
 *   4. the command is enabled, when it can say;
 *   5. it runs on the thread it needs -- the GUI thread for anything that
 *      touches a widget, otherwise wherever its provider runs it.
 *
 * Checked on every registration: a module registers only ids in its own
 * namespace and, when its manifest was verified, only ids the manifest lists.
 */

/// Who is calling, as the Host established it. Never the caller's own word.
struct GF_UI_EXPORT CommandCaller {
  QString module;     ///< empty: the Host itself
  uint32_t caps = 0;  ///< GF_HOST_CAP_* bits; ignored for the Host
  QString source;     ///< "host", "module", "lua:<chunk>:<id>"

  [[nodiscard]] auto IsHost() const -> bool { return module.isEmpty(); }
};

/// What a command provider is, with its implementation erased.
struct GF_UI_EXPORT CommandProvider {
  QString id;
  QString owner;  ///< module id, or empty for the Host
  QCborMap descriptor;
  uint32_t required_caps = 0;
  uint32_t flags = 0;

  /// Run one call. Completes through @p done, exactly once, now or later.
  std::function<void(const gf::cmd::CommandContext&, QCborMap,
                     std::vector<gf::cmd::Blob>, gf::cmd::Completer)>
      run;
  /// GF_CMD_STATE_* bits; empty means always enabled and visible.
  std::function<uint32_t(const gf::cmd::CommandContext&)> state;
};

/// The Host side of a Blob: a GFBuffer, shared rather than copied.
auto GF_UI_EXPORT MakeHostBlob(GFBuffer buffer) -> gf::cmd::Blob;

/// A Blob's bytes as a GFBuffer. No copy when it is already Host-owned.
auto GF_UI_EXPORT BlobToGFBuffer(const gf::cmd::Blob& blob) -> GFBuffer;

/**
 * @brief A command's title, in the user's language.
 *
 * A descriptor carries its title untranslated, with the translation context
 * it was marked in -- "GTrC" for a module's, the Host's own class context
 * for the Host's -- so it is translated when shown, by whichever translators
 * are installed then, and never frozen at registration.
 */
auto GF_UI_EXPORT CommandTitle(const QCborMap& descriptor) -> QString;
auto GF_UI_EXPORT CommandDescription(const QCborMap& descriptor) -> QString;
auto GF_UI_EXPORT CommandCategory(const QCborMap& descriptor) -> QString;

/// Presentation text for a Host command, marked for translation where the
/// Host's own translation scan finds it.
struct GF_UI_EXPORT HostCommandText {
  const char* tr_context;
  const char* title;
  const char* description;
  const char* category;
};

class GF_UI_EXPORT CommandRegistry {
 public:
  static auto Instance() -> CommandRegistry&;

  CommandRegistry();
  ~CommandRegistry();
  CommandRegistry(const CommandRegistry&) = delete;
  auto operator=(const CommandRegistry&) -> CommandRegistry& = delete;

  /// Result of Invoke(): the call id when accepted.
  struct Ticket {
    int status = GF_CMD_OK;
    quint64 call_id = 0;
  };

  /**
   * @brief Register a provider.
   *
   * @param allowlist for a module owner: the ids its signed manifest lists,
   *        or std::nullopt for an unverified module, which may then register
   *        anything in its own namespace and nothing outside it.
   */
  auto Register(CommandProvider provider,
                const std::optional<QStringList>& allowlist = std::nullopt)
      -> int;

  /// Register a typed Host command, replacing an earlier registration of
  /// the same id -- the Host's window may be built more than once.
  auto RegisterHost(const gf::cmd::Binding& binding,
                    const HostCommandText& text) -> int;

  auto Unregister(const QString& id, const QString& owner) -> int;

  /**
   * @brief Invoke @p id.
   *
   * @p done runs exactly once when the call finishes -- unless it is
   * cancelled first -- on whichever thread finished it. It may run before
   * Invoke() returns. When the returned status is not GF_CMD_OK, @p done is
   * not called at all.
   *
   * @p context describes the situation (document, key, selection); the
   * registry overwrites its caller fields from @p caller.
   */
  auto Invoke(const QString& id, QCborMap args,
              std::vector<gf::cmd::Blob> blobs, const CommandCaller& caller,
              gf::cmd::CommandContext context, gf::cmd::Completer done)
      -> Ticket;

  /**
   * @brief Finish call @p call_id. For a module provider, @p provider must
   *        be that module: a module cannot complete somebody else's call.
   */
  auto Finish(quint64 call_id, const QString& provider,
              gf::cmd::RawResult result) -> int;

  /// Cancel a call @p caller made. After this returns, its `done` never runs.
  auto Cancel(quint64 call_id, const QString& caller) -> int;

  [[nodiscard]] auto IsCancelled(quint64 call_id) -> bool;

  [[nodiscard]] auto Describe(const QString& id) -> std::optional<QCborMap>;
  [[nodiscard]] auto List(const QString& prefix = {}) -> QStringList;
  [[nodiscard]] auto Contains(const QString& id) -> bool;

  /// GF_CMD_STATE_* bits for @p caller in @p context. 0 for an unknown id.
  /// Must be called on the GUI thread: providers read the UI to answer.
  [[nodiscard]] auto State(const QString& id, const CommandCaller& caller,
                           const gf::cmd::CommandContext& context) -> uint32_t;

  /// Outstanding calls @p module made, or is providing. For diagnostics.
  [[nodiscard]] auto PendingCallsOf(const QString& module) -> int;

  /**
   * @brief Everything @p module registered or is owed, gone.
   *
   * Its commands are withdrawn; calls it was providing fail with
   * GF_CMD_E_UNAVAILABLE; calls it made are cancelled, so no callback ever
   * enters its code again. Until Reopen(), it is also a CLOSED module: it
   * can neither register nor invoke -- which covers whatever of it is still
   * running meanwhile, its UI script included. Idempotent.
   */
  void RemoveAllFor(const QString& module);

  /// @p module is being activated again: it may register and invoke.
  void Reopen(const QString& module);

 private:
  struct Pending;

  auto Lookup(const QString& id) -> std::shared_ptr<const CommandProvider>;
  void Deliver(const std::shared_ptr<Pending>& call, gf::cmd::RawResult r);

  QMutex mutex_;
  QHash<QString, std::shared_ptr<const CommandProvider>> providers_;
  std::unordered_map<quint64, std::shared_ptr<Pending>> pending_;
  quint64 next_call_id_ = 1;
  QSet<QString> closed_;  ///< modules withdrawn and not yet reactivated
};

}  // namespace GpgFrontend::UI
