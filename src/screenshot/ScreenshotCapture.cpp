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

#include "screenshot/ScreenshotCapture.h"

#include <QJsonArray>
#include <QPainter>
#include <QPixmap>
#include <cmath>
#include <cstring>

namespace GpgFrontend::Screenshot {

namespace {

auto RectJson(const QRect& r) -> QJsonArray {
  return QJsonArray{r.x(), r.y(), r.width(), r.height()};
}

auto ToImage(const QPixmap& pixmap) -> QImage {
  auto image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  image.setDevicePixelRatio(pixmap.devicePixelRatio());
  return image;
}

/// Where @p highlight is, in logical coordinates relative to @p origin.
auto HighlightLogicalRect(const Highlight& highlight, const QPoint& origin)
    -> QRect {
  const auto local = highlight.local_rect.value_or(
      QRect(QPoint(0, 0), highlight.widget->size()));
  return {highlight.widget->mapToGlobal(local.topLeft()) - origin,
          local.size()};
}

void PlaceHighlights(Capture& capture, const QList<Highlight>& highlights) {
  const auto image_pixels = capture.image.size();
  for (const auto& h : highlights) {
    if (h.widget.isNull() || !h.widget->isVisible()) {
      capture.missing.append(h.name);
      continue;
    }
    const auto placed = MapToImage(HighlightLogicalRect(h, capture.origin),
                                   capture.dpr, image_pixels);
    if (!placed) {
      capture.missing.append(h.name);
      continue;
    }
    capture.widgets.insert(h.name, *placed);
  }
}

}  // namespace

auto CaptureModeName(CaptureMode mode) -> QString {
  switch (mode) {
    case CaptureMode::kDIALOG:
      return u"dialog"_s;
    case CaptureMode::kCOMPOSITE:
      return u"composite"_s;
    case CaptureMode::kWINDOW:
    default:
      return u"window"_s;
  }
}

auto MapToImage(const QRect& logical, qreal dpr, const QSize& image_pixels)
    -> std::optional<PlacedRect> {
  const auto scale = [dpr](int v) {
    return static_cast<int>(std::lround(v * dpr));
  };
  const QRect pixels(scale(logical.x()), scale(logical.y()),
                     scale(logical.width()), scale(logical.height()));
  const QRect bounds(QPoint(0, 0), image_pixels);
  const auto inside = pixels.intersected(bounds);
  if (inside.isEmpty()) return std::nullopt;
  return PlacedRect{inside, inside != pixels};
}

auto CompositeBounds(const QRect& window_global,
                     const QList<QRect>& popups_global) -> QRect {
  auto bounds = window_global;
  for (const auto& popup : popups_global) bounds = bounds.united(popup);
  return bounds;
}

auto CaptureWidget(QWidget* target, CaptureMode mode,
                   const QList<Highlight>& highlights) -> Capture {
  Capture capture;
  capture.mode = mode;
  capture.image = ToImage(target->grab());
  capture.dpr = capture.image.devicePixelRatio();
  capture.origin = target->mapToGlobal(QPoint(0, 0));
  capture.layers.append(
      {u"window"_s, QRect(QPoint(0, 0), capture.image.size())});
  PlaceHighlights(capture, highlights);
  return capture;
}

auto CaptureComposite(QWidget* window, const QList<QWidget*>& popups,
                      const QList<Highlight>& highlights) -> Capture {
  const auto window_image = ToImage(window->grab());
  const auto dpr = window_image.devicePixelRatio();
  const QRect window_global(window->mapToGlobal(QPoint(0, 0)), window->size());

  QList<std::pair<QImage, QRect>> layers;
  QList<QRect> popups_global;
  for (auto* popup : popups) {
    const QRect global(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    layers.append({ToImage(popup->grab()), global});
    popups_global.append(global);
  }
  const auto canvas = CompositeBounds(window_global, popups_global);

  Capture capture;
  capture.mode = CaptureMode::kCOMPOSITE;
  capture.dpr = dpr;
  capture.origin = canvas.topLeft();
  capture.image = QImage(canvas.size() * dpr, QImage::Format_ARGB32);
  capture.image.fill(Qt::transparent);
  capture.image.setDevicePixelRatio(dpr);

  {
    QPainter painter(&capture.image);
    painter.drawImage(window_global.topLeft() - canvas.topLeft(), window_image);
    for (const auto& [image, global] : layers) {
      painter.drawImage(global.topLeft() - canvas.topLeft(), image);
    }
  }

  const auto add_layer = [&](const QString& name, const QRect& global) {
    const auto placed = MapToImage(global.translated(-canvas.topLeft()), dpr,
                                   capture.image.size());
    if (placed) capture.layers.append({name, placed->rect});
  };
  add_layer(u"window"_s, window_global);
  for (qsizetype i = 0; i < popups_global.size(); ++i) {
    add_layer(u"popup%1"_s.arg(i + 1), popups_global.at(i));
  }

  PlaceHighlights(capture, highlights);
  return capture;
}

auto SameImage(const QImage& a, const QImage& b) -> bool {
  if (a.size() != b.size()) return false;
  const auto ca = a.convertToFormat(QImage::Format_ARGB32);
  const auto cb = b.convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < ca.height(); ++y) {
    if (std::memcmp(ca.constScanLine(y), cb.constScanLine(y),
                    static_cast<size_t>(ca.bytesPerLine())) != 0) {
      return false;
    }
  }
  return true;
}

auto GeometryJson(const Capture& capture, const QString& scenario)
    -> QJsonObject {
  QJsonArray layers;
  for (const auto& layer : capture.layers) {
    layers.append(
        QJsonObject{{"name", layer.name}, {"rect", RectJson(layer.rect)}});
  }

  QJsonObject widgets;
  for (auto it = capture.widgets.cbegin(); it != capture.widgets.cend(); ++it) {
    widgets.insert(it.key(), QJsonObject{{"rect", RectJson(it->rect)},
                                         {"clipped", it->clipped}});
  }

  return QJsonObject{
      {"schema", 1},
      {"scenario", scenario},
      {"capture_mode", CaptureModeName(capture.mode)},
      {"dpr", static_cast<int>(std::lround(capture.dpr))},
      {"image_size", QJsonArray{capture.image.width(), capture.image.height()}},
      {"origin", QJsonArray{capture.origin.x(), capture.origin.y()}},
      {"layers", layers},
      {"widgets", widgets},
      {"missing", QJsonArray::fromStringList(capture.missing)},
  };
}

}  // namespace GpgFrontend::Screenshot
