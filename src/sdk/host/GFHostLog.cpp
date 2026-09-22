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

#include <QLoggingCategory>
#include <QMessageLogger>
#include <QString>

#include "GFHostImpl.h"
#include "core/module/ModuleLogCategory.h"
#include "private/GFHostAttribution.h"

/// Where a message the host could not attribute to any module goes.
///
/// Every module used to share this one category, which is why a log line could
/// never say who wrote it. It is now the fallback alone.
Q_LOGGING_CATEGORY(module, "module.unknown")

namespace {

/// The module a log call should be filed under.
///
/// The host's own thread-local record wins wherever it exists, because it is
/// the half a module does not write. The argument is the fallback, and it is
/// what makes attribution survive a module's own worker thread -- a thread the
/// host never entered, and where the thread-local is therefore empty.
auto ResolveModuleId(const char* module_id) -> QString {
  const auto* entered = GFSdkCurrentModule();
  if (entered != nullptr && *entered != '\0') {
    return QString::fromUtf8(entered);
  }
  if (module_id != nullptr && *module_id != '\0') {
    return QString::fromUtf8(module_id);
  }
  return {};
}

/// The category one message belongs to.
///
/// Trace gets a `.trace` child so that Qt's own rule matching can silence it
/// without silencing debug. An id that resolves to nothing lands on the shared
/// fallback category rather than on a category named after an empty string.
auto CategoryFor(const QString& module_id, int severity)
    -> const QLoggingCategory& {
  if (module_id.isEmpty()) return module();

  return severity == GF_LOG_TRACE
             ? GpgFrontend::Module::ModuleTraceLogCategory(module_id)
             : GpgFrontend::Module::ModuleLogCategory(module_id);
}

auto IsEnabled(const QLoggingCategory& category, int severity) -> bool {
  switch (severity) {
    case GF_LOG_TRACE:
    case GF_LOG_DEBUG:
      return category.isDebugEnabled();
    case GF_LOG_INFO:
      return category.isInfoEnabled();
    case GF_LOG_WARN:
      return category.isWarningEnabled();
    case GF_LOG_ERROR:
      return category.isCriticalEnabled();
    default:
      // An unknown severity is emitted rather than dropped: a module compiled
      // against a newer SDK saying something is better heard at the wrong
      // volume than not at all.
      return category.isCriticalEnabled();
  }
}

}  // namespace

namespace gf_host {

void GFModuleLogAt(const char* module_id, int severity, const char* file,
                   int line, const char* function, const char* msg) {
  const auto id = ResolveModuleId(module_id);
  const auto& category = CategoryFor(id, severity);
  if (!IsEnabled(category, severity)) return;

  const auto text = msg == nullptr ? QString() : QString::fromUtf8(msg);

  // Constructed here rather than through the qC* macros on purpose: the macros
  // capture the context of wherever they are written, which is THIS file, and
  // that is how every module line came to report GFSDKLog.cpp as its source.
  // Handing QMessageLogger the module's own file and line is the whole fix.
  const QMessageLogger logger(file, line, function, category.categoryName());

  // noquote: what arrives here is a finished message, not a QString to be
  // inspected. Without it every module line was wrapped in quotes it never
  // asked for, and a message that itself contained a quote got escaped.
  switch (severity) {
    case GF_LOG_TRACE:
    case GF_LOG_DEBUG:
      logger.debug().noquote() << text;
      break;
    case GF_LOG_INFO:
      logger.info().noquote() << text;
      break;
    case GF_LOG_WARN:
      logger.warning().noquote() << text;
      break;
    default:
      logger.critical().noquote() << text;
      break;
  }
}

auto GFModuleLogEnabled(const char* module_id, int severity) -> int {
  const auto id = ResolveModuleId(module_id);
  return IsEnabled(CategoryFor(id, severity), severity) ? 1 : 0;
}

void GFModuleLogTrace(const char* l) {
  GFModuleLogAt(nullptr, GF_LOG_TRACE, nullptr, 0, nullptr, l);
}

void GFModuleLogDebug(const char* l) {
  GFModuleLogAt(nullptr, GF_LOG_DEBUG, nullptr, 0, nullptr, l);
}

void GFModuleLogInfo(const char* l) {
  GFModuleLogAt(nullptr, GF_LOG_INFO, nullptr, 0, nullptr, l);
}

void GFModuleLogWarn(const char* l) {
  GFModuleLogAt(nullptr, GF_LOG_WARN, nullptr, 0, nullptr, l);
}

void GFModuleLogError(const char* l) {
  GFModuleLogAt(nullptr, GF_LOG_ERROR, nullptr, 0, nullptr, l);
}

}  // namespace gf_host