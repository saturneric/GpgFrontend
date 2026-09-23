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

#include "LuaApi.h"

#include <QApplication>
#include <QPalette>
#include <QPointer>
#include <QRegularExpression>

#include "LuaBinding.h"
#include "LuaHost.h"
#include "LuaModuleRuntime.h"
#include "LuaValue.h"
#include "NativeWidgetRegistry.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleSettingsPolicy.h"
#include "sdk/GFSDKHostApi.h"
#include "ui/command/CommandRegistry.h"
#include "ui/function/UIStyle.h"

/**
 * @file LuaApi.cpp
 * @brief Every function a module's UI script can call.
 *
 * Each is a LuaBinding: arguments are read without raising, or inside
 * Protected(); C++ work happens in the binding's own frame; results are
 * pushed inside Protected(); failure is reported through BindingOutcome and
 * raised by the trampoline once this frame is gone. Nothing here calls a
 * function that raises on its own -- a source test checks that.
 */

namespace GpgFrontend::UI::Lua {

namespace {

constexpr int kMaxContinuations = 32;

auto Rt(lua_State* L) -> LuaModuleRuntime* {
  auto* s = LuaState::Of(L);
  return s == nullptr ? nullptr : static_cast<LuaModuleRuntime*>(s->Owner());
}

/// A string argument, read without conversion -- and so without raising.
auto StringArg(lua_State* L, int i) -> std::optional<QString> {
  if (lua_type(L, i) != LUA_TSTRING) return std::nullopt;
  size_t n = 0;
  const char* s = lua_tolstring(L, i, &n);
  return QString::fromUtf8(s, static_cast<int>(n));
}

/// Push what @p fn pushes, as the binding's results.
template <typename F>
void Return(lua_State* L, BindingOutcome& out, F&& fn) {
  int n = 0;
  if (Protected(
          L, [&n, &fn](lua_State* S) -> int { return n = fn(S); }, &out)) {
    out.nret = n;
  }
}

void ReturnNil(lua_State* L, BindingOutcome& out) {
  Return(L, out, [](lua_State* S) -> int {
    lua_pushnil(S);
    return 1;
  });
}

void ReturnBool(lua_State* L, BindingOutcome& out, bool v) {
  Return(L, out, [v](lua_State* S) -> int {
    lua_pushboolean(S, v ? 1 : 0);
    return 1;
  });
}

void ReturnString(lua_State* L, BindingOutcome& out, const QString& s) {
  const auto utf8 = s.toUtf8();
  Return(L, out, [&utf8](lua_State* S) -> int {
    lua_pushlstring(S, utf8.constData(), static_cast<size_t>(utf8.size()));
    return 1;
  });
}

void ReturnHandle(lua_State* L, BindingOutcome& out, HandleKind kind,
                  quint64 tag, qint64 id, quint64 epoch) {
  Return(L, out, [=](lua_State* S) -> int {
    PushHandle(S, kind, tag, id, epoch);
    return 1;
  });
}

void FailWith(BindingOutcome& out, const QString& why) {
  out.Fail(why.toUtf8().constData());
}

/// Read one argument as a table, into plain nodes.
auto Flatten(lua_State* L, int index, LuaTree* tree, BindingOutcome& out)
    -> bool {
  ErrorText error{};
  bool flattened = false;
  const bool ok = Protected(
      L,
      [index, tree, &error, &flattened](lua_State* S) -> int {
        flattened = FlattenValue(S, index, tree, &error);
        return 0;
      },
      &out);
  if (!ok) return false;
  if (!flattened) {
    out.Fail(error.data());
    return false;
  }
  return true;
}

/// The named, top-level fields of a flattened table.
auto FieldsOf(const LuaTree& tree) -> QHash<QString, int> {
  QHash<QString, int> fields;
  if (tree.empty() || tree[0].type != LuaNode::Type::kTABLE) return fields;
  for (const int c : ChildrenOf(tree, 0)) {
    const auto& n = tree[static_cast<size_t>(c)];
    fields.insert(n.key_is_index ? QStringLiteral("[%1]").arg(n.key_index)
                                 : QString::fromUtf8(n.key),
                  c);
  }
  return fields;
}

auto UnknownField(const QHash<QString, int>& fields, const QStringList& known)
    -> QString {
  for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
    if (!known.contains(it.key())) return it.key();
  }
  return {};
}

auto IdIsValid(const QString& id) -> bool {
  static const QRegularExpression kId(QStringLiteral("^[a-z0-9_][a-z0-9_.\\-]*$"));
  return !id.isEmpty() && id.size() <= 64 && kId.match(id).hasMatch();
}

/// A handle of @p kind issued by this very runtime, or a failure.
auto OwnHandle(LuaModuleRuntime* rt, lua_State* L, int i, HandleKind kind,
               BindingOutcome& out, quint64 tag) -> std::optional<Handle> {
  const auto* h = HandleAt(L, i);
  if (h == nullptr || h->kind != kind) {
    FailWith(out, QStringLiteral("expected a %1")
                      .arg(QLatin1String(HandleTypeName(kind))));
    return std::nullopt;
  }
  if (h->state_tag != tag) {
    FailWith(out, QStringLiteral("a handle from another module"));
    return std::nullopt;
  }
  (void)rt;
  return *h;
}

/// A plain Lua value -> QVariant, for settings. Scalars, lists and maps.
auto NodeToVariant(const LuaTree& tree, int index, QString* error)
    -> std::optional<QVariant> {
  const auto& n = tree[static_cast<size_t>(index)];
  switch (n.type) {
    case LuaNode::Type::kNIL:
      return QVariant();
    case LuaNode::Type::kBOOL:
      return QVariant(n.boolean);
    case LuaNode::Type::kINTEGER:
      return QVariant(n.integer);
    case LuaNode::Type::kNUMBER:
      return QVariant(n.number);
    case LuaNode::Type::kSTRING:
      return QVariant(QString::fromUtf8(n.text));
    case LuaNode::Type::kHANDLE:
      *error = QStringLiteral("a handle cannot be stored");
      return std::nullopt;
    case LuaNode::Type::kTABLE:
      break;
  }
  const auto children = ChildrenOf(tree, index);
  bool list = true;
  for (int i = 0; i < children.size(); ++i) {
    const auto& c = tree[static_cast<size_t>(children[i])];
    list = list && c.key_is_index && c.key_index == i + 1;
  }
  if (list) {
    QVariantList l;
    for (const int c : children) {
      auto v = NodeToVariant(tree, c, error);
      if (!v.has_value()) return std::nullopt;
      l.append(*v);
    }
    return QVariant(l);
  }
  QVariantMap m;
  for (const int c : children) {
    const auto& child = tree[static_cast<size_t>(c)];
    if (child.key_is_index) {
      *error = QStringLiteral("a table is either a list or a map");
      return std::nullopt;
    }
    auto v = NodeToVariant(tree, c, error);
    if (!v.has_value()) return std::nullopt;
    m.insert(QString::fromUtf8(child.key), *v);
  }
  return QVariant(m);
}

/// QVariant -> nodes to push. The same shapes NodeToVariant accepts.
void VariantToPush(const QVariant& v, PushTree* out) {
  PushNode n;
  switch (v.userType()) {
    case QMetaType::Bool:
      n.type = PushNode::Type::kBOOL;
      n.boolean = v.toBool();
      break;
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
      n.type = PushNode::Type::kINTEGER;
      n.integer = v.toLongLong();
      break;
    case QMetaType::Double:
    case QMetaType::Float:
      n.type = PushNode::Type::kNUMBER;
      n.number = v.toDouble();
      break;
    case QMetaType::QVariantList:
    case QMetaType::QStringList: {
      const auto l = v.toList();
      n.type = PushNode::Type::kTABLE;
      n.children = static_cast<int>(l.size());
      out->push_back(n);
      for (int i = 0; i < l.size(); ++i) {
        const auto at = out->size();
        VariantToPush(l.at(i), out);
        (*out)[at].key_is_index = true;
        (*out)[at].key_index = i + 1;
      }
      return;
    }
    case QMetaType::QVariantMap: {
      const auto m = v.toMap();
      n.type = PushNode::Type::kTABLE;
      n.children = static_cast<int>(m.size());
      out->push_back(n);
      for (auto it = m.constBegin(); it != m.constEnd(); ++it) {
        const auto at = out->size();
        VariantToPush(it.value(), out);
        (*out)[at].key = it.key().toUtf8();
      }
      return;
    }
    default:
      if (v.isValid() && !v.isNull()) {
        n.type = PushNode::Type::kSTRING;
        n.text = v.toString().toUtf8();
      }
      break;
  }
  out->push_back(n);
}

void ReturnVariant(lua_State* L, BindingOutcome& out, const QVariant& v,
                   quint64 tag) {
  PushTree tree;
  VariantToPush(v, &tree);
  Return(L, out, [&tree, tag](lua_State* S) -> int {
    PushNodeValue(S, tree, 0, tag);
    return 1;
  });
}

auto Caller(const LuaModuleRuntime& rt, const QString& source)
    -> CommandCaller {
  return CommandCaller{rt.Module(), rt.Caps(), source};
}

constexpr const char* kPrelude = R"lua(
local raw_anchor, raw_mount_anchor = ui._anchor, ui._mount_anchor
ui._anchor, ui._mount_anchor = nil, nil
ui.anchor = setmetatable({
  settings = function(t) return raw_mount_anchor("settings", t) end,
  editor = function(t) return raw_mount_anchor("editor", t) end,
  dialog = function(t) return raw_mount_anchor("dialog", t or {}) end,
}, { __call = function(_, name) return raw_anchor(name) end,
     __metatable = false })
)lua";

}  // namespace

// ------------------------------------------------------------------ install

auto LuaApi::Install(LuaModuleRuntime& rt, QString* error) -> bool {
  const bool custom = (rt.Caps() & GF_HOST_CAP_UI_CUSTOM) != 0;
  bool prelude_ok = true;
  BindingOutcome out;
  const bool ok = Protected(
      rt.state_->L(),
      [custom, &prelude_ok](lua_State* L) -> int {
        for (int k = static_cast<int>(HandleKind::kCOMMAND);
             k <= static_cast<int>(HandleKind::kBLOB); ++k) {
          luaL_newmetatable(L, HandleTypeName(static_cast<HandleKind>(k)));
          lua_pushcfunction(L, &LuaBinding<&LuaApi::Index>);
          lua_setfield(L, -2, "__index");
          lua_pushboolean(L, 0);  // getmetatable() says nothing
          lua_setfield(L, -2, "__metatable");
          lua_pop(L, 1);
        }

        lua_createtable(L, 0, 3);
        lua_pushcfunction(L, &LuaBinding<&LuaApi::CommandsGet>);
        lua_setfield(L, -2, "get");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::CommandsState>);
        lua_setfield(L, -2, "state");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::CommandsInvoke>);
        lua_setfield(L, -2, "invoke");
        lua_setglobal(L, "commands");

        lua_createtable(L, 0, 5);
        lua_pushcfunction(L, &LuaBinding<&LuaApi::UiAction>);
        lua_setfield(L, -2, "action");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::UiMount>);
        lua_setfield(L, -2, "mount");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::UiSubscribe>);
        lua_setfield(L, -2, "subscribe");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::UiAnchor>);
        lua_setfield(L, -2, "_anchor");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::UiMountAnchor>);
        lua_setfield(L, -2, "_mount_anchor");
        lua_setglobal(L, "ui");

        lua_createtable(L, 0, 3);
        lua_pushcfunction(L, &LuaBinding<&LuaApi::StateGet>);
        lua_setfield(L, -2, "get");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::StateSet>);
        lua_setfield(L, -2, "set");
        lua_pushcfunction(L, &LuaBinding<&LuaApi::StateHost>);
        lua_setfield(L, -2, "host");
        lua_setglobal(L, "state");

        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &LuaBinding<&LuaApi::ThemeColor>);
        lua_setfield(L, -2, "color");
        lua_setglobal(L, "theme");

        // Absent, not refusing, without ui.custom: a script written for it
        // fails at the first use with "attempt to index a nil value".
        if (custom) {
          lua_createtable(L, 0, 2);
          lua_pushinteger(L, 0);
          lua_pushcclosure(L, &LuaBinding<&LuaApi::NativeRef>, 1);
          lua_setfield(L, -2, "widget");
          lua_pushinteger(L, 1);
          lua_pushcclosure(L, &LuaBinding<&LuaApi::NativeRef>, 1);
          lua_setfield(L, -2, "factory");
          lua_setglobal(L, "native");
        }

        if (luaL_loadbufferx(L, kPrelude, std::strlen(kPrelude), "=ui",
                             "t") != LUA_OK) {
          prelude_ok = false;
          return 0;
        }
        lua_call(L, 0, 0);
        return 0;
      },
      &out);
  if (!ok || !prelude_ok) {
    if (error != nullptr) {
      *error = ok ? QStringLiteral("the UI prelude did not load")
                  : QString::fromUtf8(out.message.data());
    }
    return false;
  }
  return true;
}

// ------------------------------------------------------------------ commands

void LuaApi::CommandsGet(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto id = StringArg(L, 1);
  if (rt == nullptr || !id.has_value()) {
    return FailWith(out, QStringLiteral("commands.get takes a command id"));
  }
  const auto d = CommandRegistry::Instance().Describe(*id);
  if (!d.has_value()) {
    return FailWith(out, QStringLiteral("unknown command: %1").arg(*id));
  }
  const auto flags =
      static_cast<uint32_t>(d->value(QStringLiteral("flags")).toInteger());
  const auto required = static_cast<uint32_t>(
      d->value(QStringLiteral("required_caps")).toInteger());
  if ((flags & gf::cmd::kHostOnly) != 0) {
    return FailWith(out,
                    QStringLiteral("%1 is the Host's own command").arg(*id));
  }
  if ((rt->caps_ & required) != required) {
    return FailWith(out, QStringLiteral("%1 needs a capability this module "
                                        "does not declare")
                             .arg(*id));
  }
  const auto h = rt->NextId();
  rt->commands_.insert(h, *id);
  ReturnHandle(L, out, HandleKind::kCOMMAND, rt->state_tag_, h, 0);
}

void LuaApi::CommandsState(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  const auto h = OwnHandle(rt, L, 1, HandleKind::kCOMMAND, out, rt->state_tag_);
  if (!h.has_value()) return;
  const auto bits = CommandRegistry::Instance().State(
      rt->commands_.value(h->id), Caller(*rt, rt->source_),
      rt->ctx_ != nullptr ? rt->CommandContextFor(*rt->ctx_)
                          : gf::cmd::CommandContext{});
  Return(L, out, [bits](lua_State* S) -> int {
    lua_createtable(S, 0, 3);
    lua_pushboolean(S, (bits & GF_CMD_STATE_ENABLED) != 0 ? 1 : 0);
    lua_setfield(S, -2, "enabled");
    lua_pushboolean(S, (bits & GF_CMD_STATE_VISIBLE) != 0 ? 1 : 0);
    lua_setfield(S, -2, "visible");
    lua_pushboolean(S, (bits & GF_CMD_STATE_CHECKED) != 0 ? 1 : 0);
    lua_setfield(S, -2, "checked");
    return 1;
  });
}

void LuaApi::CommandFlag(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  // Which flag is the closure's upvalue, read here -- inside Protected() it
  // would be the thunk's (none).
  const auto which = lua_tointegerx(L, lua_upvalueindex(1), nullptr);
  const auto h = OwnHandle(rt, L, 1, HandleKind::kCOMMAND, out, rt->state_tag_);
  if (!h.has_value()) return;
  const auto bits = CommandRegistry::Instance().State(
      rt->commands_.value(h->id), Caller(*rt, rt->source_),
      rt->ctx_ != nullptr ? rt->CommandContextFor(*rt->ctx_)
                          : gf::cmd::CommandContext{});
  const uint32_t mask = which == 0   ? GF_CMD_STATE_ENABLED
                        : which == 1 ? GF_CMD_STATE_VISIBLE
                                     : GF_CMD_STATE_CHECKED;
  ReturnBool(L, out, (bits & mask) != 0);
}

void LuaApi::CommandsInvoke(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if (rt->phase_ != LuaModuleRuntime::Phase::kHANDLER) {
    return FailWith(
        out, rt->phase_ == LuaModuleRuntime::Phase::kPURE
                 ? QStringLiteral("not allowed in update: update() computes "
                                  "state; the Host invokes the command when "
                                  "the user activates the action")
                 : QStringLiteral("commands.invoke runs from event handlers "
                                  "and continuations"));
  }
  const auto h = OwnHandle(rt, L, 1, HandleKind::kCOMMAND, out, rt->state_tag_);
  if (!h.has_value()) return;
  const auto command = rt->commands_.value(h->id);

  LuaTree tree;
  if (lua_type(L, 2) != LUA_TNONE && lua_type(L, 2) != LUA_TNIL) {
    if (!Flatten(L, 2, &tree, out)) return;
  }

  int continuation = LUA_NOREF;
  if (lua_type(L, 3) == LUA_TFUNCTION) {
    int owed = 0;
    for (const auto& c : rt->calls_) {
      if (c.continuation != LUA_NOREF) ++owed;
    }
    if (owed >= kMaxContinuations) {
      return FailWith(out, QStringLiteral("too many calls awaiting a result"));
    }
    if (!Protected(
            L,
            [&continuation](lua_State* S) -> int {
              lua_pushvalue(S, 3);
              continuation = luaL_ref(S, LUA_REGISTRYINDEX);
              return 0;
            },
            &out)) {
      return;
    }
  }
  const auto drop_continuation = [L, &continuation]() {
    if (continuation == LUA_NOREF) return;
    Protected(L, [continuation](lua_State* S) -> int {
      luaL_unref(S, LUA_REGISTRYINDEX, continuation);
      return 0;
    });
  };

  const auto d = CommandRegistry::Instance().Describe(command);
  if (!d.has_value()) {
    drop_continuation();
    return FailWith(out, QStringLiteral("the command went away"));
  }
  const auto schema = d->value(QStringLiteral("args")).toMap();
  std::vector<gf::cmd::Blob> blobs;
  QCborMap args;
  if (!tree.empty() ||
      !schema.value(QStringLiteral("fields")).toArray().isEmpty()) {
    QString error;
    const auto resolve = [rt](const Handle& hh,
                              std::vector<gf::cmd::Blob>* b,
                              QString* e) { return rt->Resolve(hh, b, e); };
    const auto v = NodeToCbor(tree, tree.empty() ? -1 : 0, schema, resolve,
                              &blobs, &error);
    if (!v.has_value()) {
      drop_continuation();
      return FailWith(out, QStringLiteral("%1: %2").arg(command, error));
    }
    args = v->toMap();
  }

  // The call is attributed to the module, and goes through exactly the path
  // a C++ module or the Host's own menu takes: the Lua layer converted the
  // arguments and decides nothing else.
  const auto handle = rt->NextId();
  QPointer<LuaModuleRuntime> guard(rt);
  const auto ticket = CommandRegistry::Instance().Invoke(
      command, args, blobs, Caller(*rt, rt->source_),
      rt->ctx_ != nullptr ? rt->CommandContextFor(*rt->ctx_)
                          : gf::cmd::CommandContext{},
      [guard, handle](gf::cmd::RawResult r) {
        auto* target = guard.data();
        if (target == nullptr) return;
        // Always later, on the runtime's own thread: a result never enters
        // Lua from inside another entry, nor from another thread.
        QMetaObject::invokeMethod(
            target,
            [guard, handle, r = std::move(r)]() mutable {
              if (!guard.isNull()) guard->Complete(handle, std::move(r));
            },
            Qt::QueuedConnection);
      });

  if (ticket.status != GF_CMD_OK) {
    drop_continuation();
    const auto why = QString::number(ticket.status);
    return Return(L, out, [why](lua_State* S) -> int {
      lua_pushnil(S);
      lua_pushstring(S, why.toUtf8().constData());
      return 2;
    });
  }

  // Spent: a Blob moves with the call.
  for (auto it = rt->blobs_.begin(); it != rt->blobs_.end();) {
    bool used = false;
    for (const auto& b : blobs) used = used || b.Storage() == it->Storage();
    it = used ? rt->blobs_.erase(it) : std::next(it);
  }
  rt->calls_.insert(handle, LuaModuleRuntime::PendingCall{
                                ticket.call_id, continuation, command,
                                rt->source_});
  ReturnHandle(L, out, HandleKind::kCALL, rt->state_tag_, handle, 0);
}

void LuaApi::CallCancel(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  const auto h = OwnHandle(rt, L, 1, HandleKind::kCALL, out, rt->state_tag_);
  if (!h.has_value()) return;
  const auto it = rt->calls_.constFind(h->id);
  if (it == rt->calls_.constEnd()) return ReturnBool(L, out, false);
  const auto call = *it;
  rt->calls_.remove(h->id);
  CommandRegistry::Instance().Cancel(call.registry_call, rt->Module());
  if (call.continuation != LUA_NOREF) {
    Protected(L, [ref = call.continuation](lua_State* S) -> int {
      luaL_unref(S, LUA_REGISTRYINDEX, ref);
      return 0;
    });
  }
  ReturnBool(L, out, true);
}

// ------------------------------------------------------------------ ui

void LuaApi::UiAnchor(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto name = StringArg(L, 1);
  if (rt == nullptr || !name.has_value()) {
    return FailWith(out, QStringLiteral("ui.anchor takes an anchor id"));
  }
  const auto* spec = FindAnchor(*name);
  if (spec == nullptr || (spec->kind != AnchorKind::kMENU &&
                          spec->kind != AnchorKind::kBUTTONS)) {
    QStringList valid;
    for (const auto* a : AllAnchors()) {
      if (a->kind == AnchorKind::kMENU || a->kind == AnchorKind::kBUTTONS) {
        valid << QLatin1String(a->id);
      }
    }
    return FailWith(out, QStringLiteral("unknown anchor \"%1\"; actions "
                                        "attach to: %2")
                             .arg(*name, valid.join(", ")));
  }
  if (spec->deprecated_since != 0) {
    LOG_W() << "module" << rt->Module() << "uses deprecated anchor" << *name;
  }
  const auto h = rt->NextId();
  rt->anchors_.insert(h, LuaModuleRuntime::AnchorRef{spec, {}, {}, {}});
  ReturnHandle(L, out, HandleKind::kANCHOR, rt->state_tag_, h, 0);
}

void LuaApi::UiMountAnchor(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto kind = StringArg(L, 1);
  if (rt == nullptr || !kind.has_value()) return FailWith(out, "bad anchor");
  const auto* spec = FindAnchor(*kind);
  if (spec == nullptr) return FailWith(out, "bad anchor");

  LuaTree tree;
  if (!Flatten(L, 2, &tree, out)) return;
  const auto fields = FieldsOf(tree);
  LuaModuleRuntime::AnchorRef ref{spec, {}, {}, {}};

  const auto text = [&](const char* key) -> QString {
    const auto it = fields.constFind(QLatin1String(key));
    if (it == fields.constEnd()) return {};
    const auto& n = tree[static_cast<size_t>(*it)];
    return n.type == LuaNode::Type::kSTRING ? QString::fromUtf8(n.text)
                                            : QString();
  };

  QStringList known;
  if (spec->kind == AnchorKind::kSETTINGS) {
    known = {"section"};
    ref.section = text("section");
    if (!IdIsValid(ref.section)) {
      return FailWith(out, "ui.anchor.settings{section = \"...\"}");
    }
  } else if (spec->kind == AnchorKind::kEDITOR) {
    known = {"document_type", "extensions"};
    ref.document_type = text("document_type");
    if (!IdIsValid(ref.document_type)) {
      return FailWith(out, "ui.anchor.editor{document_type = \"...\"}");
    }
    const auto it = fields.constFind(QStringLiteral("extensions"));
    if (it != fields.constEnd()) {
      for (const int c : ChildrenOf(tree, *it)) {
        const auto& n = tree[static_cast<size_t>(c)];
        static const QRegularExpression kExt(QStringLiteral("^[a-z0-9]{1,16}$"));
        const auto ext = QString::fromUtf8(n.text);
        if (n.type != LuaNode::Type::kSTRING || !kExt.match(ext).hasMatch()) {
          return FailWith(out, "extensions are lower-case, without the dot");
        }
        ref.extensions.append(ext);
      }
    }
  }
  const auto unknown = UnknownField(fields, known);
  if (!unknown.isEmpty()) {
    return FailWith(out, QStringLiteral("ui.anchor.%1: unknown field \"%2\"")
                             .arg(*kind, unknown));
  }
  const auto h = rt->NextId();
  rt->anchors_.insert(h, ref);
  ReturnHandle(L, out, HandleKind::kANCHOR, rt->state_tag_, h, 0);
}

void LuaApi::UiAction(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if (rt->phase_ != LuaModuleRuntime::Phase::kLOAD) {
    return FailWith(out, QStringLiteral("not allowed in update: ui.action "
                                        "registers while the script loads"));
  }

  // The fields, read raw; `update` becomes a registry reference.
  LuaTree tree;
  int update = LUA_NOREF;
  bool update_not_function = false;
  if (lua_type(L, 1) != LUA_TTABLE) return FailWith(out, "ui.action{...}");
  if (!Protected(
          L,
          [&update, &update_not_function](lua_State* S) -> int {
            lua_getfield(S, 1, "update");  // a plain table: no metamethod
            if (lua_type(S, -1) == LUA_TFUNCTION) {
              update = luaL_ref(S, LUA_REGISTRYINDEX);
            } else {
              update_not_function = lua_type(S, -1) != LUA_TNIL;
              lua_pop(S, 1);
            }
            lua_pushnil(S);
            lua_setfield(S, 1, "update");  // so the table flattens
            return 0;
          },
          &out)) {
    return;
  }
  const auto drop_update = [L, &update]() {
    if (update == LUA_NOREF) return;
    Protected(L, [update](lua_State* S) -> int {
      luaL_unref(S, LUA_REGISTRYINDEX, update);
      return 0;
    });
  };
  const auto fail = [&](const QString& why) {
    drop_update();
    FailWith(out, QStringLiteral("ui.action: ") + why);
  };
  if (update_not_function) return fail("\"update\" must be a function");
  if (!Flatten(L, 1, &tree, out)) return drop_update();

  const auto fields = FieldsOf(tree);
  const auto unknown =
      UnknownField(fields, {"id", "anchor", "command", "order", "icon"});
  if (!unknown.isEmpty()) {
    return fail(QStringLiteral("unknown field \"%1\"").arg(unknown));
  }
  const auto node = [&](const char* key) -> const LuaNode* {
    const auto it = fields.constFind(QLatin1String(key));
    return it == fields.constEnd() ? nullptr
                                   : &tree[static_cast<size_t>(*it)];
  };

  const auto* id_node = node("id");
  const auto id = id_node != nullptr && id_node->type == LuaNode::Type::kSTRING
                      ? QString::fromUtf8(id_node->text)
                      : QString();
  if (!IdIsValid(id)) return fail("\"id\" is a lower-case name");
  const auto full_id = rt->Module() + "." + id;
  for (const auto& a : rt->actions_) {
    if (a.info.id == full_id) return fail("\"" + id + "\" is already taken");
  }

  const auto* anchor = node("anchor");
  if (anchor == nullptr || anchor->type != LuaNode::Type::kHANDLE ||
      anchor->handle.kind != HandleKind::kANCHOR ||
      anchor->handle.state_tag != rt->state_tag_) {
    return fail("\"anchor\" is a ui.anchor(...)");
  }
  const auto aref = rt->anchors_.value(anchor->handle.id);
  if (aref.spec == nullptr || (aref.spec->kind != AnchorKind::kMENU &&
                               aref.spec->kind != AnchorKind::kBUTTONS)) {
    return fail("actions attach to menu and button anchors");
  }

  const auto* command = node("command");
  if (command == nullptr || command->type != LuaNode::Type::kHANDLE ||
      command->handle.kind != HandleKind::kCOMMAND ||
      command->handle.state_tag != rt->state_tag_) {
    return fail("\"command\" is a commands.get(...)");
  }
  const auto command_id = rt->commands_.value(command->handle.id);

  int order = 0;
  if (const auto* o = node("order")) {
    if (o->type != LuaNode::Type::kINTEGER) return fail("\"order\" is an integer");
    order = static_cast<int>(o->integer);
  }
  QString icon;
  if (const auto* i = node("icon")) {
    icon = QString::fromUtf8(i->text);
    if (i->type != LuaNode::Type::kSTRING || !icon.startsWith(":/")) {
      return fail("\"icon\" is a resource path, \":/...\"");
    }
  }

  LuaModuleRuntime::ActionEntry entry;
  entry.info.id = full_id;
  entry.info.anchor = QLatin1String(aref.spec->id);
  entry.info.command = command_id;
  entry.info.order = order;
  entry.info.icon = icon;
  entry.info.chunk = rt->chunk_;
  entry.update = update;
  rt->actions_.append(entry);
  out.nret = 0;
}

void LuaApi::UiMount(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if ((rt->caps_ & GF_HOST_CAP_UI_CUSTOM) == 0) {
    return FailWith(out, QStringLiteral("ui.mount needs the ui.custom "
                                        "capability"));
  }
  if (rt->phase_ != LuaModuleRuntime::Phase::kLOAD) {
    return FailWith(out, QStringLiteral("not allowed in update: ui.mount "
                                        "registers while the script loads"));
  }
  LuaTree tree;
  if (lua_type(L, 1) != LUA_TTABLE) return FailWith(out, "ui.mount{...}");
  if (!Flatten(L, 1, &tree, out)) return;
  const auto fields = FieldsOf(tree);
  const auto fail = [&out](const QString& why) {
    FailWith(out, QStringLiteral("ui.mount: ") + why);
  };
  const auto unknown = UnknownField(fields, {"id", "anchor", "widget", "order"});
  if (!unknown.isEmpty()) {
    return fail(QStringLiteral("unknown field \"%1\"").arg(unknown));
  }
  const auto node = [&](const char* key) -> const LuaNode* {
    const auto it = fields.constFind(QLatin1String(key));
    return it == fields.constEnd() ? nullptr
                                   : &tree[static_cast<size_t>(*it)];
  };

  const auto* id_node = node("id");
  const auto id = id_node != nullptr && id_node->type == LuaNode::Type::kSTRING
                      ? QString::fromUtf8(id_node->text)
                      : QString();
  if (!IdIsValid(id)) return fail("\"id\" is a lower-case name");
  const auto full_id = rt->Module() + "." + id;
  for (const auto& m : rt->mounts_) {
    if (m.id == full_id) return fail("\"" + id + "\" is already taken");
  }

  const auto* anchor = node("anchor");
  if (anchor == nullptr || anchor->type != LuaNode::Type::kHANDLE ||
      anchor->handle.kind != HandleKind::kANCHOR ||
      anchor->handle.state_tag != rt->state_tag_) {
    return fail("\"anchor\" is a ui.anchor.settings/editor/dialog{...}");
  }
  const auto aref = rt->anchors_.value(anchor->handle.id);
  if (aref.spec == nullptr || aref.spec->kind == AnchorKind::kMENU ||
      aref.spec->kind == AnchorKind::kBUTTONS) {
    return fail("widgets mount on settings, editor and dialog anchors");
  }

  const auto* widget = node("widget");
  if (widget == nullptr || widget->type != LuaNode::Type::kHANDLE ||
      (widget->handle.kind != HandleKind::kNATIVE_WIDGET &&
       widget->handle.kind != HandleKind::kNATIVE_FACTORY) ||
      widget->handle.state_tag != rt->state_tag_) {
    return fail("\"widget\" is a native.widget(...) or native.factory(...)");
  }
  const auto native = rt->natives_.value(widget->handle.id);
  const auto entry = NativeWidgetRegistry::Instance().Find(native.id);
  if (!entry.has_value() || entry->owner != rt->Module()) {
    return fail("that native widget is no longer registered");
  }

  // The widget's typed interface has to be the one the container talks to.
  const auto want =
      aref.spec->kind == AnchorKind::kEDITOR     ? NativeWidgetKind::kDOCUMENT
      : aref.spec->kind == AnchorKind::kSETTINGS ? NativeWidgetKind::kSETTINGS
                                                 : NativeWidgetKind::kDIALOG;
  if (entry->kind != want) {
    return fail("that widget does not implement what this anchor needs");
  }
  if (aref.spec->kind == AnchorKind::kEDITOR && !native.factory) {
    return fail("an editor needs a native.factory: one widget per document");
  }
  if (aref.spec->kind == AnchorKind::kEDITOR) {
    const auto owner = LuaHost::Instance().EditorOwner(aref.document_type);
    if (!owner.isEmpty() && owner != rt->Module()) {
      return fail(QStringLiteral("document type \"%1\" already belongs to %2")
                      .arg(aref.document_type, owner));
    }
  }

  int order = 0;
  if (const auto* o = node("order")) {
    if (o->type != LuaNode::Type::kINTEGER) return fail("\"order\" is an integer");
    order = static_cast<int>(o->integer);
  }

  MountInfo m;
  m.id = full_id;
  m.kind = aref.spec->kind;
  m.widget = native.id;
  m.factory = native.factory;
  m.section = aref.section;
  m.document_type = aref.document_type;
  m.extensions = aref.extensions;
  m.order = order;
  m.chunk = rt->chunk_;
  rt->mounts_.append(m);
  const auto h = rt->NextId();
  rt->mount_handles_.insert(h, m);
  ReturnHandle(L, out, HandleKind::kMOUNT, rt->state_tag_, h, 0);
}

void LuaApi::UiSubscribe(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if (rt->phase_ != LuaModuleRuntime::Phase::kLOAD) {
    return FailWith(out, QStringLiteral("ui.subscribe registers while the "
                                        "script loads"));
  }
  if (lua_type(L, 1) != LUA_TTABLE) return FailWith(out, "ui.subscribe{...}");

  int handler = LUA_NOREF;
  if (!Protected(
          L,
          [&handler](lua_State* S) -> int {
            lua_getfield(S, 1, "handler");
            if (lua_type(S, -1) == LUA_TFUNCTION) {
              handler = luaL_ref(S, LUA_REGISTRYINDEX);
            } else {
              lua_pop(S, 1);
            }
            lua_pushnil(S);
            lua_setfield(S, 1, "handler");
            return 0;
          },
          &out)) {
    return;
  }
  const auto fail = [&](const QString& why) {
    if (handler != LUA_NOREF) {
      Protected(L, [handler](lua_State* S) -> int {
        luaL_unref(S, LUA_REGISTRYINDEX, handler);
        return 0;
      });
    }
    FailWith(out, QStringLiteral("ui.subscribe: ") + why);
  };
  if (handler == LUA_NOREF) return fail("\"handler\" must be a function");

  LuaTree tree;
  if (!Flatten(L, 1, &tree, out)) return fail(QString::fromUtf8(out.message.data()));
  const auto fields = FieldsOf(tree);
  const auto unknown = UnknownField(fields, {"event", "id"});
  if (!unknown.isEmpty()) {
    return fail(QStringLiteral("unknown field \"%1\"").arg(unknown));
  }
  const auto ev = fields.value(QStringLiteral("event"), -1);
  const auto event = ev >= 0 ? QString::fromUtf8(tree[static_cast<size_t>(ev)].text)
                             : QString();
  if (!LuaModuleRuntime::Events().contains(event)) {
    return fail(QStringLiteral("\"%1\" is not a UI event; the events are: %2")
                    .arg(event, LuaModuleRuntime::Events().join(", ")));
  }
  const auto idn = fields.value(QStringLiteral("id"), -1);
  auto id = idn >= 0 ? QString::fromUtf8(tree[static_cast<size_t>(idn)].text)
                     : QString("subscription%1").arg(rt->subscriptions_.size() + 1);
  if (!IdIsValid(id)) return fail("\"id\" is a lower-case name");

  LuaModuleRuntime::SubscriptionEntry entry;
  entry.info.id = rt->Module() + "." + id;
  entry.info.event = event;
  entry.info.chunk = rt->chunk_;
  entry.handler = handler;
  rt->subscriptions_.append(entry);
  out.nret = 0;
}

void LuaApi::NativeRef(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  const bool factory = lua_tointegerx(L, lua_upvalueindex(1), nullptr) == 1;
  const auto id = StringArg(L, 1);
  if (!id.has_value() || !IdIsValid(*id)) {
    return FailWith(out, "native.widget/factory take the registered name");
  }
  const auto full = rt->Module() + "." + *id;
  const auto entry = NativeWidgetRegistry::Instance().Find(full);
  if (!entry.has_value() || entry->owner != rt->Module()) {
    return FailWith(out, QStringLiteral("no native widget \"%1\" is "
                                        "registered by this module")
                             .arg(*id));
  }
  if (entry->multi_instance != factory) {
    return FailWith(out, factory ? QStringLiteral("\"%1\" is a single widget: "
                                                  "use native.widget")
                                       .arg(*id)
                                 : QStringLiteral("\"%1\" is a factory: use "
                                                  "native.factory")
                                       .arg(*id));
  }
  const auto h = rt->NextId();
  rt->natives_.insert(h, LuaModuleRuntime::NativeRef{full, factory});
  ReturnHandle(L, out,
               factory ? HandleKind::kNATIVE_FACTORY
                       : HandleKind::kNATIVE_WIDGET,
               rt->state_tag_, h, 0);
}

// ------------------------------------------------------------------ helpers

void LuaApi::StateGet(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if ((rt->caps_ & GF_HOST_CAP_STORAGE) == 0) {
    return FailWith(out, "state.get needs the storage capability");
  }
  const auto key = StringArg(L, 1);
  const auto full = key.has_value()
                        ? Module::ResolveModuleSettingKey(
                              rt->Module(), Module::ModuleSettingScope::kMODULE,
                              *key, false)
                        : QString();
  if (full.isEmpty()) return FailWith(out, "state.get: not a valid key");
  const auto settings = GetSettings();
  if (!settings.contains(full)) {
    return Return(L, out, [](lua_State* S) -> int {
      lua_pushvalue(S, 2);  // the default, or nil
      return 1;
    });
  }
  ReturnVariant(L, out, settings.value(full), rt->state_tag_);
}

void LuaApi::StateSet(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if (rt->phase_ == LuaModuleRuntime::Phase::kPURE) {
    return FailWith(out, "not allowed in update: state.set changes state");
  }
  if ((rt->caps_ & GF_HOST_CAP_STORAGE) == 0) {
    return FailWith(out, "state.set needs the storage capability");
  }
  const auto key = StringArg(L, 1);
  const auto full = key.has_value()
                        ? Module::ResolveModuleSettingKey(
                              rt->Module(), Module::ModuleSettingScope::kMODULE,
                              *key, true)
                        : QString();
  if (full.isEmpty()) return FailWith(out, "state.set: not a valid key");
  LuaTree tree;
  if (!Flatten(L, 2, &tree, out)) return;
  QString error;
  const auto value = NodeToVariant(tree, 0, &error);
  if (!value.has_value()) return FailWith(out, "state.set: " + error);
  auto settings = GetSettings();
  if (value->isValid()) {
    settings.setValue(full, *value);
  } else {
    settings.remove(full);
  }
  out.nret = 0;
}

void LuaApi::StateHost(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  if (rt == nullptr) return FailWith(out, "no runtime");
  if ((rt->caps_ & GF_HOST_CAP_STORAGE) == 0) {
    return FailWith(out, "state.host needs the storage capability");
  }
  const auto key = StringArg(L, 1);
  const auto full = key.has_value()
                        ? Module::ResolveModuleSettingKey(
                              rt->Module(), Module::ModuleSettingScope::kHOST,
                              *key, false)
                        : QString();
  if (full.isEmpty()) {
    return FailWith(out, "state.host: not a setting the Host shares");
  }
  const auto settings = GetSettings();
  if (!settings.contains(full)) return ReturnNil(L, out);
  ReturnVariant(L, out, settings.value(full), rt->state_tag_);
}

void LuaApi::ThemeColor(lua_State* L, BindingOutcome& out) {
  const auto role = StringArg(L, 1);
  const auto p = QApplication::palette();
  QColor c;
  if (role == QStringLiteral("muted_text")) {
    c = MutedTextColor(p);
  } else if (role == QStringLiteral("border")) {
    c = BorderColor(p);
  } else if (role == QStringLiteral("warning")) {
    c = WarningColor(p);
  } else if (role == QStringLiteral("danger")) {
    c = DangerColor(p);
  } else if (role == QStringLiteral("accent_positive")) {
    c = AccentColor(p, true);
  } else if (role == QStringLiteral("accent_negative")) {
    c = AccentColor(p, false);
  } else {
    return FailWith(out, "theme.color: muted_text, border, warning, danger, "
                         "accent_positive or accent_negative");
  }
  const auto rgba = static_cast<lua_Integer>(c.rgba());
  Return(L, out, [rgba](lua_State* S) -> int {
    lua_pushinteger(S, rgba);
    return 1;
  });
}

// ------------------------------------------------------------------ handles

void LuaApi::Index(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto* h = HandleAt(L, 1);
  const auto key = StringArg(L, 2);
  if (rt == nullptr || h == nullptr || !key.has_value() ||
      h->state_tag != rt->state_tag_) {
    return ReturnNil(L, out);
  }
  const Handle self = *h;
  const bool live = self.epoch == rt->epoch_ && rt->ctx_ != nullptr;
  const auto fn = [&](lua_CFunction f) {
    Return(L, out, [f](lua_State* S) -> int {
      lua_pushcfunction(S, f);
      return 1;
    });
  };

  switch (self.kind) {
    case HandleKind::kCONTEXT:
      // Good for the one call it was passed to, and nothing after.
      if (!live) return ReturnNil(L, out);
      if (*key == "document") {
        if (!rt->ctx_->document) return ReturnNil(L, out);
        return ReturnHandle(L, out, HandleKind::kDOCUMENT, rt->state_tag_, 1,
                            rt->epoch_);
      }
      if (*key == "key") {
        if (!rt->ctx_->key) return ReturnNil(L, out);
        return ReturnHandle(L, out, HandleKind::kKEY, rt->state_tag_, 1,
                            rt->epoch_);
      }
      if (*key == "has_selection") {
        return fn(&LuaBinding<&LuaApi::ContextHasSelection>);
      }
      return ReturnNil(L, out);

    case HandleKind::kDOCUMENT: {
      if (!live || !rt->ctx_->document) return ReturnNil(L, out);
      const auto& d = *rt->ctx_->document;
      if (*key == "type") return ReturnString(L, out, d.type);
      if (*key == "modified") return ReturnBool(L, out, d.modified);
      if (*key == "has_openpgp") {
        return fn(&LuaBinding<&LuaApi::DocumentHasOpenPgp>);
      }
      if (*key == "ref") return fn(&LuaBinding<&LuaApi::MakeRef>);
      return ReturnNil(L, out);
    }

    case HandleKind::kKEY: {
      if (!live || !rt->ctx_->key) return ReturnNil(L, out);
      const auto& k = *rt->ctx_->key;
      if (*key == "fingerprint") return ReturnString(L, out, k.fingerprint);
      if (*key == "key_id") return ReturnString(L, out, k.key_id);
      if (*key == "has_secret") return ReturnBool(L, out, k.has_secret);
      if (*key == "channel") {
        const auto ch = static_cast<lua_Integer>(k.channel);
        return Return(L, out, [ch](lua_State* S) -> int {
          lua_pushinteger(S, ch);
          return 1;
        });
      }
      if (*key == "ref") return fn(&LuaBinding<&LuaApi::MakeRef>);
      return ReturnNil(L, out);
    }

    case HandleKind::kCOMMAND: {
      if (*key == "id") {
        return ReturnString(L, out, rt->commands_.value(self.id));
      }
      const lua_Integer which = *key == "enabled"   ? 0
                                : *key == "visible" ? 1
                                : *key == "checked" ? 2
                                                    : -1;
      if (which < 0) return ReturnNil(L, out);
      return Return(L, out, [which](lua_State* S) -> int {
        lua_pushinteger(S, which);
        lua_pushcclosure(S, &LuaBinding<&LuaApi::CommandFlag>, 1);
        return 1;
      });
    }

    case HandleKind::kCALL:
      if (*key == "cancel") return fn(&LuaBinding<&LuaApi::CallCancel>);
      return ReturnNil(L, out);

    case HandleKind::kMOUNT:
      if (*key == "id") {
        return ReturnString(L, out, rt->mount_handles_.value(self.id).id);
      }
      return ReturnNil(L, out);

    default:
      return ReturnNil(L, out);  // a handle with no fields: opaque
  }
}

void LuaApi::ContextHasSelection(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto* h = HandleAt(L, 1);
  const bool live = rt != nullptr && h != nullptr &&
                    h->kind == HandleKind::kCONTEXT &&
                    h->state_tag == rt->state_tag_ && h->epoch == rt->epoch_ &&
                    rt->ctx_ != nullptr;
  ReturnBool(L, out, live && rt->ctx_->has_selection);
}

void LuaApi::DocumentHasOpenPgp(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto* h = HandleAt(L, 1);
  const bool live = rt != nullptr && h != nullptr &&
                    h->kind == HandleKind::kDOCUMENT &&
                    h->state_tag == rt->state_tag_ && h->epoch == rt->epoch_ &&
                    rt->ctx_ != nullptr && rt->ctx_->document.has_value();
  ReturnBool(L, out,
             live && rt->ctx_->document->has_openpgp &&
                 rt->ctx_->document->has_openpgp());
}

void LuaApi::MakeRef(lua_State* L, BindingOutcome& out) {
  auto* rt = Rt(L);
  const auto* h = HandleAt(L, 1);
  if (rt == nullptr || h == nullptr || h->state_tag != rt->state_tag_ ||
      h->epoch != rt->epoch_ || rt->ctx_ == nullptr) {
    return ReturnNil(L, out);
  }
  const auto id = rt->NextId();
  if (h->kind == HandleKind::kDOCUMENT && rt->ctx_->document) {
    rt->document_refs_.insert(
        id, gf::cmd::DocumentRef{rt->ctx_->document->id, 0,
                                 rt->ctx_->document->type});
    return ReturnHandle(L, out, HandleKind::kDOCUMENT_REF, rt->state_tag_, id,
                        0);
  }
  if (h->kind == HandleKind::kKEY && rt->ctx_->key) {
    rt->key_refs_.insert(id, *rt->ctx_->key);
    return ReturnHandle(L, out, HandleKind::kKEY_REF, rt->state_tag_, id, 0);
  }
  ReturnNil(L, out);
}

// ------------------------------------------------------------------ reference

auto LuaApiReference() -> QString {
  return QStringLiteral(R"(Lua UI API v1

commands.get(id) -> Command                       command must exist; caps checked
commands.state(Command) -> {enabled, visible, checked}
commands.invoke(Command, args[, fn(result, err)]) -> Call | nil, status
                                                  handlers and continuations only
Command:enabled() Command:visible() Command:checked() Command.id
Call:cancel() -> bool

ui.anchor(id) -> Anchor                           menu and button anchors
ui.anchor.settings{section} ui.anchor.editor{document_type, extensions}
ui.anchor.dialog{} -> Anchor                      mount anchors
ui.action{id, anchor, command, order?, icon?, update?}     while loading
  update(ctx) -> {visible?, enabled?, checked?, args?}      pure
ui.mount{id, anchor, widget, order?} -> Mount     ui.custom; while loading
ui.subscribe{event, handler, id?}                 while loading
  events: document.activated document.state_changed document.saved
          key_database.refreshed app.ui_ready
native.widget(name) native.factory(name)          ui.custom only

state.get(key[, default]) state.set(key, value)   storage; module's own group
state.host(key)                                   storage; shared Host keys only
theme.color(role) -> 0xAARRGGBB

handle lifetimes
  Command Anchor Mount NativeWidget NativeFactory  until the module unloads
  Context Document Key                             one call only
  DocumentRef KeyRef                               until the module unloads
  Call                                             until it completes or is cancelled
  Blob                                             until used as an argument
Context: .document .key :has_selection()
Document: .type .modified :has_openpgp() :ref()
Key: .fingerprint .key_id .has_secret .channel :ref()
)");
}

}  // namespace GpgFrontend::UI::Lua
