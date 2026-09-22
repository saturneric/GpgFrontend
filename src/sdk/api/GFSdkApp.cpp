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

#include "GFSDKApp.h"

#include <QString>
#include <QStringList>
#include <algorithm>

#include "GFSdkInternal.h"

/**
 * @file GFSdkApp.cpp
 * @brief Application facts, and the one helper that needs none of them.
 */

auto GFAppVersion(GFSDKContext* ctx) -> const char* {
  GF_SDK_REQUIRE(ctx, app, "GFAppVersion", "");
  return g->version(hctx);
}

auto GFAppGitCommitHash(GFSDKContext* ctx) -> const char* {
  GF_SDK_REQUIRE(ctx, app, "GFAppGitCommitHash", "");
  return g->git_commit_hash(hctx);
}

auto GFAppQtVersion(GFSDKContext* ctx) -> const char* {
  GF_SDK_REQUIRE(ctx, app, "GFAppQtVersion", "");
  return g->qt_env_version(hctx);
}

auto GFAppUserAgent(GFSDKContext* ctx) -> const char* {
  GF_SDK_REQUIRE(ctx, app, "GFAppUserAgent", "");
  return g->http_user_agent(hctx);
}

auto GFAppLocale(GFSDKContext* ctx) -> char* {
  GF_SDK_REQUIRE(ctx, app, "GFAppLocale", nullptr);
  return g->active_locale(hctx);
}

auto GFAppIsFlatpak(GFSDKContext* ctx) -> int {
  GF_SDK_REQUIRE(ctx, app, "GFAppIsFlatpak", 0);
  return g->is_flatpak(hctx);
}

auto GFAppKeyProtectionLevel(GFSDKContext* ctx) -> int {
  GF_SDK_REQUIRE(ctx, app, "GFAppKeyProtectionLevel", -1);
  return g->key_protection_level(hctx);
}

/**
 * @brief PURE. No context, no host, no boundary crossed.
 *
 * A faithful copy of the host's own ordering, quirks included: a leading "v"
 * is dropped, only the components both versions have are compared
 * numerically, and a version with MORE components then sorts after one with
 * fewer, so "2.1.0" is greater than "2.1". Modules already depend on this
 * ordering; changing it while moving it would change update prompts, which is
 * not what a refactor is for.
 */
auto GFCompareSoftwareVersion(const char* current, const char* latest) -> int {
  const auto strip = [](const char* s) {
    auto v = QString::fromUtf8(s == nullptr ? "" : s);
    return v.startsWith('v') ? v.mid(1) : v;
  };

  const auto a = strip(current).split('.');
  const auto b = strip(latest).split('.');

  const auto min_depth = std::min(a.size(), b.size());
  for (qsizetype i = 0; i < min_depth; ++i) {
    const auto num_a = a[i].toInt();
    const auto num_b = b[i].toInt();
    if (num_a != num_b) return num_a > num_b ? 1 : -1;
  }

  if (a.size() != b.size()) return a.size() > b.size() ? 1 : -1;
  return 0;
}
