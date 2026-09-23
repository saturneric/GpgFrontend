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

#include <QByteArray>
#include <QCborMap>
#include <QCborValue>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "sdk/GFSDKCommand.hpp"

struct lua_State;

namespace GpgFrontend::UI::Lua {

/**
 * @file LuaValue.h
 * @brief Values across the boundary, in two phases that keep the invariant.
 *
 * Phase one runs inside Protected() and may raise: it reads a Lua value into
 * a flat list of plain nodes. Phase two is ordinary C++ with no Lua call in
 * sight: it checks those nodes against a command's schema and builds CBOR.
 * Results go the other way -- checked and flattened in C++, then pushed
 * inside Protected(). No step both owns C++ objects and calls Lua.
 */

/// Every kind of handle a script can hold, each with its own metatable.
enum class HandleKind : uint32_t {
  kCOMMAND = 1,
  kANCHOR,
  kMOUNT,
  kNATIVE_WIDGET,
  kNATIVE_FACTORY,
  kCONTEXT,
  kDOCUMENT,
  kKEY,
  kDOCUMENT_REF,
  kKEY_REF,
  kCALL,
  kBLOB,
};

constexpr uint32_t kHandleMagic = 0x47464C48;  // "GFLH"

/**
 * @brief A handle's whole payload. Plain data: no pointer, nothing to free.
 *
 * What a handle refers to lives in its runtime's tables under `id`. The
 * state tag ties it to one Lua state; the epoch, for the kinds that are only
 * good for one call, ties it to one entry into Lua.
 */
struct Handle {
  uint32_t magic;
  HandleKind kind;
  quint64 state_tag;
  qint64 id;
  quint64 epoch;  ///< 0: not epoch-bound
};

/// The metatable name for a kind, e.g. "gf.Document".
auto HandleTypeName(HandleKind kind) -> const char*;

/// The Handle at @p idx, or nullptr when the value is not one. Never raises.
auto HandleAt(lua_State* L, int idx) -> const Handle*;

// ------------------------------------------------------------ Lua -> C++

struct LuaNode {
  enum class Type { kNIL, kBOOL, kINTEGER, kNUMBER, kSTRING, kTABLE, kHANDLE };

  Type type = Type::kNIL;
  bool boolean = false;
  qint64 integer = 0;
  double number = 0;
  QByteArray text;
  Handle handle{};

  int parent = -1;             ///< index of the table this sits in
  bool key_is_index = false;   ///< array element rather than a named field
  qint64 key_index = 0;
  QByteArray key;
};

using LuaTree = std::vector<LuaNode>;
using ErrorText = std::array<char, 160>;

/**
 * @brief Read the value at @p idx into @p out. Call inside Protected() only.
 *
 * Tables are read raw -- no metamethod runs -- to at most 8 levels and 512
 * nodes. A function, thread or unknown userdata is refused: nothing that
 * can run code, or that the Host did not issue, becomes an argument.
 */
auto FlattenValue(lua_State* L, int idx, LuaTree* out, ErrorText* error)
    -> bool;

/// The children of node @p index, in the order they were read.
auto ChildrenOf(const LuaTree& tree, int index) -> QList<int>;

/// Turns a handle into the CBOR it stands for, or explains why it cannot.
using HandleResolver = std::function<std::optional<QCborValue>(
    const Handle&, std::vector<gf::cmd::Blob>* blobs, QString* error)>;

/**
 * @brief Node @p index as a CBOR value of @p schema, or nullopt with a reason.
 *
 * Strict in the same way the command decoder is: an undeclared field is an
 * error, a missing required one is an error, and a handle is accepted only
 * where the schema takes what it stands for.
 */
auto NodeToCbor(const LuaTree& tree, int index, const QCborMap& schema,
                const HandleResolver& resolve,
                std::vector<gf::cmd::Blob>* blobs, QString* error)
    -> std::optional<QCborValue>;

// ------------------------------------------------------------ C++ -> Lua

struct PushNode {
  enum class Type { kNIL, kBOOL, kINTEGER, kNUMBER, kSTRING, kTABLE, kBLOB };

  Type type = Type::kNIL;
  bool boolean = false;
  qint64 integer = 0;
  double number = 0;
  QByteArray text;
  qint64 blob_id = 0;  ///< the runtime's id for a Blob handle
  int children = 0;    ///< for a table: how many nodes follow as its fields
  bool key_is_index = false;
  qint64 key_index = 0;
  QByteArray key;
};

using PushTree = std::vector<PushNode>;

/// Registers a result Blob with the runtime and returns its handle id.
using BlobRegistrar = std::function<qint64(gf::cmd::Blob)>;

/**
 * @brief Check @p value against @p schema and flatten it for pushing.
 *
 * Blobs become Blob handles through @p register_blob -- never strings.
 * @return false, with the reason, when the value does not match the schema
 */
auto CborToPushTree(const QCborValue& value, const QCborMap& schema,
                    const std::vector<gf::cmd::Blob>& blobs,
                    const BlobRegistrar& register_blob, PushTree* out,
                    QString* error) -> bool;

/**
 * @brief Push node @p index and its children. Inside Protected() only.
 *
 * @return the index after the last node consumed
 */
auto PushNodeValue(lua_State* L, const PushTree& tree, int index,
                   quint64 state_tag) -> int;

/// Push a new handle userdata of @p kind. Inside Protected() only.
void PushHandle(lua_State* L, HandleKind kind, quint64 state_tag, qint64 id,
                quint64 epoch);

}  // namespace GpgFrontend::UI::Lua
