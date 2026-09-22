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

#include "GFHostImpl.h"
#include "core/function/CacheManager.h"
#include "core/function/SecureMemoryAllocator.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/model/GFBuffer.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "private/GFHostContext.h"
#include "private/GFSDKPrivate.h"
#include "ui/UIModuleManager.h"

namespace gf_host {

auto GFAllocateMemory(uint32_t size) -> void* {
  return GpgFrontend::SMAMalloc(size);
}

auto GFReallocateMemory(void* ptr, uint32_t size) -> void* {
  return GpgFrontend::SMARealloc(ptr, size);
}

void GFFreeMemory(void* ptr) { return GpgFrontend::SMAFree(ptr); }

auto GFProjectVersion() -> const char* {
  // Borrowed statics, see the note in GFHttpRequestUserAgent: a caller-owned
  // const char* is implicitly convertible to QString, so every call site that
  // assigned one straight into a QString leaked silently.
  static const QByteArray kVersion = GpgFrontend::GetProjectVersion().toUtf8();
  return kVersion.constData();
}

auto GFQtEnvVersion() -> const char* { return QT_VERSION_STR; }

void GFExecuteCommandBatchSync(GFCommandExecuteContext** contexts,
                               int32_t contexts_size) {
  // Everything here is borrowed: the module built the array, the contexts and
  // their strings, and it releases them after this returns. Copy what is
  // needed and free nothing -- gf::sdk::RunCommand passes stack memory.
  GpgFrontend::QContainer<GpgFrontend::GpgCommandExecutor::ExecuteContext>
      core_contexts;
  if (contexts == nullptr || contexts_size <= 0) return;

  for (int32_t i = 0; i < contexts_size; ++i) {
    const auto* sdk_context = contexts[i];
    if (sdk_context == nullptr || sdk_context->cb == nullptr) continue;

    QStringList args;
    for (int32_t j = 0; sdk_context->argv != nullptr && j < sdk_context->argc;
         ++j) {
      if (sdk_context->argv[j] != nullptr) {
        args.append(GFStrView(sdk_context->argv[j]));
      }
    }

    core_contexts.append(
        {GFStrView(sdk_context->cmd), args,
         [data = sdk_context->data, cb = sdk_context->cb](
             int exit_code, const QString& out, const QString& err) {
           cb(data, exit_code, out.toUtf8(), err.toUtf8());
         }});
  }

  GpgFrontend::GpgCommandExecutor::ExecuteConcurrentlySync(core_contexts);
}

auto StrlenSafe(const char* str, size_t max_len) -> size_t {
  const char* end = static_cast<const char*>(memchr(str, '\0', max_len));
  if (end == nullptr) return max_len;
  return end - str;
}

namespace {

/// One scan limit past the maximum, so a string that is exactly too long is
/// refused instead of silently truncated to the limit.
auto StrDupWith(const char* src, void* (*alloc)(uint32_t)) -> char* {
  if (src == nullptr) return nullptr;

  const auto limit = static_cast<size_t>(kGfStrlenMax);
  const auto len = StrlenSafe(src, limit + 1);
  if (len > limit) return nullptr;

  auto* dst = static_cast<char*>(alloc(static_cast<uint32_t>(len + 1)));
  if (dst == nullptr) return nullptr;
  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}

}  // namespace

auto GFModuleStrDup(const char* src) -> char* {
  return StrDupWith(src, &GFAllocateMemory);
}

auto GFModuleSecStrDup(const char* src) -> char* {
  return StrDupWith(src, &GFSecAllocateMemory);
}

auto GFAppActiveLocale() -> char* { return GFStrDup(QLocale().name()); }

auto GFAppRegisterTranslatorReader(const char* id,
                                   GFTranslatorDataReader reader) -> int {
  return GpgFrontend::UI::UIModuleManager::GetInstance()
                 .RegisterTranslatorDataReader(GFStrView(id), reader)
             ? 0
             : -1;
}

auto GFProjectGitCommitHash() -> const char* {
  static const QByteArray kHash =
      GpgFrontend::GetProjectBuildGitCommitHash().toUtf8();
  return kHash.constData();
}

auto GFIsFlatpakENV() -> int { return GpgFrontend::IsFlatpakENV() ? 1 : 0; }

auto GFSecAllocateMemory(uint32_t size) -> void* {
  return GpgFrontend::SMASecMalloc(size);
}

auto GFSecReallocateMemory(void* ptr, uint32_t size) -> void* {
  return GpgFrontend::SMASecRealloc(ptr, size);
}

void GFSecFreeMemory(void* ptr) { GpgFrontend::SMASecFree(ptr); }

namespace {

/// Where a module's value lives. The module id comes from the caller's
/// context, never from the caller, so one module cannot name another's keys;
/// the store is part of the key, so the durable and secure stores cannot
/// read each other's entries.
auto ModuleCacheKey(const QString& module_id, int store, const QString& key)
    -> QString {
  return QStringLiteral("__module_%1_%2_%3").arg(module_id).arg(store).arg(key);
}

/// The key every module shared before keys were scoped by module and store.
auto LegacyModuleCacheKey(const QString& key) -> QString {
  return QStringLiteral("__module_") + key;
}

auto IsDurable(int store) -> bool {
  return store == GF_STORE_DURABLE || store == GF_STORE_SECURE_DURABLE;
}

}  // namespace

auto GFModuleCacheGet(const QString& module_id, int store, const QString& key,
                      GpgFrontend::GFBuffer* out) -> bool {
  if (module_id.isEmpty() || key.isEmpty() || out == nullptr) return false;
  auto& cache = GpgFrontend::CacheManager::GetInstance();
  const auto scoped = ModuleCacheKey(module_id, store, key);

  if (store == GF_STORE_SESSION) {
    *out = cache.LoadSecCache(scoped);
    return !out->Empty();
  }
  if (!IsDurable(store)) return false;

  *out = cache.LoadSecDurableCache(scoped);
  if (!out->Empty()) return true;

  // Migrate a value written before keys were scoped: move it to the scoped
  // key the first time its owner asks for it. Both durable stores used the
  // same legacy key, so whichever store asks first takes it.
  const auto legacy = LegacyModuleCacheKey(key);
  auto value = cache.LoadSecDurableCache(legacy);
  if (value.Empty()) return false;

  cache.SaveSecDurableCache(scoped, value, true);
  cache.ResetDurableCache(legacy);
  *out = value;
  return true;
}

auto GFModuleCacheSet(const QString& module_id, int store, const QString& key,
                      const GpgFrontend::GFBuffer& value, int64_t ttl_seconds)
    -> bool {
  if (module_id.isEmpty() || key.isEmpty()) return false;

  // An empty value is not stored: the durable cache never writes one to disk,
  // so storing it would be undone on the next start. Treat it as a removal.
  if (value.Empty()) return GFModuleCacheRemove(module_id, store, key);

  auto& cache = GpgFrontend::CacheManager::GetInstance();
  const auto scoped = ModuleCacheKey(module_id, store, key);
  switch (store) {
    case GF_STORE_SESSION:
      cache.SaveSecCache(scoped, value, ttl_seconds > 0 ? ttl_seconds : -1);
      return true;
    case GF_STORE_DURABLE:
      cache.SaveSecDurableCache(scoped, value, false);
      return true;
    case GF_STORE_SECURE_DURABLE:
      // flush=true: a credential the user just typed has to survive a crash
      // that happens before the periodic flush would have run.
      cache.SaveSecDurableCache(scoped, value, true);
      return true;
    default:
      return false;
  }
}

auto GFModuleCacheRemove(const QString& module_id, int store,
                         const QString& key) -> bool {
  if (module_id.isEmpty() || key.isEmpty()) return false;
  auto& cache = GpgFrontend::CacheManager::GetInstance();
  const auto scoped = ModuleCacheKey(module_id, store, key);

  if (store == GF_STORE_SESSION) {
    cache.ResetCache(scoped);
    return true;
  }
  if (!IsDurable(store)) return false;

  cache.ResetDurableCache(scoped);
  // A value that was never migrated must not come back on the next read.
  const auto legacy = LegacyModuleCacheKey(key);
  if (!cache.LoadSecDurableCache(legacy).Empty()) {
    cache.ResetDurableCache(legacy);
  }
  return true;
}

auto GFAppKeyProtectionLevel() -> int {
  // The live value is published on the application object rather than read back
  // from the key manager: the manager answers about a key it is holding, and
  // this has to be answerable at any time, including before one is loaded.
  if (qApp == nullptr) return -1;

  const auto property = qApp->property("GFAppKeyProtection");
  if (!property.isValid()) return -1;

  switch (GpgFrontend::AppKeyProtectionFromString(property.toString())) {
    case GpgFrontend::AppKeyProtection::kNONE:
      return 0;
    case GpgFrontend::AppKeyProtection::kKEYCHAIN:
      return 1;
    case GpgFrontend::AppKeyProtection::kPIN:
      return 2;
  }
  return -1;
}

}  // namespace gf_host