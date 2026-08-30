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

#include "ui/function/AppearanceFont.h"

#include <algorithm>
#include <array>
#include <mutex>

namespace GpgFrontend::UI {

namespace {

// Both weights report the same family, so registering the bold one only makes
// the weight real instead of letting Qt synthesise a fake bold from regular.
constexpr std::array<const char*, 2> kBundledFontPaths{
    ":/fonts/JetBrainsMonoNL-Regular.ttf",
    ":/fonts/JetBrainsMonoNL-Bold.ttf",
};

auto bundled_family() -> QString& {
  static QString family;
  return family;
}

}  // namespace

void RegisterBundledFonts() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    // QFontDatabase needs a QGuiApplication. Degrade rather than crash: the
    // family stays empty and every caller falls back to the system font.
    if (qGuiApp == nullptr) {
      LOG_W() << "no gui application yet, bundled fonts were not registered";
      return;
    }

    for (const auto* path : kBundledFontPaths) {
      const auto id =
          QFontDatabase::addApplicationFont(QString::fromLatin1(path));
      if (id < 0) {
        LOG_W() << "failed to register bundled font:" << path;
        continue;
      }

      // Read the name back instead of hard-coding it: the family string in the
      // font file is the only authority, and it changes with the cut shipped.
      const auto families = QFontDatabase::applicationFontFamilies(id);
      if (!families.isEmpty() && bundled_family().isEmpty()) {
        bundled_family() = families.constFirst();
      }
    }

    LOG_D() << "bundled monospace family:" << bundled_family();
  });
}

auto BundledMonospaceFamily() -> QString {
  RegisterBundledFonts();
  return bundled_family();
}

auto DefaultMonospaceFont(int point_size) -> QFont {
  const auto family = BundledMonospaceFamily();
  const auto system_fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);

  QFont font = system_fixed;
  if (!family.isEmpty()) {
    // Deliberately keeping the system fixed font's size rather than starting
    // from QFont(family): a bare QFont takes the *application* default size,
    // which is the larger UI size. Callers here are replacing
    // systemFont(FixedFont) and clamp the result with std::max(), so a bigger
    // base would survive the clamp and visibly inflate the surface. Only the
    // family is ours to change.
    font.setFamily(family);
  }
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);

  // Guarded so the retrofit path can keep whatever size it inherited.
  if (point_size > 0) font.setPointSize(point_size);
  return font;
}

void ApplyMonospaceFamily(QFont& font) {
  const auto family = BundledMonospaceFamily();

  font.setFamily(
      family.isEmpty()
          ? QFontDatabase::systemFont(QFontDatabase::FixedFont).family()
          : family);
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);
}

auto IsFixedPitchFontFamily(const QString& family) -> bool {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QFontDatabase::isFixedPitch(family);
#else
  QFontDatabase font_database;
  return font_database.isFixedPitch(family);
#endif
}

auto ResolveAppearanceFont(const QString& family, int point_size) -> QFont {
  // Cached: this is asked again for every text surface each time appearance
  // settings are applied, and enumerating the installed families is not cheap.
  static const auto kFamilies = []() -> QStringList {
    // Ordering contract, do not drop as redundant: the cache is built once and
    // never refreshed, so a family registered after this point would stay
    // invisible forever. Registering here rather than relying on the startup
    // call makes the ordering structural instead of a convention that the next
    // startup refactor can quietly break. call_once makes it free after boot.
    RegisterBundledFonts();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QFontDatabase::families();
#else
    QFontDatabase font_database;
    return font_database.families();
#endif
  }();

  QFont font = DefaultMonospaceFont();

  const auto installed =
      !family.isEmpty() &&
      std::any_of(kFamilies.cbegin(), kFamilies.cend(),
                  [&family](const QString& item) {
                    return item.compare(family, Qt::CaseInsensitive) == 0;
                  });
  if (installed) {
    // The family the user picked wins over the fallback, and so do its own
    // metrics: asking for a proportional family while still requesting fixed
    // pitch lets Qt substitute something else entirely, which is how a chosen
    // script-specific font ends up rendered in the wrong one.
    const auto fixed_pitch = IsFixedPitchFontFamily(family);
    font.setFamily(family);
    font.setStyleHint(fixed_pitch ? QFont::Monospace : QFont::AnyStyle);
    font.setFixedPitch(fixed_pitch);
  }

  font.setPointSize(point_size);
  return font;
}

auto ReportFontPixelSize(int delta_px) -> int {
  return std::max(kReportMinFontPx, kReportBaseFontPx + delta_px);
}

auto ReportFont(int delta_px, bool bold) -> QFont {
  // Built on DefaultMonospaceFont() so the bundled-family lookup and its
  // fallback live in exactly one place; only the size is ours to pin.
  QFont font = DefaultMonospaceFont();
  font.setPixelSize(ReportFontPixelSize(delta_px));
  font.setBold(bold);
  return font;
}

}  // namespace GpgFrontend::UI
