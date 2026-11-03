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

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QLabel>
#include <QPalette>
#include <QWidget>

#include "GFModuleLog.h"
#include "GFSDKUI.h"

/**
 * @file GFModuleTheme.h
 * @brief The application's colours in a module's own widgets.
 *
 * Header-only, and not part of GFModule.h: it needs QtWidgets, which not
 * every module links. Applied through palettes, never stylesheets: a
 * stylesheet on one widget opts it out of the theme for every other property
 * too.
 */

namespace gf::ui {

/**
 * @brief One of the application's colours, by role (GF_UI_COLOR_*).
 *
 * From the application palette: no widget crosses to the Host to ask. When
 * the Host cannot say, @p w's text colour (or the application's, for none)
 * is the honest fallback.
 */
inline auto ThemeColor(const QWidget* w, int role) -> QColor {
  const auto rgba = GFUIThemeColorForRole(GFModuleSdkContext(), role);
  if (rgba != 0) return QColor::fromRgba(rgba);
  return (w != nullptr ? w->palette() : QApplication::palette())
      .color(QPalette::WindowText);
}

inline auto MutedColor(const QWidget* w) -> QColor {
  return ThemeColor(w, GF_UI_COLOR_MUTED_TEXT);
}

inline auto BorderColor(const QWidget* w) -> QColor {
  return ThemeColor(w, GF_UI_COLOR_BORDER);
}

inline auto WarningColor(const QWidget* w) -> QColor {
  return ThemeColor(w, GF_UI_COLOR_WARNING);
}

/**
 * @brief @p a moved @p t of the way towards @p b.
 *
 * Mixed rather than given an alpha: these colours are painted onto widgets
 * that fill their own background, where a translucent brush composites
 * against whatever Qt last drew there instead of against the page.
 */
inline auto Blend(const QColor& a, const QColor& b, double t) -> QColor {
  return QColor::fromRgbF(
      static_cast<float>(a.redF() * (1 - t) + b.redF() * t),
      static_cast<float>(a.greenF() * (1 - t) + b.greenF() * t),
      static_cast<float>(a.blueF() * (1 - t) + b.blueF() * t));
}

/**
 * @brief Whether @p event means colours taken from the palette have gone
 * stale.
 *
 * A colour derived from the palette once and baked into a child's palette is
 * no longer a palette lookup, so a theme change leaves it behind. Qt
 * announces the change; listening for it is all that is needed.
 * ApplicationPaletteChange because a palette set on the application never
 * reaches a widget with an explicit palette of its own as a PaletteChange;
 * ThemeChange because a system-wide light/dark switch arrives as that.
 *
 * Whatever this triggers must be safe to run twice: it may set colours, not
 * fonts.
 */
inline auto IsRestyle(const QEvent* event) -> bool {
  const auto type = event->type();
  return type == QEvent::PaletteChange || type == QEvent::ThemeChange ||
         type == QEvent::ApplicationPaletteChange;
}

/// Paints @p label's text in @p color, through its palette.
inline void SetLabelColor(QLabel* label, const QColor& color) {
  auto palette = label->palette();
  palette.setColor(QPalette::WindowText, color);
  palette.setColor(QPalette::Text, color);
  label->setPalette(palette);
}

}  // namespace gf::ui
