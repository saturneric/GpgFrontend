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

#include "GFSdkInternal.h"

#include <QString>

/**
 * @file GFSdkInternal.cpp
 * @brief The one function in this directory that is not a public entry point.
 */

namespace gf_sdk_api {

void GFSdkReportUnavailable(const GFSDKContext* ctx, const char* group,
                            const char* entry_point) {
  // No context, or a context with no table: the module is calling before it
  // was activated. There is nothing to log through, and inventing a channel
  // for it would mean holding state.
  if (ctx == nullptr || ctx->host == nullptr) return;

  const auto* log = ctx->host->log;
  if (log == nullptr || log->write == nullptr) return;

  const auto message =
      QString(
          "%1 needs the \"%2\" capability, which this module's signed "
          "manifest does not declare. Add it to module.json's "
          "\"capabilities\" and rebuild; the call did nothing.")
          .arg(QLatin1String(entry_point), QLatin1String(group));

  log->write(ctx->host->context, GF_LOG_ERROR, __FILE__, __LINE__,
             "GFSdkReportUnavailable", message.toUtf8().constData());
}

}  // namespace gf_sdk_api
