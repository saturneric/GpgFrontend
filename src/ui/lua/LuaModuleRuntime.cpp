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

#include "LuaModuleRuntime.h"

#include <QPointer>
#include <QRandomGenerator>

#include "LuaApi.h"
#include "LuaBinding.h"
#include "ui/command/CommandRegistry.h"

namespace GpgFrontend::UI::Lua {

namespace {

auto NewTag() -> quint64 {
  return QRandomGenerator::system()->generate64() | 1U;
}

}  // namespace

LuaModuleRuntime::LuaModuleRuntime(QString module, uint32_t caps,
                                   LuaState::Limits limits)
    : module_(std::move(module)),
      caps_(caps),
      state_(std::make_unique<LuaState>(limits)),
      state_tag_(NewTag()) {
  if (!state_->Ok()) {
    state_.reset();
    return;
  }
  state_->SetOwner(this);
  state_->SetLogPrefix(module_ + ":");
  QString error;
  if (!LuaApi::Install(*this, &error)) {
    RecordError(QStringLiteral("install"), error);
    state_.reset();
  }
}

LuaModuleRuntime::~LuaModuleRuntime() { Teardown(); }

void LuaModuleRuntime::EndEntry() {
  ++epoch_;
  ctx_ = nullptr;
  phase_ = Phase::kIDLE;
}

void LuaModuleRuntime::RecordError(const QString& where, const QString& what) {
  last_error_ = QStringLiteral("%1 / %2: %3").arg(module_, where, what);
  LOG_W() << "module UI script:" << last_error_;
}

auto LuaModuleRuntime::Events() -> const QStringList& {
  static const QStringList kEvents = {
      QStringLiteral("document.activated"),
      QStringLiteral("document.state_changed"),
      QStringLiteral("document.saved"),
      QStringLiteral("key_database.refreshed"),
      QStringLiteral("app.ui_ready"),
  };
  return kEvents;
}

auto LuaModuleRuntime::CommandContextFor(const UiContext& ctx) const
    -> gf::cmd::CommandContext {
  gf::cmd::CommandContext c;
  if (ctx.document.has_value()) {
    c.document = gf::cmd::DocumentRef{ctx.document->id, 0, ctx.document->type};
  }
  c.has_selection = ctx.has_selection;
  c.key = ctx.key;
  return c;
}

auto LuaModuleRuntime::Load(const QByteArray& source, const QString& chunk,
                            QString* error) -> bool {
  if (state_ == nullptr || stopping_.load() || torn_down_) {
    if (error != nullptr) *error = QStringLiteral("the UI runtime is closed");
    return false;
  }
  if (chunks_.contains(chunk)) {
    if (error != nullptr) *error = QStringLiteral("chunk already loaded");
    return false;
  }

  phase_ = Phase::kLOAD;
  chunk_ = chunk;
  source_ = QStringLiteral("lua:") + chunk;
  QString local;
  const bool ok =
      state_->LoadAndRun(source, chunk, error != nullptr ? error : &local);
  EndEntry();

  if (!ok) {
    // Everything the chunk registered goes with it: a module is integrated
    // all the way or not at all.
    QList<int> refs;
    for (auto it = actions_.begin(); it != actions_.end();) {
      if (it->info.chunk == chunk) {
        refs.append(it->update);
        it = actions_.erase(it);
      } else {
        ++it;
      }
    }
    for (auto it = subscriptions_.begin(); it != subscriptions_.end();) {
      if (it->info.chunk == chunk) {
        refs.append(it->handler);
        it = subscriptions_.erase(it);
      } else {
        ++it;
      }
    }
    for (auto it = mounts_.begin(); it != mounts_.end();) {
      if (it->chunk == chunk) {
        it = mounts_.erase(it);
      } else {
        ++it;
      }
    }
    Protected(state_->L(), [&refs](lua_State* L) -> int {
      for (const int r : refs) luaL_unref(L, LUA_REGISTRYINDEX, r);
      return 0;
    });
    RecordError(chunk, error != nullptr ? *error : local);
    emit SignalChanged();
    return false;
  }

  chunks_.append(chunk);
  emit SignalChanged();
  return true;
}

auto LuaModuleRuntime::Actions(const QString& anchor) const
    -> QList<ActionInfo> {
  QList<ActionInfo> out;
  for (const auto& a : actions_) {
    if (anchor.isEmpty() || a.info.anchor == anchor) out.append(a.info);
  }
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
    return a.order != b.order ? a.order < b.order : a.id < b.id;
  });
  return out;
}

auto LuaModuleRuntime::Mounts() const -> QList<MountInfo> { return mounts_; }

auto LuaModuleRuntime::Subscriptions() const -> QList<SubscriptionInfo> {
  QList<SubscriptionInfo> out;
  for (const auto& s : subscriptions_) out.append(s.info);
  return out;
}

auto LuaModuleRuntime::Resolve(const Handle& h,
                               std::vector<gf::cmd::Blob>* blobs,
                               QString* error) -> std::optional<QCborValue> {
  const auto fail = [error](const QString& why) -> std::optional<QCborValue> {
    if (error != nullptr && error->isEmpty()) *error = why;
    return std::nullopt;
  };
  if (h.state_tag != state_tag_) {
    return fail(QStringLiteral("a handle from another module, or from before "
                               "this module was reloaded"));
  }
  gf::cmd::EncodeState st;
  switch (h.kind) {
    case HandleKind::kDOCUMENT:
      if (h.epoch != epoch_ || ctx_ == nullptr || !ctx_->document) {
        return fail(QStringLiteral("a document handle outlived its call; "
                                   "keep document:ref() instead"));
      }
      return QCborValue(gf::cmd::EncodeMap(
          gf::cmd::DocumentRef{ctx_->document->id, 0, ctx_->document->type},
          st));
    case HandleKind::kKEY:
      if (h.epoch != epoch_ || ctx_ == nullptr || !ctx_->key) {
        return fail(QStringLiteral("a key handle outlived its call; keep "
                                   "key:ref() instead"));
      }
      return QCborValue(gf::cmd::EncodeMap(*ctx_->key, st));
    case HandleKind::kDOCUMENT_REF: {
      const auto it = document_refs_.constFind(h.id);
      if (it == document_refs_.constEnd()) return fail("a stale document ref");
      return QCborValue(gf::cmd::EncodeMap(*it, st));
    }
    case HandleKind::kKEY_REF: {
      const auto it = key_refs_.constFind(h.id);
      if (it == key_refs_.constEnd()) return fail("a stale key ref");
      return QCborValue(gf::cmd::EncodeMap(*it, st));
    }
    case HandleKind::kMOUNT: {
      const auto it = mount_handles_.constFind(h.id);
      if (it == mount_handles_.constEnd()) return fail("an unknown mount");
      return QCborValue(gf::cmd::EncodeMap(gf::cmd::ViewRef{it->id}, st));
    }
    case HandleKind::kBLOB: {
      const auto it = blobs_.constFind(h.id);
      if (it == blobs_.constEnd()) return fail("a Blob already used up");
      QCborMap ref;
      ref.insert(QStringLiteral("$blob"), static_cast<qint64>(blobs->size()));
      blobs->push_back(*it);
      return QCborValue(ref);
    }
    default:
      return fail(QStringLiteral("a %1 cannot be an argument")
                      .arg(QLatin1String(HandleTypeName(h.kind))));
  }
}

auto LuaModuleRuntime::CheckUpdateResult(const ActionEntry& action,
                                         const LuaTree& tree) -> ActionState {
  ActionState st;
  st.visible = true;
  st.enabled = true;
  const auto fail = [&st](const QString& why) {
    st.visible = false;
    st.enabled = false;
    st.error = why;
    return st;
  };

  if (tree.empty() || tree[0].type != LuaNode::Type::kTABLE) {
    return fail(QStringLiteral("update() must return a table"));
  }
  const auto descriptor =
      CommandRegistry::Instance().Describe(action.info.command);
  if (!descriptor.has_value()) return fail(QStringLiteral("unknown command"));
  const auto args_schema =
      descriptor->value(QStringLiteral("args")).toMap();
  const bool checkable =
      (descriptor->value(QStringLiteral("flags")).toInteger() &
       gf::cmd::kCheckable) != 0;

  int args_node = -1;
  for (const int c : ChildrenOf(tree, 0)) {
    const auto& n = tree[static_cast<size_t>(c)];
    if (n.key_is_index) return fail(QStringLiteral("update() returned a list"));
    if (n.key == "visible" || n.key == "enabled" || n.key == "checked") {
      if (n.type != LuaNode::Type::kBOOL) {
        return fail(QStringLiteral("\"%1\" must be a boolean")
                        .arg(QString::fromUtf8(n.key)));
      }
      if (n.key == "visible") st.visible = n.boolean;
      if (n.key == "enabled") st.enabled = n.boolean;
      if (n.key == "checked") {
        if (!checkable) {
          return fail(QStringLiteral("\"checked\" on a command that is not "
                                     "checkable"));
        }
        st.checked = n.boolean;
        st.has_checked = true;
      }
    } else if (n.key == "args") {
      args_node = c;
    } else {
      return fail(QStringLiteral("update() returned an unknown field \"%1\"")
                      .arg(QString::fromUtf8(n.key)));
    }
  }

  // Arguments are what the command would run with, so an action that
  // cannot run -- hidden, or disabled -- needs none, and a wrong one is not
  // an error until it could matter.
  if (!st.visible || !st.enabled) {
    if (args_node >= 0) {
      QString ignored;
      std::vector<gf::cmd::Blob> unused;
      const auto resolve = [this](const Handle& h,
                                  std::vector<gf::cmd::Blob>* b,
                                  QString* e) { return Resolve(h, b, e); };
      if (!NodeToCbor(tree, args_node, args_schema, resolve, &unused,
                      &ignored)
               .has_value()) {
        return fail(QStringLiteral("args: ") + ignored);
      }
    }
    return st;
  }
  if (args_node < 0 &&
      args_schema.value(QStringLiteral("fields")).toArray().isEmpty()) {
    return st;  // a command that takes nothing
  }
  QString error;
  const auto resolve = [this](const Handle& h,
                              std::vector<gf::cmd::Blob>* blobs,
                              QString* err) { return Resolve(h, blobs, err); };
  // No `args` reads as an empty table: a command whose fields are all
  // optional runs, and one with a required field says which is missing.
  LuaTree empty;
  empty.push_back(LuaNode{});
  empty[0].type = LuaNode::Type::kTABLE;
  const auto args =
      args_node >= 0
          ? NodeToCbor(tree, args_node, args_schema, resolve, &st.blobs, &error)
          : NodeToCbor(empty, 0, args_schema, resolve, &st.blobs, &error);
  if (!args.has_value()) return fail(QStringLiteral("args: ") + error);
  st.args = args->toMap();
  return st;
}

auto LuaModuleRuntime::Evaluate(const QString& action_id, const UiContext& ctx)
    -> ActionState {
  ActionState st;
  if (state_ == nullptr || stopping_.load() || torn_down_) return st;

  ActionEntry* action = nullptr;
  for (auto& a : actions_) {
    if (a.info.id == action_id) action = &a;
  }
  if (action == nullptr) {
    st.error = QStringLiteral("unknown action");
    return st;
  }
  if (action->info.disabled) return st;

  const CommandCaller caller{module_, caps_,
                             QStringLiteral("lua:%1:%2")
                                 .arg(action->info.chunk, action->info.id)};
  const auto bits = CommandRegistry::Instance().State(
      action->info.command, caller, CommandContextFor(ctx));

  if (action->update == LUA_NOREF) {
    st.visible = (bits & GF_CMD_STATE_VISIBLE) != 0;
    st.enabled = (bits & GF_CMD_STATE_ENABLED) != 0;
    return st;
  }

  phase_ = Phase::kPURE;
  ctx_ = &ctx;
  source_ = caller.source;
  state_->ArmCallBudget();
  const int ref = action->update;
  const quint64 tag = state_tag_;
  const quint64 epoch = epoch_;
  LuaTree tree;
  ErrorText flatten_error{};
  bool flattened = false;
  BindingOutcome out;
  const bool ok = Protected(
      state_->L(),
      [ref, tag, epoch, &tree, &flatten_error, &flattened](lua_State* L) -> int {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        PushHandle(L, HandleKind::kCONTEXT, tag, 1, epoch);
        lua_call(L, 1, 1);
        flattened = FlattenValue(L, -1, &tree, &flatten_error);
        return 0;
      },
      &out);

  if (!ok || !flattened) {
    const auto why = ok ? QString::fromUtf8(flatten_error.data())
                        : QString::fromUtf8(out.message.data());
    // An exhausted budget will be exhausted again: the action is switched
    // off rather than stalling every menu that shows it.
    if (state_->BudgetExhausted()) action->info.disabled = true;
    action->info.last_error = why;
    RecordError(action->info.id, why);
    EndEntry();
    st.error = why;
    return st;
  }

  // Resolved before the epoch moves on: handles from this call are still
  // good for exactly this long.
  st = CheckUpdateResult(*action, tree);
  EndEntry();
  if (!st.error.isEmpty()) {
    action->info.last_error = st.error;
    RecordError(action->info.id, st.error);
    return st;
  }

  // The command has the last word: an action is never shown for a command
  // the module may not use, nor enabled when the command says it is not.
  st.visible = st.visible && (bits & GF_CMD_STATE_VISIBLE) != 0;
  st.enabled = st.enabled && (bits & GF_CMD_STATE_ENABLED) != 0;
  return st;
}

auto LuaModuleRuntime::Trigger(const QString& action_id, const UiContext& ctx)
    -> int {
  QString command;
  QString chunk;
  for (const auto& a : actions_) {
    if (a.info.id == action_id) {
      command = a.info.command;
      chunk = a.info.chunk;
    }
  }
  if (command.isEmpty()) return GF_CMD_E_UNKNOWN;

  // Fresh: whatever the menu showed when it opened, what runs is what is
  // true now.
  auto st = Evaluate(action_id, ctx);
  if (!st.error.isEmpty()) return GF_CMD_E_BAD_ARGS;
  if (!st.visible || !st.enabled) return GF_CMD_E_DISABLED;

  // Any Blob the arguments used is spent.
  for (auto it = blobs_.begin(); it != blobs_.end();) {
    bool used = false;
    for (const auto& b : st.blobs) used = used || b.Storage() == it->Storage();
    it = used ? blobs_.erase(it) : std::next(it);
  }

  const CommandCaller caller{
      module_, caps_, QStringLiteral("lua:%1:%2").arg(chunk, action_id)};
  const auto ticket = CommandRegistry::Instance().Invoke(
      command, st.args, std::move(st.blobs), caller, CommandContextFor(ctx),
      [module = module_, command](gf::cmd::RawResult r) {
        if (r.status != GF_CMD_OK) {
          LOG_W() << "module" << module << "action ->" << command
                  << "failed:" << r.status << r.error;
        }
      });
  return ticket.status;
}

void LuaModuleRuntime::Deliver(const QString& event, const UiContext& ctx,
                               qint64 document_id) {
  if (state_ == nullptr || stopping_.load() || torn_down_) return;

  for (auto& sub : subscriptions_) {
    if (sub.info.event != event || sub.info.disabled) continue;
    if (stopping_.load()) return;

    qint64 ref_id = 0;
    if (document_id != 0) {
      ref_id = NextId();
      document_refs_.insert(
          ref_id, gf::cmd::DocumentRef{document_id, 0,
                                       ctx.document.has_value()
                                           ? ctx.document->type
                                           : QString()});
    }

    phase_ = Phase::kHANDLER;
    ctx_ = &ctx;
    source_ = QStringLiteral("lua:%1:%2").arg(sub.info.chunk, sub.info.id);
    state_->ArmCallBudget();
    const int handler = sub.handler;
    const quint64 tag = state_tag_;
    const quint64 epoch = epoch_;
    BindingOutcome out;
    const bool ok = Protected(
        state_->L(),
        [handler, tag, epoch, ref_id](lua_State* L) -> int {
          lua_rawgeti(L, LUA_REGISTRYINDEX, handler);
          PushHandle(L, HandleKind::kCONTEXT, tag, 1, epoch);
          lua_createtable(L, 0, 1);
          if (ref_id != 0) {
            PushHandle(L, HandleKind::kDOCUMENT_REF, tag, ref_id, 0);
            lua_setfield(L, -2, "document");
          }
          lua_call(L, 2, 0);
          return 0;
        },
        &out);
    if (!ok) {
      const auto why = QString::fromUtf8(out.message.data());
      if (state_ != nullptr && state_->BudgetExhausted()) {
        sub.info.disabled = true;
      }
      sub.info.last_error = why;
      RecordError(sub.info.id, why);
    }
    EndEntry();
  }
}

void LuaModuleRuntime::Complete(qint64 call_handle, gf::cmd::RawResult r) {
  if (state_ == nullptr || stopping_.load() || torn_down_) return;
  const auto call = calls_.take(call_handle);
  if (call.continuation == LUA_NOREF || call.continuation == -2) return;

  // Checked against the command's declared result before any of it reaches
  // Lua: a provider cannot hand a script something its schema did not say.
  PushTree tree;
  QString error;
  if (r.status == GF_CMD_OK) {
    const auto d = CommandRegistry::Instance().Describe(call.command);
    const auto schema = d.has_value()
                            ? d->value(QStringLiteral("result")).toMap()
                            : QCborMap{};
    const auto registrar = [this](gf::cmd::Blob b) -> qint64 {
      const auto id = NextId();
      blobs_.insert(id, std::move(b));
      return id;
    };
    if (!CborToPushTree(QCborValue(r.result), schema, r.blobs, registrar,
                        &tree, &error)) {
      LOG_W() << "module" << module_ << "command" << call.command
              << "returned a result its schema does not describe";
      tree.clear();
      error = QStringLiteral("bad_result");
    }
  } else {
    error = r.error.isEmpty() ? QStringLiteral("failed (%1)").arg(r.status)
                              : r.error;
  }

  phase_ = Phase::kHANDLER;
  ctx_ = nullptr;
  source_ = call.source;
  state_->ArmCallBudget();
  const int fn = call.continuation;
  const quint64 tag = state_tag_;
  const auto error_utf8 = error.toUtf8();
  const bool has_error = !error.isEmpty();
  BindingOutcome out;
  const bool ok = Protected(
      state_->L(),
      [fn, tag, &tree, &error_utf8, has_error](lua_State* L) -> int {
        lua_rawgeti(L, LUA_REGISTRYINDEX, fn);
        luaL_unref(L, LUA_REGISTRYINDEX, fn);
        if (!has_error && !tree.empty()) {
          PushNodeValue(L, tree, 0, tag);
        } else {
          lua_pushnil(L);
        }
        if (has_error) {
          lua_pushlstring(L, error_utf8.constData(),
                          static_cast<size_t>(error_utf8.size()));
        } else {
          lua_pushnil(L);
        }
        lua_call(L, 2, 0);
        return 0;
      },
      &out);
  if (!ok) RecordError(call.source, QString::fromUtf8(out.message.data()));
  EndEntry();
}

void LuaModuleRuntime::Teardown() {
  if (torn_down_) return;

  stopping_.store(true);
  teardown_log_ << QStringLiteral("callbacks stopped");

  for (const auto& call : calls_) {
    CommandRegistry::Instance().Cancel(call.registry_call, module_);
  }
  calls_.clear();
  teardown_log_ << QStringLiteral("calls cancelled");

  actions_.clear();
  subscriptions_.clear();
  teardown_log_ << QStringLiteral("actions and subscriptions removed");
  emit SignalChanged();

  mounts_.clear();
  teardown_log_ << QStringLiteral("mounts removed");
  emit SignalChanged();

  commands_.clear();
  anchors_.clear();
  natives_.clear();
  mount_handles_.clear();
  blobs_.clear();
  document_refs_.clear();
  key_refs_.clear();
  state_tag_ = NewTag();  // every handle already issued stops resolving
  teardown_log_ << QStringLiteral("handles invalidated");

  state_.reset();
  teardown_log_ << QStringLiteral("state closed");
  torn_down_ = true;
}

auto LuaModuleRuntime::Snapshot() const -> LuaRuntimeSnapshot {
  LuaRuntimeSnapshot s;
  s.module = module_;
  if (state_ != nullptr) {
    s.memory_used = state_->MemoryUsed();
    s.memory_limit = state_->MemoryLimit();
  }
  s.chunks = chunks_;
  s.actions = Actions();
  s.mounts = mounts_;
  s.subscriptions = Subscriptions();
  for (const auto& n : natives_) s.native_widgets.append(n.id);
  s.native_widgets.removeDuplicates();
  s.pending_calls = static_cast<int>(calls_.size());
  s.last_error = last_error_;
  s.torn_down = torn_down_;
  return s;
}

}  // namespace GpgFrontend::UI::Lua
