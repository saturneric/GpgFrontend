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

#include "ui/widgets/StatusIndicatorInfo.h"

#include "core/utils/GpgUtils.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief Join tooltip lines, dropping the ones that had nothing to say.
 *
 * Paths and the key database name are all optional, and an empty line in the
 * middle of a tooltip reads as a bug.
 */
auto JoinTooltip(const QStringList& lines, const QString& hint) -> QString {
  QStringList kept;
  for (const auto& line : lines) {
    if (!line.isEmpty()) kept.append(line);
  }
  if (kept.isEmpty()) return hint;
  if (hint.isEmpty()) return kept.join("\n");

  // The hint is what a click does, not more of the same detail, so it stands
  // apart from the lines above it.
  return kept.join("\n") + "\n\n" + hint;
}

auto ClickToManageProfiles() -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::StatusIndicatorInfo",
                                     "Click to manage profiles.");
}

auto ClickToSeeStatus() -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::StatusIndicatorInfo",
                                     "Click to see the full status.");
}

}  // namespace

auto DescribeProfileIndicator(ProfileKind kind, const QString& display_name,
                              const QString& root_path,
                              const QString& package_path, bool clickable)
    -> StatusIndicatorInfo {
  StatusIndicatorInfo info;
  info.caption = QCoreApplication::translate(
      "GpgFrontend::UI::StatusIndicatorInfo", "Profile");

  const auto packaged = kind == ProfileKind::kPACKAGED;

  info.value =
      packaged ? QCoreApplication::translate(
                     "GpgFrontend::UI::StatusIndicatorInfo", "%1 (temporary)")
                     .arg(display_name)
               : display_name;

  info.tooltip = JoinTooltip(
      {packaged ? QCoreApplication::translate(
                      "GpgFrontend::UI::StatusIndicatorInfo",
                      "Opened from a file, and not kept on this computer. "
                      "Closing asks whether to save the changes back into it.")
                : QCoreApplication::translate(
                      "GpgFrontend::UI::StatusIndicatorInfo",
                      "This window's profile — its own settings, keys and "
                      "saved state"),
       QDir::toNativeSeparators(packaged ? package_path : root_path)},
      clickable ? ClickToManageProfiles() : QString());

  return info;
}

auto DescribeEngineIndicator(OpenPGPEngine engine, const QString& version,
                             const QString& key_db_name,
                             const QString& key_db_path)
    -> StatusIndicatorInfo {
  StatusIndicatorInfo info;

  // No caption: "GnuPG 2.4.3" is already unmistakable, and the strip has three
  // things to fit into the corner of a status bar.
  const auto engine_name = ConvertOpenPGPEngine2String(engine);
  info.value = version.isEmpty() ? engine_name
                                 : QString("%1 %2").arg(engine_name, version);

  info.tooltip = JoinTooltip(
      {QCoreApplication::translate("GpgFrontend::UI::StatusIndicatorInfo",
                                   "Current OpenPGP backend and version"),
       key_db_name.isEmpty()
           ? QString()
           : QCoreApplication::translate("GpgFrontend::UI::StatusIndicatorInfo",
                                         "Key database: %1")
                 .arg(key_db_name),
       QDir::toNativeSeparators(key_db_path)},
      ClickToSeeStatus());

  return info;
}

auto DescribeDeploymentIndicator(bool portable_mode, bool self_contained)
    -> StatusIndicatorInfo {
  StatusIndicatorInfo info;

  // Same wording as the About dialog's Status tab, which is what a click on
  // this segment opens — two names for one thing would be worse than none.
  info.value =
      portable_mode
          ? QCoreApplication::translate("GpgFrontend::UI::StatusIndicatorInfo",
                                        "Portable Mode")
          : QCoreApplication::translate("GpgFrontend::UI::StatusIndicatorInfo",
                                        "Installed Mode");

  info.tooltip = JoinTooltip(
      {portable_mode
           ? QCoreApplication::translate(
                 "GpgFrontend::UI::StatusIndicatorInfo",
                 "Running from the folder it was unpacked into, taking its "
                 "settings and keys along with it.")
           : QCoreApplication::translate(
                 "GpgFrontend::UI::StatusIndicatorInfo",
                 "Installed on this computer, with its settings and keys kept "
                 "in this user's data folder."),
       self_contained ? QCoreApplication::translate(
                            "GpgFrontend::UI::StatusIndicatorInfo",
                            "This profile keeps its own keys, separate from "
                            "the rest of the computer.")
                      : QString()},
      ClickToSeeStatus());

  return info;
}

}  // namespace GpgFrontend::UI
