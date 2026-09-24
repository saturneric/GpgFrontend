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
#include <QStringList>
#include <array>

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

  // Which capabilities put each grantable group into the table -- the same
  // rule FillHostApiGroups() applies on the host side. The rest are always
  // present, so a missing one means the host is too old or broken, not that
  // the manifest forgot something, and telling the author to declare it
  // would mislead.
  struct Grantable {
    const char* group;
    const char* capabilities;
    uint32_t bits;
  };
  static const std::array<Grantable, 8> kGrantable = {{
      {"gpg", "gpg", GF_HOST_CAP_GPG},
      {"pgp", "pgp", GF_HOST_CAP_PGP},
      {"ui", "ui", GF_HOST_CAP_UI},
      {"editor", "editor", GF_HOST_CAP_EDITOR},
      {"storage", "storage", GF_HOST_CAP_STORAGE},
      {"process", "process", GF_HOST_CAP_PROCESS},
      {"script", "ui", GF_HOST_CAP_UI},
      {"native", "ui\" and \"ui.custom",
       GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM},
  }};
  const Grantable* grantable = nullptr;
  for (const auto& g : kGrantable) {
    if (qstrcmp(g.group, group) == 0) grantable = &g;
  }

  // What the table says was granted is the fact; a group whose bits were all
  // granted and is still missing is the host's problem, not the manifest's.
  const bool withheld =
      grantable != nullptr &&
      (ctx->host->granted & grantable->bits) != grantable->bits;
  const auto message =
      withheld
          ? QString(
                "%1 needs the \"%2\" capability, which this module's signed "
                "manifest does not declare. Add it to module.json's "
                "\"capabilities\" and rebuild; the call did nothing.")
                .arg(QLatin1String(entry_point),
                     QLatin1String(grantable->capabilities))
          : QString(
                "%1 needs the \"%2\" group, which the host did not provide. "
                "The host is older than this module or failed to start the "
                "SDK; the call did nothing.")
                .arg(QLatin1String(entry_point), QLatin1String(group));

  log->write(ctx->host->context, GF_LOG_ERROR, __FILE__, __LINE__,
             "GFSdkReportUnavailable", message.toUtf8().constData());
}

}  // namespace gf_sdk_api
