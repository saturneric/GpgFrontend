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
#include "core/function/SecureMemoryAllocator.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/utils/BuildInfoUtils.h"
#include "private/GFSDKPrivat.h"
#include "ui/UIModuleManager.h"

auto GFAllocateMemory(uint32_t size) -> void* {
  return GpgFrontend::SecureMemoryAllocator::Allocate(size);
}

auto GFReallocateMemory(void* ptr, uint32_t size) -> void* {
  return GpgFrontend::SecureMemoryAllocator::Reallocate(ptr, size);
}

void GFFreeMemory(void* ptr) {
  return GpgFrontend::SecureMemoryAllocator::Deallocate(ptr);
}

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

auto GFAppActiveLocale() -> char* { return GFStrDup(QLocale().name()); }

auto GFAppRegisterTranslatorReader(const char* id,
                                   GFTranslatorDataReader reader) -> int {
  return GpgFrontend::UI::UIModuleManager::GetInstance()
                 .RegisterTranslatorDataReader(GFUnStrDup(id), reader)
             ? 0
             : -1;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFCacheSave(const char* key,
                                               const char* value) -> int {
  GpgFrontend::CacheManager::GetInstance().SaveCache(GFUnStrDup(key),
                                                     GFUnStrDup(value));
  return 0;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFCacheGet(const char* key) -> const char* {
  auto value =
      GpgFrontend::CacheManager::GetInstance().LoadCache(GFUnStrDup(key));
  return GFStrDup(value);
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFCacheSaveWithTTL(const char* key,
                                                      const char* value,
                                                      int ttl) -> int {
  GpgFrontend::CacheManager::GetInstance().SaveCache(GFUnStrDup(key),
                                                     GFUnStrDup(value), ttl);
  return 0;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFProjectGitCommitHash() -> const char* {
  return GFStrDup(GpgFrontend::GetProjectBuildGitCommitHash());
}