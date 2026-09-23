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

#include "LuaValue.h"

#include <cmath>
#include <cstring>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
}

namespace GpgFrontend::UI::Lua {

namespace {

constexpr int kMaxDepth = 8;
constexpr size_t kMaxNodes = 512;

void SetError(ErrorText* e, const char* text) {
  if (e == nullptr) return;
  std::strncpy(e->data(), text, e->size() - 1);
  e->back() = '\0';
}

/**
 * Plain locals only: this runs inside Protected() and calls functions that
 * may raise. Nodes are appended by value -- the temporary is gone before the
 * next Lua call -- and every other local is an int or a pointer.
 */
auto FlattenAt(lua_State* L, int abs, LuaTree* out, int parent, int depth,
               ErrorText* error) -> bool {
  if (depth > kMaxDepth) {
    SetError(error, "value nested too deeply");
    return false;
  }
  if (out->size() >= kMaxNodes) {
    SetError(error, "value too large");
    return false;
  }

  out->push_back(LuaNode{});
  const int self = static_cast<int>(out->size()) - 1;
  (*out)[self].parent = parent;

  switch (lua_type(L, abs)) {
    case LUA_TNIL:
      return true;
    case LUA_TBOOLEAN:
      (*out)[self].type = LuaNode::Type::kBOOL;
      (*out)[self].boolean = lua_toboolean(L, abs) != 0;
      return true;
    case LUA_TNUMBER:
      if (lua_isinteger(L, abs) != 0) {
        (*out)[self].type = LuaNode::Type::kINTEGER;
        (*out)[self].integer = lua_tointegerx(L, abs, nullptr);
      } else {
        (*out)[self].type = LuaNode::Type::kNUMBER;
        (*out)[self].number = lua_tonumberx(L, abs, nullptr);
      }
      return true;
    case LUA_TSTRING: {
      size_t n = 0;
      const char* s = lua_tolstring(L, abs, &n);  // a string: no conversion
      (*out)[self].type = LuaNode::Type::kSTRING;
      (*out)[self].text = QByteArray(s, static_cast<int>(n));
      return true;
    }
    case LUA_TUSERDATA: {
      const auto* h = HandleAt(L, abs);
      if (h == nullptr) {
        SetError(error, "a userdata the Host did not issue");
        return false;
      }
      (*out)[self].type = LuaNode::Type::kHANDLE;
      (*out)[self].handle = *h;
      return true;
    }
    case LUA_TTABLE:
      break;
    default:
      SetError(error, "a function or thread cannot be a value here");
      return false;
  }

  (*out)[self].type = LuaNode::Type::kTABLE;
  if (lua_checkstack(L, 3) == 0) {
    SetError(error, "Lua stack exhausted");
    return false;
  }
  lua_pushnil(L);
  while (lua_next(L, abs) != 0) {  // raw: no metamethod runs
    const int value = lua_gettop(L);
    bool is_index = false;
    qint64 index = 0;
    const char* key = nullptr;
    size_t key_len = 0;
    if (lua_type(L, value - 1) == LUA_TNUMBER &&
        lua_isinteger(L, value - 1) != 0) {
      is_index = true;
      index = lua_tointegerx(L, value - 1, nullptr);
    } else if (lua_type(L, value - 1) == LUA_TSTRING) {
      key = lua_tolstring(L, value - 1, &key_len);  // a string key: safe
    } else {
      SetError(error, "a table key that is neither a name nor an index");
      lua_pop(L, 2);
      return false;
    }

    const int child = static_cast<int>(out->size());
    if (!FlattenAt(L, value, out, self, depth + 1, error)) {
      lua_pop(L, 2);
      return false;
    }
    (*out)[child].key_is_index = is_index;
    (*out)[child].key_index = index;
    if (key != nullptr) {
      (*out)[child].key = QByteArray(key, static_cast<int>(key_len));
    }
    lua_pop(L, 1);
  }
  return true;
}

auto Type(const QCborMap& schema) -> QString {
  return schema.value(QStringLiteral("type")).toString();
}

auto IsOptional(const QCborMap& schema) -> bool {
  return schema.value(QStringLiteral("optional")).toBool();
}

auto Fields(const QCborMap& schema) -> QCborArray {
  return schema.value(QStringLiteral("fields")).toArray();
}

}  // namespace

auto HandleTypeName(HandleKind kind) -> const char* {
  switch (kind) {
    case HandleKind::kCOMMAND:
      return "gf.Command";
    case HandleKind::kANCHOR:
      return "gf.Anchor";
    case HandleKind::kMOUNT:
      return "gf.Mount";
    case HandleKind::kNATIVE_WIDGET:
      return "gf.NativeWidget";
    case HandleKind::kNATIVE_FACTORY:
      return "gf.NativeFactory";
    case HandleKind::kCONTEXT:
      return "gf.Context";
    case HandleKind::kDOCUMENT:
      return "gf.Document";
    case HandleKind::kKEY:
      return "gf.Key";
    case HandleKind::kDOCUMENT_REF:
      return "gf.DocumentRef";
    case HandleKind::kKEY_REF:
      return "gf.KeyRef";
    case HandleKind::kCALL:
      return "gf.Call";
    case HandleKind::kBLOB:
      return "gf.Blob";
  }
  return "gf.Unknown";
}

auto HandleAt(lua_State* L, int idx) -> const Handle* {
  if (lua_type(L, idx) != LUA_TUSERDATA) return nullptr;
  if (lua_rawlen(L, idx) != sizeof(Handle)) return nullptr;
  const auto* h = static_cast<const Handle*>(lua_touserdata(L, idx));
  if (h == nullptr || h->magic != kHandleMagic) return nullptr;
  return h;
}

auto FlattenValue(lua_State* L, int idx, LuaTree* out, ErrorText* error)
    -> bool {
  return FlattenAt(L, lua_absindex(L, idx), out, -1, 0, error);
}

auto ChildrenOf(const LuaTree& tree, int index) -> QList<int> {
  QList<int> children;
  for (int i = index + 1; i < static_cast<int>(tree.size()); ++i) {
    if (tree[i].parent == index) children.append(i);
  }
  return children;
}

auto NodeToCbor(const LuaTree& tree, int index, const QCborMap& schema,
                const HandleResolver& resolve,
                std::vector<gf::cmd::Blob>* blobs, QString* error)
    -> std::optional<QCborValue> {
  const auto fail = [error](const QString& why) -> std::optional<QCborValue> {
    if (error != nullptr && error->isEmpty()) *error = why;
    return std::nullopt;
  };
  const auto type = Type(schema);

  if (index < 0 || tree[index].type == LuaNode::Type::kNIL) {
    if (IsOptional(schema)) return QCborValue(QCborValue::Null);
    return fail(QStringLiteral("a required value is missing"));
  }
  const auto& n = tree[index];

  if (type == "bool") {
    if (n.type != LuaNode::Type::kBOOL) return fail("expected a boolean");
    return QCborValue(n.boolean);
  }
  if (type == "int") {
    if (n.type == LuaNode::Type::kINTEGER) return QCborValue(n.integer);
    if (n.type == LuaNode::Type::kNUMBER && std::floor(n.number) == n.number &&
        std::isfinite(n.number)) {
      return QCborValue(static_cast<qint64>(n.number));
    }
    return fail("expected an integer");
  }
  if (type == "double") {
    if (n.type == LuaNode::Type::kINTEGER) {
      return QCborValue(static_cast<double>(n.integer));
    }
    if (n.type == LuaNode::Type::kNUMBER) return QCborValue(n.number);
    return fail("expected a number");
  }
  if (type == "string") {
    if (n.type != LuaNode::Type::kSTRING) return fail("expected a string");
    return QCborValue(QString::fromUtf8(n.text));
  }
  if (type == "bytes") {
    if (n.type != LuaNode::Type::kSTRING) return fail("expected bytes");
    return QCborValue(n.text);
  }
  if (type == "blob") {
    // Only a Blob the Host issued. A string is not accepted: secret bytes
    // never pass through an ordinary Lua value.
    if (n.type != LuaNode::Type::kHANDLE ||
        n.handle.kind != HandleKind::kBLOB) {
      return fail("expected a Blob; a string cannot become one");
    }
    return resolve(n.handle, blobs, error);
  }
  if (type == "string_list" || type == "list") {
    if (n.type != LuaNode::Type::kTABLE) return fail("expected a list");
    const auto item_schema =
        type == "list" ? schema.value(QStringLiteral("item")).toMap()
                       : QCborMap{{QStringLiteral("type"), "string"}};
    QCborArray items;
    const auto children = ChildrenOf(tree, index);
    for (int i = 0; i < children.size(); ++i) {
      const auto& c = tree[children[i]];
      if (!c.key_is_index || c.key_index != i + 1) {
        return fail("a list takes 1..n and nothing else");
      }
    }
    for (const int c : children) {
      auto v = NodeToCbor(tree, c, item_schema, resolve, blobs, error);
      if (!v.has_value()) return std::nullopt;
      items.append(*v);
    }
    return QCborValue(items);
  }
  if (type == "object") {
    if (n.type == LuaNode::Type::kHANDLE) {
      // A Document, Key or Mount stands for the reference type the schema
      // asks for; the resolver says whether it may.
      auto v = resolve(n.handle, blobs, error);
      if (!v.has_value()) return std::nullopt;
      const auto m = v->toMap();
      for (const auto& f : Fields(schema)) {
        if (!m.contains(f.toMap().value(QStringLiteral("name")))) {
          return fail("that handle is not what this field takes");
        }
      }
      if (m.size() != Fields(schema).size()) {
        return fail("that handle is not what this field takes");
      }
      return v;
    }
    if (n.type != LuaNode::Type::kTABLE) return fail("expected a table");

    const auto children = ChildrenOf(tree, index);
    QCborMap out;
    int matched = 0;
    for (const auto& f : Fields(schema)) {
      const auto fm = f.toMap();
      const auto name = fm.value(QStringLiteral("name")).toString().toUtf8();
      int child = -1;
      for (const int c : children) {
        if (!tree[c].key_is_index && tree[c].key == name) child = c;
      }
      if (child >= 0) ++matched;
      auto v = NodeToCbor(tree, child,
                          fm.value(QStringLiteral("schema")).toMap(), resolve,
                          blobs, error);
      if (!v.has_value()) {
        if (error != nullptr) {
          *error = QStringLiteral("field \"%1\": %2")
                       .arg(QString::fromUtf8(name), *error);
        }
        return std::nullopt;
      }
      if (!v->isNull()) out.insert(QString::fromUtf8(name), *v);
    }
    if (matched != children.size()) {
      return fail("a field the command does not declare");
    }
    return QCborValue(out);
  }
  return fail(QStringLiteral("a schema type Lua cannot supply: ") + type);
}

auto CborToPushTree(const QCborValue& value, const QCborMap& schema,
                    const std::vector<gf::cmd::Blob>& blobs,
                    const BlobRegistrar& register_blob, PushTree* out,
                    QString* error) -> bool {
  const auto fail = [error](const QString& why) {
    if (error != nullptr && error->isEmpty()) *error = why;
    return false;
  };
  const auto type = Type(schema);

  if (value.isNull() || value.isUndefined()) {
    if (!IsOptional(schema)) return fail("a required value is missing");
    out->push_back(PushNode{});
    return true;
  }

  PushNode node;
  if (type == "bool") {
    if (!value.isBool()) return fail("expected a boolean");
    node.type = PushNode::Type::kBOOL;
    node.boolean = value.toBool();
  } else if (type == "int") {
    if (!value.isInteger()) return fail("expected an integer");
    node.type = PushNode::Type::kINTEGER;
    node.integer = value.toInteger();
  } else if (type == "double") {
    if (!value.isDouble() && !value.isInteger()) return fail("expected a number");
    node.type = PushNode::Type::kNUMBER;
    node.number = value.toDouble();
  } else if (type == "string") {
    if (!value.isString()) return fail("expected a string");
    node.type = PushNode::Type::kSTRING;
    node.text = value.toString().toUtf8();
  } else if (type == "bytes") {
    if (!value.isByteArray()) return fail("expected bytes");
    node.type = PushNode::Type::kSTRING;
    node.text = value.toByteArray();
  } else if (type == "blob") {
    const auto i = value.toMap().value(QStringLiteral("$blob"));
    if (!i.isInteger() || i.toInteger() < 0 ||
        static_cast<size_t>(i.toInteger()) >= blobs.size()) {
      return fail("a blob reference out of range");
    }
    node.type = PushNode::Type::kBLOB;
    node.blob_id = register_blob(blobs[static_cast<size_t>(i.toInteger())]);
  } else if (type == "string_list" || type == "list") {
    if (!value.isArray()) return fail("expected a list");
    const auto items = value.toArray();
    node.type = PushNode::Type::kTABLE;
    node.children = static_cast<int>(items.size());
    out->push_back(node);
    const auto item_schema =
        type == "list" ? schema.value(QStringLiteral("item")).toMap()
                       : QCborMap{{QStringLiteral("type"), "string"}};
    for (qsizetype i = 0; i < items.size(); ++i) {
      const auto at = out->size();
      if (!CborToPushTree(items.at(i), item_schema, blobs, register_blob, out,
                          error)) {
        return false;
      }
      (*out)[at].key_is_index = true;
      (*out)[at].key_index = i + 1;
    }
    return true;
  } else if (type == "object") {
    if (!value.isMap()) return fail("expected an object");
    const auto m = value.toMap();
    int present = 0;
    for (const auto& f : Fields(schema)) {
      const auto name = f.toMap().value(QStringLiteral("name"));
      if (m.contains(name) && !m.value(name).isNull()) ++present;
    }
    int declared = 0;
    for (auto it = m.constBegin(); it != m.constEnd(); ++it) {
      bool known = false;
      for (const auto& f : Fields(schema)) {
        known = known || f.toMap().value(QStringLiteral("name")) == it.key();
      }
      if (!known) return fail("a field the schema does not declare");
      ++declared;
    }
    node.type = PushNode::Type::kTABLE;
    node.children = present;
    out->push_back(node);
    for (const auto& f : Fields(schema)) {
      const auto fm = f.toMap();
      const auto name = fm.value(QStringLiteral("name"));
      const auto v = m.value(name);
      if (v.isNull() || v.isUndefined()) {
        if (!fm.value(QStringLiteral("schema")).toMap().value("optional")
                 .toBool()) {
          return fail(QStringLiteral("missing field \"%1\"")
                          .arg(name.toString()));
        }
        continue;
      }
      const auto at = out->size();
      if (!CborToPushTree(v, fm.value(QStringLiteral("schema")).toMap(), blobs,
                          register_blob, out, error)) {
        return false;
      }
      (*out)[at].key = name.toString().toUtf8();
    }
    (void)declared;
    return true;
  } else {
    return fail(QStringLiteral("a schema type Lua cannot receive: ") + type);
  }
  out->push_back(node);
  return true;
}

auto PushNodeValue(lua_State* L, const PushTree& tree, int index,
                   quint64 state_tag) -> int {
  // Plain locals only, as in FlattenAt: this runs inside Protected().
  const auto& n = tree[static_cast<size_t>(index)];
  luaL_checkstack(L, 3, "result");
  switch (n.type) {
    case PushNode::Type::kNIL:
      lua_pushnil(L);
      return index + 1;
    case PushNode::Type::kBOOL:
      lua_pushboolean(L, n.boolean ? 1 : 0);
      return index + 1;
    case PushNode::Type::kINTEGER:
      lua_pushinteger(L, static_cast<lua_Integer>(n.integer));
      return index + 1;
    case PushNode::Type::kNUMBER:
      lua_pushnumber(L, n.number);
      return index + 1;
    case PushNode::Type::kSTRING:
      lua_pushlstring(L, n.text.constData(), static_cast<size_t>(n.text.size()));
      return index + 1;
    case PushNode::Type::kBLOB:
      PushHandle(L, HandleKind::kBLOB, state_tag, n.blob_id, 0);
      return index + 1;
    case PushNode::Type::kTABLE:
      break;
  }
  lua_createtable(L, 0, n.children);
  int next = index + 1;
  for (int c = 0; c < n.children; ++c) {
    const auto& child = tree[static_cast<size_t>(next)];
    const bool by_index = child.key_is_index;
    const lua_Integer at = static_cast<lua_Integer>(child.key_index);
    const char* key = child.key.constData();
    next = PushNodeValue(L, tree, next, state_tag);
    if (by_index) {
      lua_rawseti(L, -2, at);
    } else {
      lua_setfield(L, -2, key);
    }
  }
  return next;
}

void PushHandle(lua_State* L, HandleKind kind, quint64 state_tag, qint64 id,
                quint64 epoch) {
  auto* h = static_cast<Handle*>(lua_newuserdatauv(L, sizeof(Handle), 0));
  *h = Handle{kHandleMagic, kind, state_tag, id, epoch};
  luaL_setmetatable(L, HandleTypeName(kind));
}

}  // namespace GpgFrontend::UI::Lua
