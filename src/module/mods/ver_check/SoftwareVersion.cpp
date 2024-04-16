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

#include "SoftwareVersion.h"

#include <GFSDKBasic.h>
#include <GFSDKExtra.h>
#include <GFSDKLog.h>
#include <spdlog/spdlog.h>

#include <QString>

template <>
struct fmt::formatter<QString> {
  // Parses format specifications.
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  // Formats the QString qstr and writes it to the output.
  template <typename FormatContext>
  auto format(const QString& qstr, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    // Convert QString to UTF-8 QString (to handle Unicode characters
    // correctly)
    QByteArray utf8_array = qstr.toUtf8();
    return fmt::format_to(ctx.out(), "{}", utf8_array.constData());
  }
};

template <>
struct fmt::formatter<QByteArray> {
  // Parses format specifications.
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  // Formats the QString qstr and writes it to the output.
  template <typename FormatContext>
  auto format(const QByteArray& qarray, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    // Convert QString to UTF-8 QString (to handle Unicode characters
    // correctly)
    return fmt::format_to(ctx.out(), "{}", qarray.constData());
  }
};

auto SoftwareVersion::NeedUpgrade() const -> bool {
  GFModuleLogDebug(
      fmt::format(
          "compair version current {} latest {}, result {}", current_version,
          latest_version,
          GFCompareSoftwareVersion(GFModuleStrDup(current_version.toUtf8()),
                                   GFModuleStrDup(latest_version.toUtf8())))
          .c_str());

  GFModuleLogDebug(fmt::format("load done: {}, pre-release: {}, draft: {}",
                               loading_done,
                               latest_prerelease_version_from_remote,
                               latest_draft_from_remote)
                       .c_str());
  return loading_done && !latest_prerelease_version_from_remote &&
         !latest_draft_from_remote &&
         GFCompareSoftwareVersion(GFModuleStrDup(current_version.toUtf8()),
                                  GFModuleStrDup(latest_version.toUtf8())) < 0;
}

auto SoftwareVersion::VersionWithdrawn() const -> bool {
  return loading_done && !current_version_publish_in_remote &&
         current_version_is_a_prerelease && !current_version_is_drafted;
}

auto SoftwareVersion::CurrentVersionReleased() const -> bool {
  return loading_done && current_version_publish_in_remote;
}