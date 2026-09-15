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

#include "GFSDKModuleAttribution.h"

#include <QByteArray>
#include <QMap>
#include <QString>

#include "core/utils/CommonUtils.h"
#include "private/GFSDKHandleSweep.h"
#include "private/GFSDKPrivat.h"

namespace {

/**
 * @brief The module this thread is working for, as a stable C string.
 *
 * Stored as a QByteArray rather than a QString because GFSdkEnterModule()
 * hands the previous value back to the caller as a `const char*` to restore.
 * Keeping every value this thread has seen alive for the life of the thread
 * is what makes that pointer safe to hold across the nested call: the list is
 * bounded by nesting depth, which is bounded by the host's own call depth.
 */
struct ThreadAttribution {
  QList<QByteArray> stack;
  const char* current = nullptr;
};

auto Attribution() -> ThreadAttribution& {
  thread_local ThreadAttribution state;
  return state;
}

}  // namespace

namespace gf_sdk_internal {

auto CurrentModuleId() -> QString {
  const auto* id = Attribution().current;
  return id == nullptr ? QString() : QString::fromUtf8(id);
}

}  // namespace gf_sdk_internal

auto GFSdkEnterModule(const char* module_id) -> const char* {
  auto& state = Attribution();
  const auto* previous = state.current;

  if (module_id == nullptr || *module_id == '\0') {
    state.current = nullptr;
    return previous;
  }

  state.stack.append(QByteArray(module_id));
  state.current = state.stack.last().constData();
  return previous;
}

void GFSdkLeaveModule(const char* previous) {
  auto& state = Attribution();
  state.current = previous;

  // Nothing is popped here. `previous` is a pointer INTO this thread's stack,
  // so shrinking it would be what invalidates the value being restored. The
  // stack is bounded by the host's call depth into modules, which is shallow,
  // and it dies with the thread.
}

auto GFSdkCurrentModule() -> const char* { return Attribution().current; }

auto GFSdkSweepModuleHandles(const char* module_id) -> size_t {
  if (module_id == nullptr || *module_id == '\0') return 0;
  const auto id = QString::fromUtf8(module_id);

  // Every reclaimed handle, grouped by the entry point that issued it. A bare
  // count says a module leaked; this says what kind of call it leaked from,
  // which is the difference between a number and somewhere to look.
  QMap<QString, int> by_origin;
  const auto tally = [&by_origin](const QList<const char*>& origins) {
    for (const auto* origin : origins) {
      by_origin[origin == nullptr ? QStringLiteral("unknown")
                                  : QString::fromUtf8(origin)]++;
    }
  };

  // Results first, deliberately. A result owns a buffer handle and releases it
  // through the ordinary path, so reclaiming results first takes those nested
  // buffers out of the buffer registry properly -- sweeping buffers first
  // would destroy them behind the results still pointing at them.
  tally(gf_sdk_internal::SweepResultHandles(id));
  tally(gf_sdk_internal::SweepBufferHandles(id));
  tally(gf_sdk_internal::SweepListHandles(id));

  size_t total = 0;
  QStringList detail;
  for (auto it = by_origin.constBegin(); it != by_origin.constEnd(); ++it) {
    total += static_cast<size_t>(it.value());
    detail.append(QString("%1 x %2").arg(it.value()).arg(it.key()));
  }

  if (total != 0) {
    LOG_W() << "module" << id << "leaked" << total
            << "sdk handle(s), reclaimed at unload:" << detail.join(", ");
  }

  return total;
}
