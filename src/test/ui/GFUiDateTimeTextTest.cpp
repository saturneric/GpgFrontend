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

#include "GpgFrontendTest.h"
#include "ui/function/DateTimeText.h"

namespace GpgFrontend::Test {

using UI::LongDateTimeText;
using UI::ShortDateTimeText;
using UI::StripTimeZoneFields;

TEST(DateTimeTextTest, ZoneFieldsAreStrippedQuotedTextKept) {
  EXPECT_EQ(StripTimeZoneFields(u"dddd, MMMM d, yyyy h:mm:ss AP tttt"_s),
            u"dddd, MMMM d, yyyy h:mm:ss AP"_s);
  EXPECT_EQ(StripTimeZoneFields(u"yyyy-MM-dd HH:mm t"_s),
            u"yyyy-MM-dd HH:mm"_s);
  // A "t" inside quoted text is the locale's own wording, not a field.
  EXPECT_EQ(StripTimeZoneFields(u"d 'at' HH:mm"_s), u"d 'at' HH:mm"_s);
  EXPECT_EQ(StripTimeZoneFields(u"HH:mm:ss, tttt"_s), u"HH:mm:ss"_s);
}

TEST(DateTimeTextTest, LongTextHasNoZoneName) {
  const QLocale locale(QLocale::English, QLocale::UnitedStates);
  const QDateTime at(QDate(2025, 1, 2), QTime(12, 0), QTimeZone::UTC);
  const auto text = LongDateTimeText(at, locale);
  EXPECT_TRUE(text.startsWith(u"Thursday, January 2, 2025"_s))
      << text.toStdString();
  EXPECT_FALSE(text.contains(u"UTC"_s)) << text.toStdString();
  EXPECT_FALSE(text.contains(u"Time"_s)) << text.toStdString();
}

TEST(DateTimeTextTest, ShortTextIsTheLocaleShortFormat) {
  const QLocale locale(QLocale::English, QLocale::UnitedStates);
  const QDateTime at(QDate(2025, 1, 2), QTime(12, 0), QTimeZone::UTC);
  EXPECT_EQ(ShortDateTimeText(at, locale),
            locale.toString(at, QLocale::ShortFormat));
  EXPECT_LT(ShortDateTimeText(at, locale).size(),
            locale.toString(at, QLocale::LongFormat).size());
}

}  // namespace GpgFrontend::Test
