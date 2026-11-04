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

#include "core/utils/ReportSealPainter.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <cmath>

namespace GpgFrontend {

auto BlendReportColor(const QColor& a, const QColor& b, int wa, int wb)
    -> QColor {
  const int w = wa + wb;
  return {(a.red() * wa + b.red() * wb) / w,
          (a.green() * wa + b.green() * wb) / w,
          (a.blue() * wa + b.blue() * wb) / w};
}

void PaintReportSeal(QPainter& p, const QRectF& bounds, ReportSealKind kind,
                     const QString& inscription, const ReportSealStyle& style,
                     bool detail) {
  constexpr double kPi = 3.14159265358979323846;
  // Stamped, not placed: a small fixed counterclockwise tilt, the same in
  // every render. The whole seal turns as one; a circle keeps its bounds, so
  // nothing clips.
  constexpr double kTiltDegrees = -8.0;

  p.save();
  p.setRenderHint(QPainter::Antialiasing);
  // The seal is the right-hand square; the waves fill what is left of it.
  const double side = bounds.height();
  const QRectF seal_rect(bounds.right() - side, bounds.top(), side, side);
  p.translate(seal_rect.center());
  p.rotate(kTiltDegrees);
  p.translate(-seal_rect.center());

  const QRectF box = seal_rect.adjusted(2.0, 2.0, -2.0, -2.0);
  const QPointF c = box.center();
  const double r = box.width() / 2.0;
  const bool information = kind == ReportSealKind::kINFORMATION;

  // The ink: the outcome's colour, a little muted; the symbol stronger.
  const QColor ink = information
                         ? style.muted
                         : BlendReportColor(style.accent, style.muted, 4, 1);
  const QColor symbol_ink =
      information ? style.muted
                  : BlendReportColor(style.accent, style.text, 6, 1);

  // Shallow relief: a pale edge up-left, a faint shade down-right, the stroke
  // itself graded slightly within its own colour.
  const QColor lift =
      style.dark ? QColor(255, 255, 255, 30) : QColor(255, 255, 255, 200);
  const QColor deep = ink.darker(170);
  const QColor shade = style.dark
                           ? QColor(0, 0, 0, 90)
                           : QColor(deep.red(), deep.green(), deep.blue(), 55);
  const auto graded = [&](const QColor& base) -> QBrush {
    QLinearGradient g(box.topLeft(), box.bottomRight());
    g.setColorAt(0.0, base.lighter(style.dark ? 118 : 112));
    g.setColorAt(1.0, base.darker(style.dark ? 104 : 112));
    return {g};
  };
  const auto relief = [&](const auto& draw, double width, const QColor& base,
                          Qt::PenCapStyle cap) {
    const double d = std::max(0.4, width * 0.28);
    QPen pen(shade, width, Qt::SolidLine, cap, Qt::RoundJoin);
    p.setPen(pen);
    draw(QPointF(d, d));
    pen.setColor(lift);
    p.setPen(pen);
    draw(QPointF(-d * 0.8, -d * 0.8));
    pen.setBrush(graded(base));
    p.setPen(pen);
    draw(QPointF(0, 0));
  };
  const auto ring = [&](double radius, double width) {
    p.setBrush(Qt::NoBrush);
    relief([&](const QPointF& o) { p.drawEllipse(c + o, radius, radius); },
           width, ink, Qt::FlatCap);
  };

  // Cancellation waves: five short parallel lines running left from the seal
  // and slipping a little under its edge, as a postmark's would.
  {
    const double start = bounds.left() + 3.0;  // inside the bounds
    const double end = c.x() - r * 0.72;
    const double amplitude = std::max(0.8, r * 0.055);
    const double wavelength = std::max(8.0, r * 0.52);
    QColor wave_ink = ink;
    wave_ink.setAlpha(style.dark ? 185 : 170);
    p.setPen(QPen(wave_ink, std::max(1.2, r * 0.042), Qt::SolidLine,
                  Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    constexpr int kWaves = 5;
    const double spacing = r * 0.20;
    for (int line = 0; line < kWaves && end > start; ++line) {
      const double dy = (line - (kWaves - 1) / 2.0) * spacing;
      QPainterPath wave;
      constexpr int kSteps = 48;
      for (int i = 0; i <= kSteps; ++i) {
        const double x = start + (end - start) * i / kSteps;
        const double y =
            c.y() + dy +
            amplitude * std::sin(2.0 * kPi * (x - start) / wavelength);
        if (i == 0) {
          wave.moveTo(x, y);
        } else {
          wave.lineTo(x, y);
        }
      }
      p.drawPath(wave);
    }
  }

  // Nearly transparent: the seal belongs to the paper.
  QColor wash = ink;
  wash.setAlpha(style.dark ? 16 : 10);
  p.setPen(Qt::NoPen);
  p.setBrush(wash);
  p.drawEllipse(c, r, r);

  // Contours: a strong outer line with a fine one just inside it, and the
  // band's fine inner edge.
  const double outer_w = std::max(1.6, r * 0.065);
  const double fine_w = std::max(0.7, r * 0.018);
  const double outer_r = r - outer_w / 2.0;
  const double fine_r = outer_r - outer_w / 2.0 - std::max(1.6, r * 0.05);
  const double band_inner = detail ? r * 0.64 : r * 0.80;
  ring(outer_r, outer_w);
  ring(fine_r, fine_w);
  ring(band_inner, fine_w);

  if (detail) {
    // The band: the brand along the top, the outcome along the bottom, a
    // small diamond where they meet on either side.
    QFont font = style.font;
    font.setPixelSize(std::max(6, static_cast<int>(std::lround(r * 0.16))));
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    p.setFont(font);
    const QFontMetricsF fm(font);
    const double text_r = (fine_r - fine_w + band_inner + fine_w) / 2.0;
    p.setPen(ink);
    const auto arc = [&](const QString& text, bool bottom) {
      const double total = fm.horizontalAdvance(text);
      const double span = total / text_r * 180.0 / kPi;
      double angle = bottom ? 90.0 + span / 2.0 : -90.0 - span / 2.0;
      for (const QChar ch : text) {
        const double adv = fm.horizontalAdvance(ch);
        const double step = adv / text_r * 180.0 / kPi;
        const double mid = bottom ? angle - step / 2.0 : angle + step / 2.0;
        p.save();
        p.translate(c);
        if (bottom) {
          p.rotate(mid - 90.0);
          p.drawText(QPointF(-adv / 2.0, text_r + fm.capHeight() / 2.0), ch);
        } else {
          p.rotate(mid + 90.0);
          p.drawText(QPointF(-adv / 2.0, -text_r + fm.capHeight() / 2.0), ch);
        }
        p.restore();
        angle += bottom ? -step : step;
      }
    };
    arc(u"GPGFRONTEND"_s, false);
    arc(inscription, true);

    p.setPen(Qt::NoPen);
    p.setBrush(ink);
    const double dia = std::max(1.2, r * 0.045);
    for (const double at_side : {-1.0, 1.0}) {
      const QPointF at = c + QPointF(at_side * text_r, 0.0);
      const QPolygonF diamond{at + QPointF(0, -dia), at + QPointF(dia, 0),
                              at + QPointF(0, dia), at + QPointF(-dia, 0)};
      p.drawPolygon(diamond);
    }

    // Fine radial ticks just inside the band, clear of the symbol.
    QColor tick = ink;
    tick.setAlpha(150);
    p.setPen(QPen(tick, std::max(0.6, r * 0.012), Qt::SolidLine, Qt::FlatCap));
    constexpr int kTicks = 48;
    const double t0 = band_inner - fine_w;
    const double t1 = t0 - std::max(1.5, r * 0.045);
    for (int i = 0; i < kTicks; ++i) {
      const double a = 2.0 * kPi * i / kTicks;
      const QPointF dir(std::cos(a), std::sin(a));
      p.drawLine(c + dir * t0, c + dir * t1);
    }
  }

  // The symbol: bold, optically centred, well clear of the rings.
  const double k = band_inner * (detail ? 0.46 : 0.50);
  const double sw = std::max(2.2, band_inner * 0.17);
  QPainterPath symbol;
  QPointF dot;
  bool has_dot = false;
  if (kind == ReportSealKind::kSUCCESS) {
    symbol.moveTo(c + QPointF(-0.74 * k, 0.04 * k));
    symbol.lineTo(c + QPointF(-0.20 * k, 0.56 * k));
    symbol.lineTo(c + QPointF(0.80 * k, -0.50 * k));
  } else if (kind == ReportSealKind::kFAILURE) {
    symbol.moveTo(c + QPointF(-0.58 * k, -0.58 * k));
    symbol.lineTo(c + QPointF(0.58 * k, 0.58 * k));
    symbol.moveTo(c + QPointF(0.58 * k, -0.58 * k));
    symbol.lineTo(c + QPointF(-0.58 * k, 0.58 * k));
  } else if (information) {
    symbol.moveTo(c + QPointF(0, -0.15 * k));
    symbol.lineTo(c + QPointF(0, 0.75 * k));
    dot = c + QPointF(0, -0.68 * k);
    has_dot = true;
  } else {
    // Warning, cancellation, an unsupported operation: "!".
    symbol.moveTo(c + QPointF(0, -0.78 * k));
    symbol.lineTo(c + QPointF(0, 0.30 * k));
    dot = c + QPointF(0, 0.80 * k);
    has_dot = true;
  }
  p.setBrush(Qt::NoBrush);
  relief([&](const QPointF& o) { p.drawPath(symbol.translated(o)); }, sw,
         symbol_ink, Qt::RoundCap);
  if (has_dot) {
    const double dr = sw * 0.62;
    const auto draw_dot = [&](const QColor& color, const QPointF& o) {
      p.setPen(Qt::NoPen);
      p.setBrush(color);
      p.drawEllipse(dot + o, dr, dr);
    };
    const double d = std::max(0.4, sw * 0.28);
    draw_dot(shade, QPointF(d, d));
    draw_dot(lift, QPointF(-d * 0.8, -d * 0.8));
    draw_dot(symbol_ink, QPointF(0, 0));
  }
  p.restore();
}

}  // namespace GpgFrontend
