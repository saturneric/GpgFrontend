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

#include "KeyCategoryRules.h"

namespace GpgFrontend::UI {

namespace {

// Hues nearer than this read as the same colour at swatch size.
constexpr int kMinKeyCategoryHueDistance = 12;
// Below this a colour reads as a grey (the slate "All" tab is 72): its hue
// says nothing about how it looks.
constexpr int kMinChromaticSaturation = 100;

auto LooksAlike(const QColor& a, const QColor& b) -> bool {
  if (a.rgb() == b.rgb()) return true;
  if (a.hsvSaturation() < kMinChromaticSaturation ||
      b.hsvSaturation() < kMinChromaticSaturation) {
    return false;
  }

  const int diff = std::abs(a.hsvHue() - b.hsvHue());
  return std::min(diff, 360 - diff) < kMinKeyCategoryHueDistance;
}

}  // namespace

// The translate() context below is spelled out at every call site on purpose.
// lupdate is a static parser: hoisting it into a constant silently drops every
// string here from the .ts files, with no warning and no compile error.

auto ValidateKeyCategoryName(const QString& name, const QStringList& taken)
    -> KeyCategoryNameProblem {
  const auto trimmed = name.trimmed();
  if (trimmed.isEmpty()) return KeyCategoryNameProblem::kEmpty;
  if (trimmed.size() > kMaxKeyCategoryNameLength) {
    return KeyCategoryNameProblem::kTooLong;
  }

  const bool duplicate =
      std::any_of(taken.cbegin(), taken.cend(), [&](const QString& other) {
        return other.trimmed().compare(trimmed, Qt::CaseInsensitive) == 0;
      });
  if (duplicate) return KeyCategoryNameProblem::kDuplicate;

  return KeyCategoryNameProblem::kNone;
}

auto DescribeKeyCategoryNameProblem(KeyCategoryNameProblem problem) -> QString {
  switch (problem) {
    case KeyCategoryNameProblem::kNone:
    case KeyCategoryNameProblem::kEmpty:
      return {};

    case KeyCategoryNameProblem::kTooLong:
      return QCoreApplication::translate("GpgFrontend::UI::KeyCategoryRules",
                                         "The name is too long.");

    case KeyCategoryNameProblem::kDuplicate:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyCategoryRules",
          "A category with this name already exists.");
  }

  return {};
}

auto KeyCategoryPresetColors()
    -> std::array<KeyCategoryPresetColor, kKeyCategoryPresetColorCount> {
  // The same tone family as the built-in tabs' colours, so a custom category
  // sits beside them in the rail without looking out of place. In hue order,
  // so the grid reads as a colour wheel; a few deep tones close it.
  return {{
      {QColor(0xEF4444),
       QCoreApplication::translate("GpgFrontend::UI::KeyCategoryRules", "Red")},
      {QColor(0xF97316), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Orange")},
      {QColor(0xF59E0B), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Amber")},
      {QColor(0xEAB308), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Yellow")},
      {QColor(0x84CC16), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Lime")},
      {QColor(0x22C55E), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Green")},
      {QColor(0x10B981), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Emerald")},
      {QColor(0x14B8A6), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Teal")},
      {QColor(0x06B6D4), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Cyan")},
      {QColor(0x0EA5E9),
       QCoreApplication::translate("GpgFrontend::UI::KeyCategoryRules", "Sky")},
      {QColor(0x3B82F6), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Blue")},
      {QColor(0x6366F1), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Indigo")},
      {QColor(0x8B5CF6), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Violet")},
      {QColor(0xA855F7), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Purple")},
      {QColor(0xD946EF), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Fuchsia")},
      {QColor(0xEC4899), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Pink")},
      {QColor(0xF43F5E), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Rose")},
      {QColor(0x92400E), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Brown")},
      {QColor(0x1E3A8A), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Navy")},
      {QColor(0x78716C), QCoreApplication::translate(
                             "GpgFrontend::UI::KeyCategoryRules", "Stone")},
  }};
}

auto SuggestKeyCategoryColor(const QList<QColor>& used) -> QColor {
  const auto presets = KeyCategoryPresetColors();

  for (const auto& preset : presets) {
    const bool taken = std::any_of(
        used.cbegin(), used.cend(),
        [&](const QColor& c) { return LooksAlike(c, preset.color); });
    if (!taken) return preset.color;
  }

  return presets.at(static_cast<std::size_t>(used.size()) % presets.size())
      .color;
}

}  // namespace GpgFrontend::UI
