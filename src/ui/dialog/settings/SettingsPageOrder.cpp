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

#include "SettingsPageOrder.h"

#include <algorithm>

namespace GpgFrontend::UI {

auto SettingsSectionOrder() -> const QStringList& {
  static const QStringList kOrder{"application", "keys_engines", "features",
                                  "system"};
  return kOrder;
}

auto OrderSettingsPageDescriptors(QVector<SettingsPageDescriptor> descriptors,
                                  const QStringList& canonical_sections)
    -> QVector<SettingsPageDescriptor> {
  // A section the caller did not name still has to land somewhere predictable,
  // so it gets a rank past every canonical one, in the order it first shows up.
  // Doing this in its own pass means the comparison below is a plain integer
  // one and cannot depend on where an element currently sits.
  QHash<QString, int> rank_of_section;
  auto next_unknown_rank = static_cast<int>(canonical_sections.size());
  for (const auto& descriptor : descriptors) {
    if (rank_of_section.contains(descriptor.section_id)) continue;

    const auto canonical_index =
        canonical_sections.indexOf(descriptor.section_id);
    rank_of_section.insert(descriptor.section_id, canonical_index >= 0
                                                      ? canonical_index
                                                      : next_unknown_rank++);
  }

  // Stable, so pages sharing a section stay in the order they were collected —
  // that order is the only thing expressing the author's intent within a group.
  std::stable_sort(descriptors.begin(), descriptors.end(),
                   [&rank_of_section](const SettingsPageDescriptor& lhs,
                                      const SettingsPageDescriptor& rhs) {
                     return rank_of_section.value(lhs.section_id) <
                            rank_of_section.value(rhs.section_id);
                   });

  return descriptors;
}

}  // namespace GpgFrontend::UI
