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
#include <QObject>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>

#include "sdk/GFSDKCommand.hpp"
#include "ui/lua/LuaAnchors.h"
#include "ui/lua/LuaState.h"
#include "ui/lua/LuaValue.h"

namespace GpgFrontend::UI::Lua {

/// The document a context describes. Never its content.
struct UiDocument {
  qint64 id = 0;
  QString type;
  bool modified = false;
  /// Asked only if the script asks; may read the document, so it is lazy.
  std::function<bool()> has_openpgp;
};

/// The situation one entry into Lua sees. Built by the Host, per entry.
struct UiContext {
  std::optional<UiDocument> document;
  bool has_selection = false;
  std::optional<gf::cmd::KeyRef> key;
};

/// What an action's update() said, checked.
struct ActionState {
  bool visible = false;
  bool enabled = false;
  bool checked = false;
  bool has_checked = false;
  QCborMap args;
  std::vector<gf::cmd::Blob> blobs;
  QString error;  ///< empty when update() ran and its result was valid
};

struct ActionInfo {
  QString id;  ///< "<module id>.<script id>"
  QString anchor;
  QString command;
  int order = 0;
  QString icon;
  QString chunk;
  bool disabled = false;  ///< after an error that must not repeat
  QString last_error;
};

struct MountInfo {
  QString id;
  AnchorKind kind = AnchorKind::kDIALOG;
  QString widget;  ///< the native widget registration's id
  bool factory = false;
  QString section;
  QString document_type;
  QStringList extensions;
  int order = 0;
  QString chunk;
};

struct SubscriptionInfo {
  QString id;
  QString event;
  QString chunk;
  bool disabled = false;
  QString last_error;
};

/// Everything the Module Controller shows about one module's UI script.
struct LuaRuntimeSnapshot {
  QString module;
  size_t memory_used = 0;
  size_t memory_limit = 0;
  QStringList chunks;
  QList<ActionInfo> actions;
  QList<MountInfo> mounts;
  QList<SubscriptionInfo> subscriptions;
  QStringList native_widgets;
  int pending_calls = 0;
  QString last_error;
  bool torn_down = false;
};

/**
 * @brief One module's UI script: its sandboxed state and what it registered.
 *
 * Owned by the thread that created it -- the GUI thread in the application
 * -- and entered only from there. The Host calls Evaluate() before showing
 * an action, Trigger() when the user activates one, and Deliver() for the
 * closed set of UI events; the script never runs otherwise.
 *
 * Every handle the script holds is plain data resolved against this
 * runtime's tables. Tearing the runtime down therefore invalidates every
 * handle at once, and nothing in Lua can point into C++ that is gone.
 */
class GF_UI_EXPORT LuaModuleRuntime : public QObject {
  Q_OBJECT

 public:
  friend struct LuaApi;

  LuaModuleRuntime(QString module, uint32_t caps, LuaState::Limits limits = {});
  ~LuaModuleRuntime() override;

  [[nodiscard]] auto Module() const -> const QString& { return module_; }
  [[nodiscard]] auto Caps() const -> uint32_t { return caps_; }

  /**
   * @brief Load and run one script chunk.
   *
   * Everything the chunk registered is rolled back if it fails, so a module
   * never ends up half-integrated.
   */
  auto Load(const QByteArray& source, const QString& chunk, QString* error)
      -> bool;

  [[nodiscard]] auto Actions(const QString& anchor = {}) const
      -> QList<ActionInfo>;
  [[nodiscard]] auto Mounts() const -> QList<MountInfo>;
  [[nodiscard]] auto Subscriptions() const -> QList<SubscriptionInfo>;

  /// Run an action's update() against @p ctx. Pure: it may read, not act.
  auto Evaluate(const QString& action_id, const UiContext& ctx) -> ActionState;

  /**
   * @brief The user activated an action.
   *
   * update() runs again against a FRESH context, its result is checked
   * again, and the command is invoked only if the action is still visible
   * and enabled -- so what runs is what the user is looking at now, not what
   * was true when the menu opened.
   *
   * @return the registry's status for the invocation
   */
  auto Trigger(const QString& action_id, const UiContext& ctx) -> int;

  /// A UI event, to the subscriptions for it. Unknown events do nothing.
  void Deliver(const QString& event, const UiContext& ctx,
               qint64 document_id = 0);

  /// The events a script may subscribe to. Closed; not the event catalog.
  static auto Events() -> const QStringList&;

  /**
   * @brief Stop everything, in order. Idempotent.
   *
   *  1. no callback enters Lua from here on;
   *  2. outstanding command calls are cancelled, their continuations dropped;
   *  3. subscriptions and actions are removed;
   *  4. mounts are removed (their containers are closed by the Host);
   *  5. every handle is invalidated;
   *  6. the Lua state is closed.
   */
  void Teardown();

  /// Step 1 alone, safe from any thread: no callback enters Lua after this.
  void StopCallbacks() { stopping_.store(true); }

  [[nodiscard]] auto TornDown() const -> bool { return torn_down_; }
  [[nodiscard]] auto Snapshot() const -> LuaRuntimeSnapshot;

  /// For tests: the teardown steps, in the order they ran.
  [[nodiscard]] auto TeardownLog() const -> const QStringList& {
    return teardown_log_;
  }

  /// For tests: the state, to reach its limits and fault injection.
  [[nodiscard]] auto State() -> LuaState* { return state_.get(); }

 signals:
  /// What the script registered changed; placements rebuild.
  void SignalChanged();

 private:
  enum class Phase { kIDLE, kLOAD, kPURE, kHANDLER };

  struct AnchorRef {
    const AnchorSpec* spec = nullptr;
    QString section;
    QString document_type;
    QStringList extensions;
  };

  struct NativeRef {
    QString id;
    bool factory = false;
  };

  struct PendingCall {
    quint64 registry_call = 0;
    /// A Lua registry reference, or LUA_NOREF when none. Spelled as a number
    /// because this header does not include Lua's; the source checks it.
    static constexpr int kNoRef = -2;
    int continuation = kNoRef;
    QString command;
    QString source;
  };

  struct ActionEntry {
    ActionInfo info;
    int update = -2;  ///< registry ref, LUA_NOREF when none
  };

  struct SubscriptionEntry {
    SubscriptionInfo info;
    int handler = -2;
  };

  auto NextId() -> qint64 { return ++next_id_; }
  void EndEntry();
  auto CommandContextFor(const UiContext& ctx) const -> gf::cmd::CommandContext;
  auto Resolve(const Handle& h, std::vector<gf::cmd::Blob>* blobs,
               QString* error) -> std::optional<QCborValue>;
  auto CheckUpdateResult(const ActionEntry& action, const LuaTree& tree)
      -> ActionState;
  void Complete(qint64 call_handle, gf::cmd::RawResult result);
  void RecordError(const QString& where, const QString& what);

  QString module_;
  uint32_t caps_;
  std::unique_ptr<LuaState> state_;
  quint64 state_tag_;

  Phase phase_ = Phase::kIDLE;
  QString chunk_;                   ///< the chunk being loaded
  quint64 epoch_ = 1;               ///< bumped when every entry returns
  const UiContext* ctx_ = nullptr;  ///< valid during an entry only
  QString source_;                  ///< who the current entry acts as

  qint64 next_id_ = 0;
  QHash<qint64, QString> commands_;
  QHash<qint64, AnchorRef> anchors_;
  QHash<qint64, NativeRef> natives_;
  QHash<qint64, MountInfo> mount_handles_;
  QHash<qint64, PendingCall> calls_;
  QHash<qint64, gf::cmd::Blob> blobs_;
  QHash<qint64, gf::cmd::DocumentRef> document_refs_;
  QHash<qint64, gf::cmd::KeyRef> key_refs_;

  QList<ActionEntry> actions_;
  QList<MountInfo> mounts_;
  QList<SubscriptionEntry> subscriptions_;
  QStringList chunks_;

  std::atomic<bool> stopping_{false};
  bool torn_down_ = false;
  QString last_error_;
  QStringList teardown_log_;
};

}  // namespace GpgFrontend::UI::Lua
