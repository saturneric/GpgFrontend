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
#include "core/utils/CommonUtils.h"
#include "ui/function/UidPattern.h"

namespace GpgFrontend::Test {

namespace {

using UI::UidPattern;
using UI::UidPatternProblem;

/// The same draw every time.
auto Fixed() -> UI::UidPatternRandom {
  auto counter = std::make_shared<quint64>(0);
  return [counter]() { return (*counter)++; };
}

// Local time, as the expansion reads it; the timestamp is the same instant.
const QDateTime kNow(QDate(2026, 10, 27), QTime(13, 5, 9));

}  // namespace

TEST(UidPatternTest, TheDefaultMakesAnAnonymousUserId) {
  const auto uid = UI::ExpandUidPattern(UidPattern{}, Fixed(), kNow);
  EXPECT_TRUE(uid.name.startsWith(u"anon-"_s));
  EXPECT_EQ(uid.name.size(), 5 + 8);
  EXPECT_TRUE(IsEmailAddress(uid.email)) << uid.email.toStdString();
  // The name and the address share their token.
  EXPECT_TRUE(uid.email.startsWith(uid.name + u"@"_s));
  EXPECT_TRUE(UI::CheckUidPattern(UidPattern{}).Ok());
}

TEST(UidPatternTest, AMadeUpAddressCanNeverReachAnybody) {
  const QStringList reserved{u".test"_s, u".example"_s, u".invalid"_s};
  auto random = UI::SystemUidPatternRandom();
  for (int i = 0; i < 50; ++i) {
    const auto email = UI::ExpandUidPattern(UidPattern{}, random, kNow).email;
    EXPECT_TRUE(
        std::any_of(reserved.cbegin(), reserved.cend(),
                    [&](const QString& tld) { return email.endsWith(tld); }))
        << email.toStdString();
  }
}

TEST(UidPatternTest, ATokenIsTheSameEverywhereAndShorterOnesArePrefixes) {
  UidPattern p;
  p.name = u"{token}"_s;
  p.email = u"{token:16}@x.test"_s;
  p.comment = u"{token:4}"_s;
  const auto uid = UI::ExpandUidPattern(p, UI::SystemUidPatternRandom(), kNow);
  EXPECT_EQ(uid.name.size(), 8);
  EXPECT_TRUE(uid.email.startsWith(uid.name));
  EXPECT_TRUE(uid.name.startsWith(uid.comment));
  EXPECT_EQ(uid.comment.size(), 4);
}

TEST(UidPatternTest, DatesAndBracesAreWrittenOut) {
  UidPattern p;
  p.name = u"test {date} {{x}}"_s;
  p.email = {};
  const auto uid = UI::ExpandUidPattern(p, Fixed(), kNow);
  EXPECT_EQ(uid.name, u"test 20261027 {x}"_s);
  EXPECT_TRUE(uid.email.isEmpty());
}

TEST(UidPatternTest, TheDateCanBeWrittenAnyWay) {
  UidPattern p;
  p.name = u"k-{date:yyyy-MM-dd}-{time}"_s;
  p.email = {};
  p.comment = u"{date:yyMMdd HH.mm} {date:MMM}"_s;
  const auto uid = UI::ExpandUidPattern(p, Fixed(), kNow);
  EXPECT_EQ(uid.name, u"k-2026-10-27-130509"_s);
  // Month names come from the C locale, whatever the language.
  EXPECT_EQ(uid.comment, u"261027 13.05 Oct"_s);
  EXPECT_TRUE(UI::CheckUidPattern(p).Ok());
}

TEST(UidPatternTest, UnixTimeIsTheSameInstant) {
  UidPattern p;
  p.name = u"t{timestamp}"_s;
  p.email = u"{timestamp:ms}@x.test"_s;
  const auto uid = UI::ExpandUidPattern(p, Fixed(), kNow);
  EXPECT_EQ(uid.name, u"t"_s + QString::number(kNow.toSecsSinceEpoch()));
  EXPECT_EQ(uid.email,
            QString::number(kNow.toMSecsSinceEpoch()) + u"@x.test"_s);
  EXPECT_TRUE(UI::CheckUidPattern(p).Ok());
}

TEST(UidPatternTest, ADateFormatIsCheckedLikeAnyText) {
  UidPattern p;
  // Nothing to format is no placeholder.
  p.name = u"{date:}"_s;
  EXPECT_EQ(UI::CheckUidPattern(p).name,
            UidPatternProblem::kUnknownPlaceholder);
  // A format may write a delimiter a user ID cannot hold.
  p.name = u"{date:'('yyyy')'}"_s;
  EXPECT_EQ(UI::CheckUidPattern(p).name, UidPatternProblem::kInvalidText);
  p = {};
  p.name = u"{timestamp:s}"_s;
  EXPECT_EQ(UI::CheckUidPattern(p).name,
            UidPatternProblem::kUnknownPlaceholder);
}

TEST(UidPatternTest, MistakesAreFoundPerPart) {
  UidPattern p;
  p.name = u"{nonsense}"_s;
  p.email = u"{token"_s;
  p.comment = u"(x)"_s;
  const auto check = UI::CheckUidPattern(p);
  EXPECT_EQ(check.name, UidPatternProblem::kUnknownPlaceholder);
  EXPECT_EQ(check.email, UidPatternProblem::kUnbalancedBrace);
  EXPECT_EQ(check.comment, UidPatternProblem::kInvalidText);
  EXPECT_FALSE(check.Ok());

  p = {};
  p.name = u"   "_s;
  EXPECT_EQ(UI::CheckUidPattern(p).name, UidPatternProblem::kNameEmpty);

  p = {};
  p.email = u"{token}"_s;  // no @
  EXPECT_EQ(UI::CheckUidPattern(p).email, UidPatternProblem::kInvalidEmail);

  p = {};
  p.name = u"{token:99}"_s;  // longer than a token can be
  EXPECT_EQ(UI::CheckUidPattern(p).name,
            UidPatternProblem::kUnknownPlaceholder);

  for (const auto problem :
       {UidPatternProblem::kUnknownPlaceholder,
        UidPatternProblem::kUnbalancedBrace, UidPatternProblem::kNameEmpty,
        UidPatternProblem::kInvalidText, UidPatternProblem::kInvalidEmail}) {
    EXPECT_FALSE(UI::DescribeUidPatternProblem(problem).isEmpty());
  }
}

TEST(UidPatternTest, APatternSurvivesItsStoredForm) {
  UidPattern p;
  p.name = u"n-{token}"_s;
  p.email = {};
  p.comment = u"c"_s;
  EXPECT_EQ(UI::UidPatternFromJson(UI::UidPatternToJson(p)), p);
  // Missing parts take their defaults.
  EXPECT_EQ(UI::UidPatternFromJson({}), UidPattern{});
}

}  // namespace GpgFrontend::Test
