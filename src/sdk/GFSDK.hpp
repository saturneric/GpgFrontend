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
#include <QCborValue>
#include <QVariant>
#include <QList>
#include <QString>
#include <QStringList>
#include <vector>

#include "GFSDKBuffer.h"
#include "GFSDKContext.h"
#include "GFSDKGpg.h"
#include "GFSDKGpgList.h"
#include "GFSDKProcess.h"
#include "GFSDKStorage.h"
#include "GFSDKUI.h"

/**
 * @file GFSDK.hpp
 * @brief C++ conveniences over the C SDK. Header-only, and still stateless.
 *
 * The C ABI reports absence, takes octets and hands back owned handles,
 * because those are the honest shapes for a boundary. None of that is
 * pleasant to write forty times in a module, so the pleasant forms live here:
 * they take and return QString, apply a fallback where a caller wants one,
 * and release what they borrowed.
 *
 * Every one of them takes the context explicitly, exactly like the C
 * functions they wrap. Nothing here holds state, and nothing looks a context
 * up: this is sugar over the ABI, not a second way of reaching the host.
 */

namespace gf::sdk {

/// Take a buffer's octets and release it. NULL yields an empty result.
inline auto TakeBytes(GFSDKContext* ctx, GFBufferRef buf) -> QByteArray {
  if (buf == nullptr) return {};
  const auto* data = static_cast<const char*>(GFBufferData(ctx, buf));
  const auto size = static_cast<qsizetype>(GFBufferSize(ctx, buf));
  QByteArray bytes(data == nullptr ? "" : data, size);
  GFBufferRelease(ctx, buf);
  return bytes;
}

/// The same, decoded as UTF-8.
inline auto TakeString(GFSDKContext* ctx, GFBufferRef buf) -> QString {
  return QString::fromUtf8(TakeBytes(ctx, buf));
}

/// A borrowed view over @p bytes, for passing into the C API.
///
/// The returned handle is OWNED and must be released; it is spelled as a
/// scope object so a caller cannot forget.
class Bytes {
 public:
  Bytes(GFSDKContext* ctx, const QByteArray& bytes)
      : ctx_(ctx),
        buf_(GFBufferNewFromBytes(ctx, bytes.constData(),
                                  static_cast<size_t>(bytes.size()))) {}
  ~Bytes() { GFBufferRelease(ctx_, buf_); }

  Bytes(const Bytes&) = delete;
  auto operator=(const Bytes&) -> Bytes& = delete;

  [[nodiscard]] auto view() const -> GFBufferView { return buf_; }
  operator GFBufferView() const { return buf_; }  // NOLINT(runtime/explicit)

 private:
  GFSDKContext* ctx_;
  GFBufferRef buf_;
};

/* --- the register table ---------------------------------------------------
 *
 * The C calls report absence, which is what makes "is it set" answerable at
 * all. These apply a fallback, which is what most callers actually want.
 */

inline auto StateText(GFSDKContext* ctx, const QString& ns, const QString& key,
                      const QString& fallback = {}) -> QString {
  GFBufferRef out = nullptr;
  if (GFStorageStateGetText(ctx, ns.toUtf8().constData(),
                            key.toUtf8().constData(), &out) != 0) {
    return fallback;
  }
  return TakeString(ctx, out);
}

inline void SetStateText(GFSDKContext* ctx, const QString& ns,
                         const QString& key, const QString& value) {
  const Bytes bytes(ctx, value.toUtf8());
  GFStorageStateSetText(ctx, ns.toUtf8().constData(), key.toUtf8().constData(),
                        bytes.view());
}

/**
 * @brief A setting, by key, in the module's own group or a shared Host one.
 *
 * Values are whatever a QVariant carries through CBOR: numbers, booleans,
 * strings, lists and maps of those. Never a secret -- settings are plain
 * files; secrets go in the GF_STORE_SECURE_DURABLE cache.
 */
inline auto Setting(GFSDKContext* ctx, int scope, const QString& key,
                    const QVariant& fallback = {}) -> QVariant {
  GFBufferRef out = nullptr;
  if (GFStorageSettingGet(ctx, scope, key.toUtf8().constData(), &out) != 0) {
    return fallback;
  }
  const auto* data = static_cast<const char*>(GFBufferData(ctx, out));
  const auto size = GFBufferSize(ctx, out);
  const auto value =
      QCborValue::fromCbor(QByteArray(data, static_cast<int>(size))).toVariant();
  GFBufferRelease(ctx, out);
  return value;
}

inline auto SetSetting(GFSDKContext* ctx, int scope, const QString& key,
                       const QVariant& value) -> bool {
  const Bytes bytes(ctx, QCborValue::fromVariant(value).toCbor());
  return GFStorageSettingSet(ctx, scope, key.toUtf8().constData(),
                             bytes.view()) == 0;
}

inline auto RemoveSetting(GFSDKContext* ctx, int scope, const QString& key)
    -> bool {
  return GFStorageSettingRemove(ctx, scope, key.toUtf8().constData()) == 0;
}

inline auto StateBool(GFSDKContext* ctx, const QString& ns, const QString& key,
                      bool fallback = false) -> bool {
  int value = 0;
  if (GFStorageStateGetBool(ctx, ns.toUtf8().constData(),
                            key.toUtf8().constData(), &value) != 0) {
    return fallback;
  }
  return value != 0;
}

inline void SetStateBool(GFSDKContext* ctx, const QString& ns,
                         const QString& key, bool value) {
  GFStorageStateSetBool(ctx, ns.toUtf8().constData(), key.toUtf8().constData(),
                        value ? 1 : 0);
}

inline auto StateChildren(GFSDKContext* ctx, const QString& ns,
                          const QString& key) -> QStringList {
  GFStringListRef list = nullptr;
  if (GFStorageStateListChildren(ctx, ns.toUtf8().constData(),
                                 key.toUtf8().constData(), &list) != 0) {
    return {};
  }

  QStringList children;
  const auto count = GFStringListCount(ctx, list);
  children.reserve(static_cast<qsizetype>(count));
  for (size_t i = 0; i < count; ++i) {
    children.append(QString::fromUtf8(GFStringListAt(ctx, list, i)));
  }
  GFStringListRelease(ctx, list);
  return children;
}

/* --- caches --------------------------------------------------------------- */

/// @return the stored value, or an empty string when the key is absent
inline auto CacheText(GFSDKContext* ctx, int store, const QString& key)
    -> QString {
  GFBufferRef out = nullptr;
  if (GFStorageCacheGet(ctx, store, key.toUtf8().constData(), &out) != 0) {
    return {};
  }
  return TakeString(ctx, out);
}

inline void SetCacheText(GFSDKContext* ctx, int store, const QString& key,
                         const QString& value, qint64 ttl_seconds = 0) {
  const Bytes bytes(ctx, value.toUtf8());
  GFStorageCacheSet(ctx, store, key.toUtf8().constData(), bytes.view(),
                    ttl_seconds);
}

inline void RemoveCache(GFSDKContext* ctx, int store, const QString& key) {
  GFStorageCacheRemove(ctx, store, key.toUtf8().constData());
}

/* --- keys ------------------------------------------------------------------
 *
 * The C forms take and return buffer handles, which is right for a boundary
 * and tedious in a module. These are the same calls with the handle work
 * done.
 */

inline auto ImportKeys(GFSDKContext* ctx, int channel, void* parent,
                       const QByteArray& data) -> int {
  const Bytes bytes(ctx, data);
  return GFGpgImportKeys(ctx, channel, parent, bytes.view());
}

inline auto ExportKey(GFSDKContext* ctx, int channel, const QString& key_id,
                      bool ascii) -> QByteArray {
  GFBufferRef out = nullptr;
  if (GFGpgExportKey(ctx, channel, key_id.toUtf8().constData(), ascii ? 1 : 0,
                     &out) != 0) {
    return {};
  }
  return TakeBytes(ctx, out);
}

inline auto PublicKey(GFSDKContext* ctx, int channel, const QString& key_id,
                      bool ascii) -> QByteArray {
  return TakeBytes(
      ctx,
      GFGpgPublicKey(ctx, channel, key_id.toUtf8().constData(), ascii ? 1 : 0));
}

/**
 * @brief Everything the host can say about one finished operation.
 *
 * The C call fills in a struct of three owned `char*`; this releases them and
 * hands back QStrings. Nine entry points used to exist for this, differing
 * only in the operation they named and in whether they produced the JSON.
 *
 * The capsule is CONSUMED by the call, so ask for everything in one go.
 */
struct Analysis {
  int status = -1;
  QString report;
  QString cards;
  QString info_json;
};

inline auto AnalyseResult(GFSDKContext* ctx, int channel, int operation,
                          uint32_t err, const QString& capsule_id,
                          uint32_t want = GF_GPG_ANALYSE_WANT_REPORT |
                                          GF_GPG_ANALYSE_WANT_CARDS |
                                          GF_GPG_ANALYSE_WANT_INFO_JSON)
    -> Analysis {
  GFGpgAnalysis raw{};
  raw.struct_size = sizeof(GFGpgAnalysis);

  Analysis out;
  out.status = GFGpgAnalyseResult(ctx, channel, operation, err,
                                  capsule_id.toUtf8().constData(), want, &raw);

  const auto take = [ctx](char* text) -> QString {
    if (text == nullptr) return {};
    auto value = QString::fromUtf8(text);
    GFMemFree(ctx, GF_ARENA_NORMAL, text);
    return value;
  };
  out.report = take(raw.report);
  out.cards = take(raw.cards);
  out.info_json = take(raw.info_json);
  return out;
}

/* --- user interface --------------------------------------------------------
 *
 * The spec structs exist so the ABI can grow a field without a new entry
 * point. A caller that wants today's fields should not have to fill one in by
 * hand, so these do it.
 */

inline auto RegisterSettingsPage(GFSDKContext* ctx, const char* page_id,
                                 const char* section_id, const char* title,
                                 const char* keywords, QObjectFactory factory,
                                 void* data) -> int {
  GFUISettingsPageSpec spec{};
  spec.struct_size = sizeof(GFUISettingsPageSpec);
  spec.page_id = page_id;
  spec.section_id = section_id;
  spec.title = title;
  spec.keywords = keywords;
  spec.factory = factory;
  spec.data = data;
  return GFUIRegisterSettingsPage(ctx, &spec);
}

inline auto RegisterTabPageView(GFSDKContext* ctx, const char* tab_type,
                                QObjectFactory factory, void* data) -> int {
  GFUITabViewSpec spec{};
  spec.struct_size = sizeof(GFUITabViewSpec);
  spec.tab_type = tab_type;
  spec.factory = factory;
  spec.data = data;
  return GFUIRegisterTabPageView(ctx, &spec);
}

/// The directory a file dialog for user files should open in.
inline auto UserFilePath(GFSDKContext* ctx) -> QString {
  return TakeString(ctx, GFUIDefaultUserFilePath(ctx));
}

/* --- external programs ---------------------------------------------------- */

/** One command for @ref RunCommands. */
struct Command {
  QString program;
  QStringList arguments;
  GFCommandExecuteCallback callback = nullptr;
  void* data = nullptr; /**< passed to @ref callback; the caller frees it */
};

/**
 * @brief Run several commands concurrently and wait for all of them.
 *
 * The contexts, argument arrays and strings are built here and live until the
 * call returns, which is all the host needs: process.execute borrows
 * everything it is given, so nothing is allocated through the SDK arenas.
 */
inline auto RunCommands(GFSDKContext* ctx, const QList<Command>& commands)
    -> int {
  if (commands.isEmpty()) return -1;

  // Every byte array is created before any pointer into one is taken, so no
  // later append can move storage that an argv entry already points at.
  std::vector<std::vector<QByteArray>> keep(commands.size());
  for (qsizetype i = 0; i < commands.size(); ++i) {
    keep[i].reserve(commands[i].arguments.size() + 1);
    keep[i].push_back(commands[i].program.toUtf8());
    for (const auto& argument : commands[i].arguments) {
      keep[i].push_back(argument.toUtf8());
    }
  }

  std::vector<std::vector<char*>> argv(commands.size());
  std::vector<GFCommandExecuteContext> contexts(commands.size());
  std::vector<GFCommandExecuteContext*> batch(commands.size());
  for (qsizetype i = 0; i < commands.size(); ++i) {
    for (size_t j = 1; j < keep[i].size(); ++j) {
      argv[i].push_back(keep[i][j].data());
    }
    contexts[i].cmd = keep[i].front().data();
    contexts[i].argc = static_cast<int32_t>(argv[i].size());
    contexts[i].argv = argv[i].data();
    contexts[i].cb = commands[i].callback;
    contexts[i].data = commands[i].data;
    batch[i] = &contexts[i];
  }

  return GFProcessExecute(ctx, batch.data(), batch.size());
}

/**
 * @brief Run one command and wait for it.
 *
 * A batch of one: see @ref RunCommands.
 */
inline auto RunCommand(GFSDKContext* ctx, const QString& command,
                       const QStringList& arguments,
                       GFCommandExecuteCallback callback, void* data) -> int {
  return RunCommands(ctx, {Command{command, arguments, callback, data}});
}

}  // namespace gf::sdk
