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

#include "ModuleRuntimeStubs.h"

#include <GFSDKBasic.h>
#include <GFSDKBasicModel.h>
#include <GFSDKLog.h>
#include <GFSDKModule.h>
#include <GFSDKModuleModel.h>
#include <GFSDKUI.h>

#include <QByteArray>
#include <QMap>
#include <QString>
#include <cstdint>
#include <cstdlib>
#include <cstring>

/**
 * @file ModuleRuntimeStubs.cpp
 * @brief The host, reduced to what gf_module_runtime actually calls.
 *
 * The runtime deliberately links no SDK library: it declares the SDK symbols
 * it uses and leaves them undefined, so a module resolves them. That is what
 * makes this file possible -- the runtime can be linked into a test binary
 * against a host that only records what it was asked to do.
 *
 * It also means these stubs double as a check: anything the runtime calls that
 * is not defined here fails to link, so the runtime's dependency on the host
 * cannot grow silently.
 */

namespace stubs {

Recorder& Rec() {
  static Recorder r;
  return r;
}

void Recorder::Reset() { *this = Recorder{}; }

}  // namespace stubs

using stubs::Rec;

// ------------------------------------------------------------- allocation

extern "C" {

void* GFAllocateMemory(uint32_t size) {
  Rec().allocations++;
  return std::malloc(size);
}

void* GFSecAllocateMemory(uint32_t size) {
  Rec().allocations++;
  return std::malloc(size);
}

void* GFReallocateMemory(void* ptr, uint32_t size) {
  return std::realloc(ptr, size);
}

void GFFreeMemory(void* ptr) {
  if (ptr != nullptr) Rec().frees++;
  std::free(ptr);
}

void GFSecFreeMemory(void* ptr) {
  if (ptr != nullptr) Rec().frees++;
  std::free(ptr);
}

char* GFModuleStrDup(const char* str) {
  if (str == nullptr) return nullptr;
  Rec().allocations++;
  const auto n = std::strlen(str);
  auto* out = static_cast<char*>(std::malloc(n + 1));
  std::memcpy(out, str, n + 1);
  return out;
}

char* GFModuleSecStrDup(const char* str) { return GFModuleStrDup(str); }

// ------------------------------------------------------------ the host

void GFModuleListenEvent(const char* module_id, const char* event_id) {
  Rec().listened.append(QString::fromUtf8(event_id));
  Rec().listened_as = QString::fromUtf8(module_id);
}

void GFModuleTriggerModuleEventCallback(GFModuleEvent* event,
                                        const char* /*module_id*/,
                                        GFModuleEventParam* params) {
  QMap<QString, QString> answered;
  for (auto* p = params; p != nullptr;) {
    answered.insert(QString::fromUtf8(p->name), QString::fromUtf8(p->value));
    auto* done = p;
    p = p->next;
    // The host owns what it is handed, on every path.
    GFFreeMemory(const_cast<char*>(done->name));
    GFFreeMemory(const_cast<char*>(done->value));
    GFFreeMemory(done);
  }
  if (event != nullptr) {
    GFFreeMemory(const_cast<char*>(event->id));
    GFFreeMemory(const_cast<char*>(event->trigger_id));
    GFFreeMemory(event);
  }
  Rec().answers.append(answered);
}

int GFAppRegisterTranslatorReader(const char* id,
                                  GFTranslatorDataReader reader) {
  Rec().translator_registered_for = QString::fromUtf8(id);
  Rec().translator_reader = reader;
  return 0;
}

void* GFUIGetGUIObject(const char* /*handle*/) { return nullptr; }

void GFModuleLogDebug(const char* /*msg*/) {}
void GFModuleLogInfo(const char* /*msg*/) {}
void GFModuleLogWarn(const char* msg) {
  Rec().warnings.append(QString::fromUtf8(msg));
}
void GFModuleLogError(const char* msg) {
  Rec().errors.append(QString::fromUtf8(msg));
}

// The module log macros route through GFModuleLogAt now, so the recording has
// to live here or the runtime's warnings and errors would stop being observed
// while every test still passed.
void GFModuleLogAt(const char* /*module_id*/, int severity,
                   const char* /*file*/, int /*line*/, const char* /*function*/,
                   const char* msg) {
  switch (severity) {
    case GF_LOG_WARN:
      GFModuleLogWarn(msg);
      break;
    case GF_LOG_ERROR:
      GFModuleLogError(msg);
      break;
    default:
      break;
  }
}

// Nothing is filtered in the harness: a test that asserts on a message must
// not depend on a level having been configured.
int GFModuleLogEnabled(const char* /*module_id*/, int /*severity*/) {
  return 1;
}

}  // extern "C"
