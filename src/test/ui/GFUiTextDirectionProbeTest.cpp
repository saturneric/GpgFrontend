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

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextOption>

#include "GpgFrontendTest.h"

/**
 * Pins the Qt text layout behaviour the per-line direction feature is built on.
 *
 * None of this is our code. It is here because the design rests on three facts
 * about QTextEngine and the two document layouts that are not spelled out in
 * the documentation, and a Qt upgrade that changed any of them would otherwise
 * break the editor silently, in a way no other test would notice.
 */
namespace GpgFrontend::Test {

namespace {

const auto* const kArabic = "مرحبا";

auto MixedRightToLeftText() -> QString {
  // Arabic first, so the first strong character asks for a right-to-left base,
  // then Latin, so the two bases actually order the line differently. A line of
  // pure Arabic looks the same under either base and would prove nothing.
  return QString::fromUtf8(kArabic) + QStringLiteral(" abc");
}

}  // namespace

TEST(TextDirectionProbeTest, AnUnsetDirectionReadsAsAutomatic) {
  // Qt::LayoutDirectionAuto is the value that hands per-paragraph resolution
  // back to QTextEngine, so the whole feature depends on it being what an
  // untouched option and an untouched block format already carry.
  EXPECT_EQ(QTextOption{}.textDirection(), Qt::LayoutDirectionAuto);

  QTextDocument doc;
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::LayoutDirectionAuto);

  // The one in real doubt: QTextFormat::layoutDirection() goes through
  // intProperty(), and an absent property could just as well default to
  // Qt::LeftToRight (0). If it does, QTextBlock::textDirection() short-circuits
  // on it and never reaches its own first-strong scan.
  EXPECT_EQ(QTextBlockFormat{}.layoutDirection(), Qt::LayoutDirectionAuto);
}

TEST(TextDirectionProbeTest, EachBlockResolvesItsOwnDirectionUnderAutomatic) {
  QTextDocument doc;
  doc.setPlainText(QStringLiteral("hello\n") + QString::fromUtf8(kArabic));
  ASSERT_EQ(doc.blockCount(), 2);

  auto option = doc.defaultTextOption();
  option.setTextDirection(Qt::LayoutDirectionAuto);
  doc.setDefaultTextOption(option);

  EXPECT_EQ(doc.findBlockByNumber(0).textDirection(), Qt::LeftToRight);
  EXPECT_EQ(doc.findBlockByNumber(1).textDirection(), Qt::RightToLeft);
}

TEST(TextDirectionProbeTest, PinningTheDocumentDirectionSuppressesPerBlock) {
  // The bug being fixed, stated as a test: a document-wide direction is what
  // stops each line from deciding for itself.
  QTextDocument doc;
  doc.setPlainText(QStringLiteral("hello\n") + QString::fromUtf8(kArabic));

  auto option = doc.defaultTextOption();
  option.setTextDirection(Qt::LeftToRight);
  doc.setDefaultTextOption(option);

  EXPECT_EQ(doc.findBlockByNumber(1).textDirection(), Qt::LeftToRight);
}

TEST(TextDirectionProbeTest, APlainTextLayoutPropagatesOnlyTheDocumentDefault) {
  // The decisive one. QPlainTextDocumentLayout::layoutBlock() is private and
  // non-virtual, so if it hands each block nothing but the document's default
  // option then QTextBlockFormat direction and alignment are dead in a
  // QPlainTextEdit, and per-line alignment cannot be done there at all.
  QTextDocument doc;
  doc.setDocumentLayout(new QPlainTextDocumentLayout(&doc));
  doc.setPlainText(QStringLiteral("hello\n") + QString::fromUtf8(kArabic));

  auto option = doc.defaultTextOption();
  option.setTextDirection(Qt::LayoutDirectionAuto);
  option.setAlignment(Qt::AlignLeft);
  doc.setDefaultTextOption(option);

  auto block = doc.findBlockByNumber(1);
  ASSERT_TRUE(block.isValid());

  QTextCursor cursor(block);
  auto block_format = cursor.blockFormat();
  block_format.setLayoutDirection(Qt::RightToLeft);
  block_format.setAlignment(Qt::AlignRight);
  cursor.setBlockFormat(block_format);

  ASSERT_EQ(block.blockFormat().layoutDirection(), Qt::RightToLeft);

  auto* layout = qobject_cast<QPlainTextDocumentLayout*>(doc.documentLayout());
  ASSERT_NE(layout, nullptr);
  layout->ensureBlockLayout(block);
  ASSERT_GT(block.layout()->lineCount(), 0);

  const auto laid_out = block.layout()->textOption();

  // Whichever way these land, the answer is recorded rather than assumed. Both
  // matching the document default means block formats are ignored here.
  EXPECT_EQ(laid_out.textDirection(), Qt::LayoutDirectionAuto)
      << "block format direction reached the plain text layout";
  EXPECT_EQ(laid_out.alignment(), Qt::AlignLeft)
      << "block format alignment reached the plain text layout";
}

TEST(TextDirectionProbeTest, AutomaticReordersALineTheEngineResolvesItself) {
  // The last hop: the plain text layout passes Qt::LayoutDirectionAuto through
  // verbatim, and QTextEngine::isRightToLeft() then falls back to the first
  // strong character of that one block's own string. Needs glyph metrics, so it
  // has to run where a font database is usable.
  qreal x_pinned_ltr = 0;
  qreal x_automatic = 0;
  qreal natural_width = 0;

  RunOnMainThread([&]() {
    const auto text = MixedRightToLeftText();

    auto measure = [&text](Qt::LayoutDirection dir, qreal* width) {
      QTextLayout layout(text);
      QTextOption option;
      option.setTextDirection(dir);
      layout.setTextOption(option);

      layout.beginLayout();
      auto line = layout.createLine();
      line.setLineWidth(1000);
      layout.endLayout();

      if (width != nullptr) *width = line.naturalTextWidth();
      return line.cursorToX(0);
    };

    x_pinned_ltr = measure(Qt::LeftToRight, &natural_width);
    x_automatic = measure(Qt::LayoutDirectionAuto, nullptr);
  });

  // With no usable font every advance is zero and both answers collapse to 0,
  // which would pass the comparison below without having measured anything.
  if (natural_width <= 0.0) {
    GTEST_SKIP() << "no usable font in this environment";
  }

  EXPECT_NE(x_pinned_ltr, x_automatic)
      << "automatic did not re-resolve the base direction from the content";
}

}  // namespace GpgFrontend::Test
