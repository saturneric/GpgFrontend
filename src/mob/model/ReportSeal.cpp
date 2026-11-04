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

#include "model/ReportSeal.h"

#include <QPainter>

#include "core/utils/ReportSealPainter.h"

namespace GpgFrontend::Mobile {

namespace {

auto KindOf(const QString& kind) -> ReportSealKind {
  if (kind == QStringLiteral("success")) return ReportSealKind::kSUCCESS;
  if (kind == QStringLiteral("failure")) return ReportSealKind::kFAILURE;
  if (kind == QStringLiteral("cancelled")) return ReportSealKind::kCANCELLED;
  if (kind == QStringLiteral("information")) {
    return ReportSealKind::kINFORMATION;
  }
  return ReportSealKind::kWARNING;
}

}  // namespace

ReportSeal::ReportSeal(QQuickItem* parent) : QQuickPaintedItem(parent) {
  setAntialiasing(true);
  connect(this, &ReportSeal::SignalChanged, this, [this]() { update(); });
}

void ReportSeal::paint(QPainter* painter) {
  // Lettering and ornament need room; below this the seal keeps only its
  // contours and symbol (the desktop's 70 device pixels, in the page's own).
  constexpr double kDetailFrom = 56.0;
  PaintReportSeal(*painter, QRectF(0.0, 0.0, width(), height()), KindOf(kind_),
                  inscription_,
                  {accent_, text_, muted_, dark_, painter->font()},
                  height() >= kDetailFrom);
}

}  // namespace GpgFrontend::Mobile
