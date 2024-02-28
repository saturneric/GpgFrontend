/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "Basic.h"

#include "core/function/SecureMemoryAllocator.h"
#include "core/function/gpg/GpgCommandExecutor.h"

auto AllocateMemory(uint32_t size) -> void* {
  return GpgFrontend::SecureMemoryAllocator::Allocate(size);
}

void FreeMemory(void* ptr) {
  return GpgFrontend::SecureMemoryAllocator::Deallocate(ptr);
}

void ExecuteCommandSync(const char* cmd, int32_t argc, const char** argv,
                        CommandExeucteCallback cb, void* data) {
  QStringList args;
  for (int i = 0; i < argc; i++) {
    args.append(QString::fromUtf8(argv[i]));
  }

  GpgFrontend::GpgCommandExecutor::ExecuteContext context{
      cmd, args, [=](int exit_code, const QString& out, const QString& err) {
        cb(data, exit_code, out.toUtf8(), err.toUtf8());
      }};

  GpgFrontend::GpgCommandExecutor::ExecuteSync(context);
}

void ExecuteCommandBatchSync(int32_t context_size,
                             const CommandExecuteContext* context) {
  QList<GpgFrontend::GpgCommandExecutor::ExecuteContext> contexts;
  for (int i = 0; i < context_size; i++) {
    auto exec_context = context[i];
    QStringList args;
    for (int i = 0; i < exec_context.argc; i++) {
      args.append(QString::fromUtf8(exec_context.argv[i]));
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