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

#pragma once

namespace GpgFrontend::UI {

/**
 * @brief Hand the fonts bundled in ":/fonts" to the font database.
 *
 * Idempotent and safe to call from any thread: the work happens once, so a
 * second call cannot register a duplicate family (Qt tolerates that, but the
 * family would then be listed twice in every QFontComboBox).
 *
 * Does nothing when no QGuiApplication exists yet, since QFontDatabase needs
 * one. A core-only caller therefore degrades instead of crashing.
 */
void GF_UI_EXPORT RegisterBundledFonts();

/**
 * @brief Family name of the bundled monospaced font, read back from the font
 *        database rather than hard-coded.
 *
 * @return the family, or an empty string when the bundled font failed to load
 */
auto GF_UI_EXPORT BundledMonospaceFamily() -> QString;

/**
 * @brief A fresh monospaced font: the bundled family, or the system's
 *        fixed-pitch font if the bundle could not be loaded.
 *
 * This is what every surface that wants "a monospaced font" should ask for.
 * Hard-coding a family literal does not work across platforms -- "Monospace"
 * in particular is a Fontconfig alias that resolves on Linux only, and
 * elsewhere Qt silently substitutes a proportional font, which is how aligned
 * columns of hashes end up ragged.
 *
 * @param point_size point size to apply, or -1 to leave the size alone
 * @return the monospaced font
 */
auto GF_UI_EXPORT DefaultMonospaceFont(int point_size = -1) -> QFont;

/**
 * @brief Move @p font onto the monospaced family, keeping its size and weight.
 *
 * The retrofit counterpart of DefaultMonospaceFont(), for the surfaces that
 * start from a widget's own font on purpose so they inherit the surrounding
 * point size and only want the family changed.
 *
 * @param font font to adjust in place
 */
void GF_UI_EXPORT ApplyMonospaceFamily(QFont& font);

/**
 * @brief The font a text surface should use for a stored appearance setting.
 *
 * Starts from the bundled monospaced font (the system's fixed-pitch font if
 * the bundle failed to load) and only takes @p family over it when that family
 * is actually installed: a font that was uninstalled since it was chosen must
 * fall back to something readable rather than let Qt substitute an arbitrary
 * family.
 *
 * @param family stored family name, empty to keep the bundled monospaced font
 * @param point_size point size to apply
 * @return the resolved font
 */
auto GF_UI_EXPORT ResolveAppearanceFont(const QString& family, int point_size)
    -> QFont;

/**
 * @brief Base size of the generated report document, in device-independent
 *        pixels.
 *
 * Pixels rather than points, deliberately: Qt reports 72 logical DPI on macOS
 * and 96 everywhere else, so the same point size is a different physical size
 * per platform. The report used to inherit the platform's default UI point size
 * (13pt on macOS, 9pt on Windows, 10-11pt on most Linux desktops) and derive
 * every label from it by relative steps, which is how it ended up visibly
 * larger on macOS while the pixel-sized card chrome around it stayed put.
 *
 * 13px reproduces what Linux and Windows render today.
 */
constexpr int kReportBaseFontPx = 13;

/// Nothing in the report goes below this, however deep the step.
constexpr int kReportMinFontPx = 9;

/**
 * @brief A size on the report's typographic scale.
 *
 * @param delta_px step from kReportBaseFontPx, clamped at kReportMinFontPx
 * @return the pixel size to apply
 */
auto GF_UI_EXPORT ReportFontPixelSize(int delta_px) -> int;

/**
 * @brief The monospaced font a surface of the report document should use.
 *
 * Absolute, not derived from whatever font the surrounding widget happens to
 * carry: that is the whole point of the scale. Every label in the document asks
 * for its own step here instead of stepping off its parent.
 *
 * @param delta_px step from kReportBaseFontPx, clamped at kReportMinFontPx
 * @param bold whether to ask for the bold cut
 * @return the report font at that step
 */
auto GF_UI_EXPORT ReportFont(int delta_px = 0, bool bold = false) -> QFont;

/**
 * @brief Whether @p family is a monospaced family.
 *
 * Wraps the version split in QFontDatabase: Qt 6 made its query functions
 * static, while on Qt 5 they are members that need an instance.
 *
 * @param family family name to query
 * @return true when the family is fixed pitch
 */
auto GF_UI_EXPORT IsFixedPitchFontFamily(const QString& family) -> bool;

}  // namespace GpgFrontend::UI
