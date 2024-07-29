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

#include "core/function/SecureMemoryAllocator.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/utils/BuildInfoUtils.h"
#include "sdk/private/CommonUtils.h"
#include "ui/GpgFrontendUIInit.h"

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

void GFExecuteCommandSync(const char* cmd, int32_t argc, const char** argv,
                          GFCommandExeucteCallback cb, void* data) {
  QStringList args;
  for (int i = 0; i < argc; i++) {
    args.append(GFUnStrDup(argv[i]));
  }

  GpgFrontend::GpgCommandExecutor::ExecuteContext const context{
      cmd, args, [=](int exit_code, const QString& out, const QString& err) {
        cb(data, exit_code, out.toUtf8(), err.toUtf8());
      }};

  GpgFrontend::GpgCommandExecutor::ExecuteSync(context);
}

void GFExecuteCommandBatchSync(int32_t context_size,
                               const GFCommandExecuteContext* context) {
  QList<GpgFrontend::GpgCommandExecutor::ExecuteContext> contexts;

  for (int i = 0; i < context_size; i++) {
    const auto& exec_context = context[i];

    QStringList args;
    const char** argv = exec_context.argv;
    for (int j = 0; j < exec_context.argc; j++) {
      args.append(GFUnStrDup(argv[j]));
    }

    contexts.append(
        {exec_context.cmd, args,
         [data = exec_context.data, cb = exec_context.cb](
             int exit_code, const QString& out, const QString& err) {
           cb(data, exit_code, out.toUtf8(), err.toUtf8());
         }});
  }

  GpgFrontend::GpgCommandExecutor::ExecuteConcurrentlySync(contexts);
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

auto GFAppRegisterTranslator(char* data, int size) -> int {
  auto b = QByteArray(data, size);
  QMetaObject::invokeMethod(QApplication::instance()->thread(), [b]() {
    GpgFrontend::UI::InstallTranslatorFromQMData(b);
  });
  GFFreeMemory(data);
  return 0;
}
