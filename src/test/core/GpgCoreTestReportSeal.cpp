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

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include "GpgFrontendTest.h"
#include "shell/present/ReportSealPainter.h"

namespace GpgFrontend::Test {

namespace {

/// How many pixels of @p image are not fully transparent.
auto Inked(const QImage& image) -> int {
  int inked = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(image.pixel(x, y)) != 0) ++inked;
    }
  }
  return inked;
}

auto Paint(ReportSealKind kind, int waves) -> QImage {
  QImage image(56 + waves, 56, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  QPainter p(&image);
  // Without the lettering, which needs a font: the contours, the symbol and
  // the waves only.
  PaintReportSeal(p, QRectF(0, 0, image.width(), image.height()), kind, {},
                  {QColor(0x22C55E), Qt::black, Qt::gray, false, {}}, false);
  p.end();
  return image;
}

}  // namespace

// The seal the desktop and the phone both stamp: within its bounds, and the
// symbol tells the outcomes apart.
TEST(ReportSealTest, EachOutcomeLeavesItsOwnMark) {
  const auto success = Paint(ReportSealKind::kSUCCESS, 0);
  const auto failure = Paint(ReportSealKind::kFAILURE, 0);
  EXPECT_GT(Inked(success), 0);
  EXPECT_NE(success, failure);
  // The painter is left as it was found.
  QImage image(10, 10, QImage::Format_ARGB32_Premultiplied);
  QPainter p(&image);
  const auto before = p.transform();
  PaintReportSeal(p, QRectF(0, 0, 10, 10), ReportSealKind::kWARNING, {},
                  {Qt::red, Qt::black, Qt::gray, true, {}}, false);
  EXPECT_EQ(p.transform(), before);
}

TEST(ReportSealTest, TheWavesRunInFromTheLeft) {
  const auto with = Paint(ReportSealKind::kSUCCESS, 30);
  // The left strip, clear of the seal, holds the waves.
  EXPECT_GT(Inked(with.copy(0, 0, 20, with.height())), 0);
}

TEST(ReportSealTest, ColoursAreMixedByWeight) {
  EXPECT_EQ(BlendReportColor(QColor(100, 0, 0), QColor(0, 100, 0), 1, 1),
            QColor(50, 50, 0));
  EXPECT_EQ(BlendReportColor(Qt::white, Qt::black, 3, 1),
            QColor(191, 191, 191));
}

}  // namespace GpgFrontend::Test
