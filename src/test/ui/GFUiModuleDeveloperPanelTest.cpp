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
#include "sdk/GFSDKTypes.h"
#include "ui/dialog/controller/ModuleDeveloperPanel.h"

/**
 * @file GFUiModuleDeveloperPanelTest.cpp
 * @brief What the developer tab parses and prints, without the widgets.
 */

namespace GpgFrontend::Test {

TEST(ModuleDeveloperPanelTest, EventParamsAreOneKeyValuePerLine) {
  QString error;
  const auto p =
      UI::ParseEventParams("channel=0\n\n  data = a=b=c\r\nempty=\n", &error);
  ASSERT_TRUE(p.has_value()) << error.toStdString();
  EXPECT_EQ(p->size(), 3);
  EXPECT_EQ(p->value("channel").ConvertToQString(), "0");
  EXPECT_EQ(p->value("data").ConvertToQString(), " a=b=c")
      << "the first '=' splits; the value is kept as written";
  EXPECT_TRUE(p->value("empty").Empty());

  EXPECT_TRUE(UI::ParseEventParams({}, &error).has_value());
}

TEST(ModuleDeveloperPanelTest, AMalformedParameterLineIsRefusedByNumber) {
  QString error;
  EXPECT_FALSE(UI::ParseEventParams("a=1\nnot a pair", &error).has_value());
  EXPECT_TRUE(error.startsWith("line 2")) << error.toStdString();

  EXPECT_FALSE(UI::ParseEventParams("=1", &error).has_value());
  EXPECT_TRUE(error.contains("empty")) << error.toStdString();

  EXPECT_FALSE(UI::ParseEventParams("a=1\na=2", &error).has_value());
  EXPECT_TRUE(error.contains("twice")) << error.toStdString();
}

TEST(ModuleDeveloperPanelTest, CommandArgsAreOneJsonObject) {
  QString error;
  const auto args = UI::ParseCommandArgs(R"({"type":"text","n":2})", &error);
  ASSERT_TRUE(args.has_value()) << error.toStdString();
  EXPECT_EQ(args->value(QStringLiteral("type")).toString(), "text");
  EXPECT_EQ(args->value(QStringLiteral("n")).toInteger(), 2);

  const auto none = UI::ParseCommandArgs("  ", &error);
  ASSERT_TRUE(none.has_value());
  EXPECT_TRUE(none->isEmpty());

  EXPECT_FALSE(UI::ParseCommandArgs("[1,2]", &error).has_value());
  EXPECT_FALSE(UI::ParseCommandArgs("{\"a\":", &error).has_value());
  EXPECT_FALSE(error.isEmpty());
}

TEST(ModuleDeveloperPanelTest, AnAnswersPayloadIsShownBySizeNotContent) {
  const Module::Event::Params params = {
      {"ret", GFBuffer(QString("-1"))},
      {"err", GFBuffer(QString("the module was deactivated"))},
      {"data", GFBuffer(QString("my decrypted secret"))},
  };
  const auto line = UI::DescribeEventAnswer("com.example.m", params);
  EXPECT_TRUE(line.startsWith("com.example.m: ret=-1")) << line.toStdString();
  EXPECT_TRUE(line.contains("the module was deactivated"));
  EXPECT_TRUE(line.contains("data(19 bytes)")) << line.toStdString();
  EXPECT_FALSE(line.contains("secret")) << "an answer may carry plaintext";

  EXPECT_TRUE(UI::DescribeEventAnswer({}, params).startsWith("host: "))
      << "an answer the Host made because no module could";
}

TEST(ModuleDeveloperPanelTest, EveryCommandStatusHasAName) {
  for (const int status :
       {GF_CMD_OK, GF_CMD_E_UNKNOWN, GF_CMD_E_DENIED, GF_CMD_E_BAD_ARGS,
        GF_CMD_E_DISABLED, GF_CMD_E_UNAVAILABLE, GF_CMD_E_CANCELLED,
        GF_CMD_E_FAILED}) {
    EXPECT_FALSE(UI::DescribeCommandStatus(status).startsWith("status"))
        << status;
  }
  EXPECT_EQ(UI::DescribeCommandStatus(42), "status 42");
}

}  // namespace GpgFrontend::Test
