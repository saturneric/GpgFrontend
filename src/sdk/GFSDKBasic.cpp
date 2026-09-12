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

#include "GFSDKBasic.h"

#include "core/function/CacheManager.h"
#include "core/model/GFBuffer.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/function/SecureMemoryAllocator.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "private/GFSDKPrivat.h"
#include "ui/UIModuleManager.h"

auto GFAllocateMemory(uint32_t size) -> void* {
  return GpgFrontend::SMAMalloc(size);
}

auto GFReallocateMemory(void* ptr, uint32_t size) -> void* {
  return GpgFrontend::SMARealloc(ptr, size);
}

void GFFreeMemory(void* ptr) { return GpgFrontend::SMAFree(ptr); }

auto GFProjectVersion() -> const char* {
  return GFStrDup(GpgFrontend::GetProjectVersion());
}

auto GFQtEnvVersion() -> const char* { return GFStrDup(QT_VERSION_STR); }

void GFExecuteCommandSync(const char* cmd, int32_t argc, char** argv,
                          GFCommandExecuteCallback cb, void* data) {
  QStringList args = CharArrayToQStringList(argv, argc);
  GpgFrontend::GpgCommandExecutor::ExecuteContext const context{
      cmd, args, [=](int exit_code, const QString& out, const QString& err) {
        cb(data, exit_code, out.toUtf8(), err.toUtf8());
      }};
  GpgFrontend::GpgCommandExecutor::ExecuteSync(context);
}

void GFExecuteCommandBatchSync(GFCommandExecuteContext** contexts,
                               int32_t contexts_size) {
  GpgFrontend::QContainer<GpgFrontend::GpgCommandExecutor::ExecuteContext>
      core_contexts;

  GpgFrontend::QContainer<GFCommandExecuteContext> sdk_contexts =
      ArrayToQList(contexts, contexts_size);
  for (const auto sdk_context : sdk_contexts) {
    QStringList args =
        CharArrayToQStringList(sdk_context.argv, sdk_context.argc);
    core_contexts.append(
        {GFUnStrDup(sdk_context.cmd), args,
         [data = sdk_context.data, cb = sdk_context.cb](
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

auto GFModuleStrDup(const char* src) -> char* {
  auto len = StrlenSafe(src, kGfStrlenMax);
  if (len > kGfStrlenMax) return nullptr;

  char* dst = static_cast<char*>(GFAllocateMemory((len + 1) * sizeof(char)));
  memcpy(dst, src, len);
  dst[len] = '\0';

  return dst;
}

auto GFModuleSecStrDup(const char* src) -> char* {
  auto len = StrlenSafe(src, kGfStrlenMax);
  if (len > kGfStrlenMax) return nullptr;

  char* dst = static_cast<char*>(GFSecAllocateMemory((len + 1) * sizeof(char)));
  memcpy(dst, src, len);
  dst[len] = '\0';

  return dst;
}

auto GFAppActiveLocale() -> char* { return GFStrDup(QLocale().name()); }

auto GFAppRegisterTranslatorReader(const char* id,
                                   GFTranslatorDataReader reader) -> int {
  return GpgFrontend::UI::UIModuleManager::GetInstance()
                 .RegisterTranslatorDataReader(GFUnStrDup(id), reader)
             ? 0
             : -1;
}

auto GF_SDK_EXPORT GFCacheSave(const char* key, const char* value) -> int {
  GpgFrontend::CacheManager::GetInstance().SaveCache(GFUnStrDup(key),
                                                     GFUnStrDup(value));
  return 0;
}

auto GF_SDK_EXPORT GFCacheGet(const char* key) -> const char* {
  auto value =
      GpgFrontend::CacheManager::GetInstance().LoadCache(GFUnStrDup(key));
  return GFStrDup(value);
}

auto GF_SDK_EXPORT GFCacheSaveWithTTL(const char* key, const char* value,
                                      int ttl) -> int {
  GpgFrontend::CacheManager::GetInstance().SaveCache(GFUnStrDup(key),
                                                     GFUnStrDup(value), ttl);
  return 0;
}

auto GF_SDK_EXPORT GFProjectGitCommitHash() -> const char* {
  return GFStrDup(GpgFrontend::GetProjectBuildGitCommitHash());
}

auto GF_SDK_EXPORT GFIsFlatpakENV() -> bool {
  return GpgFrontend::IsFlatpakENV();
}

auto GF_SDK_EXPORT GFSecAllocateMemory(uint32_t size) -> void* {
  return GpgFrontend::SMASecMalloc(size);
}

auto GF_SDK_EXPORT GFSecReallocateMemory(void* ptr, uint32_t size) -> void* {
  return GpgFrontend::SMASecRealloc(ptr, size);
}

void GF_SDK_EXPORT GFSecFreeMemory(void* ptr) { GpgFrontend::SMASecFree(ptr); }

auto GF_SDK_EXPORT GFDurableCacheGet(const char* key) -> const char* {
  auto value = GpgFrontend::CacheManager::GetInstance().LoadDurableCache(
      "__module_" + GFUnStrDup(key));
  return GFStrDup(value.toJson());
}

auto GF_SDK_EXPORT GFDurableCacheSave(const char* key, const char* value)
    -> int {
  GpgFrontend::CacheManager::GetInstance().SaveDurableCache(
      "__module_" + GFUnStrDup(key),
      QJsonDocument::fromJson(GFUnStrDup(value).toUtf8()));
  return 0;
}

auto GF_SDK_EXPORT GFSecDurableCacheGet(const char* key) -> char* {
  auto buffer = GpgFrontend::CacheManager::GetInstance().LoadSecDurableCache(
      "__module_" + GFUnStrDup(key));
  if (buffer.Empty()) return nullptr;

  // GFModuleSecStrDup copies into zeroizing memory; the GFBuffer wipes itself
  // when it leaves scope, so the secret never sits in an ordinary allocation.
  return GFModuleSecStrDup(
      QString::fromUtf8(buffer.Data(), static_cast<int>(buffer.Size()))
          .toUtf8()
          .constData());
}

auto GF_SDK_EXPORT GFSecDurableCacheSave(const char* key, const char* value)
    -> int {
  if (key == nullptr || value == nullptr) return -1;

  // Both arguments are owned, as everywhere else in this SDK -- but they come
  // from different allocators and must go back to the matching one. The key is
  // ordinary module memory; the secret came from GFModuleSecStrDup and has to
  // be released through the secure allocator, which also wipes it. Freeing it
  // the ordinary way aborts the process.
  const auto key_string = GFUnStrDup(key);

  auto utf8 = QByteArray(value);
  GpgFrontend::GFBuffer buffer(utf8);

  // flush=true: a credential the user just typed has to survive a crash that
  // happens before the periodic flush would have run.
  GpgFrontend::CacheManager::GetInstance().SaveSecDurableCache(
      "__module_" + key_string, buffer, true);

  utf8.fill('\0');
  GFSecFreeMemory(static_cast<void*>(const_cast<char*>(value)));
  return 0;
}

auto GF_SDK_EXPORT GFSecDurableCacheRemove(const char* key) -> int {
  if (key == nullptr) return -1;
  GpgFrontend::CacheManager::GetInstance().ResetDurableCache(
      "__module_" + GFUnStrDup(key));
  return 0;
}

auto GF_SDK_EXPORT GFAppKeyProtectionLevel() -> int {
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
