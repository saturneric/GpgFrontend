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

#include <QImage>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QWidget>
#include <optional>

/**
 * @file ScreenshotCapture.h
 * @brief Turning widgets into an image plus the geometry of what is in it.
 *
 * Nothing is ever read back from the screen. A window is grabbed with
 * QWidget::grab(), and a popup is grabbed the same way and painted onto the
 * window's image, so overlapping windows, compositor shadows and the
 * wallpaper cannot leak into a capture.
 */

namespace GpgFrontend::Screenshot {

/**
 * @brief A widget, or a part of one, whose position the pipeline wants.
 */
struct Highlight {
  QString name;
  QPointer<QWidget> widget;
  std::optional<QRect>
      local_rect;  ///< in widget coordinates; whole widget if unset
};

/**
 * @brief A rect in image pixels, and whether it had to be cut to fit.
 */
struct PlacedRect {
  QRect rect;
  bool clipped = false;
};

struct Layer {
  QString name;
  QRect rect;  ///< image pixels
};

enum class CaptureMode : std::uint8_t { kWINDOW, kDIALOG, kCOMPOSITE };

auto CaptureModeName(CaptureMode mode) -> QString;

struct Capture {
  CaptureMode mode = CaptureMode::kWINDOW;
  QImage image;
  qreal dpr = 1.0;
  QPoint origin;  ///< logical global position of the image's top-left
  QList<Layer> layers;
  QMap<QString, PlacedRect> widgets;
  QStringList missing;
};

/**
 * @brief Place a rect given in logical coordinates relative to the image
 *        origin into image pixels, cut to the image.
 *
 * @return nullopt when nothing of it is inside the image
 */
/// Logical height of the title bar drawn above every capture.
inline constexpr int kCaptionHeight = 32;

/**
 * @brief A Windows 11 style title bar for @p window, @p width logical pixels
 *        wide: icon, title and the caption buttons the window would have.
 *
 * Drawn, never grabbed: the real frame belongs to the desktop compositor and
 * carries the accent colour, focus state and whatever lies behind it.
 */
auto RenderCaption(const QWidget* window, int width, qreal dpr) -> QImage;

auto MapToImage(const QRect& logical, qreal dpr, const QSize& image_pixels)
    -> std::optional<PlacedRect>;

/**
 * @brief The logical canvas a window and its popups share.
 */
auto CompositeBounds(const QRect& window_global,
                     const QList<QRect>& popups_global) -> QRect;

/**
 * @brief Grab @p target alone.
 */
auto CaptureWidget(QWidget* target, CaptureMode mode,
                   const QList<Highlight>& highlights) -> Capture;

/**
 * @brief Grab @p window and each of @p popups and paint them onto one
 *        canvas in that order, each where it is on screen relative to the
 *        window. Outside all of them is transparent.
 */
auto CaptureComposite(QWidget* window, const QList<QWidget*>& popups,
                      const QList<Highlight>& highlights) -> Capture;

auto SameImage(const QImage& a, const QImage& b) -> bool;

/**
 * @brief The geometry JSON written next to the PNG, without "env" and
 *        "error", which the caller adds.
 */
auto GeometryJson(const Capture& capture, const QString& scenario)
    -> QJsonObject;

}  // namespace GpgFrontend::Screenshot
