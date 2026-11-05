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

#include "crypto/ResultPresenter.h"
#include "shell/result_analyse/GpgOpResultInfo.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::PresentResult;

namespace {

auto Info(int status) -> GpgOpResultInfo {
  GpgOpResultInfo info;
  info.op = GpgOperation::kDECRYPT;
  info.status = status;
  return info;
}

}  // namespace

TEST(ResultPresenterTest, HowItWentIsTheDesktopsReading) {
  EXPECT_EQ(PresentResult(Info(1)).value("severity").toString(), "ok");
  EXPECT_EQ(PresentResult(Info(0)).value("severity").toString(), "warn");
  EXPECT_EQ(PresentResult(Info(-1)).value("severity").toString(), "critical");

  // A success an engine still reported an error under is a warning, as the
  // desktop's Status Panel shows it.
  auto info = Info(1);
  GpgOpStageError error;
  error.scope = GpgOperation::kDECRYPT;
  error.error = {GFErrorCode::kGENERAL, GFErrorOrigin::kRPGP, 0};
  info.stageErrors.append(error);
  const auto report = PresentResult(info);
  EXPECT_EQ(report.value("severity").toString(), "warn");
  EXPECT_EQ(report.value("outcome").toString(), "warning");
  EXPECT_FALSE(report.value("headline").toString().isEmpty());
}

}  // namespace GpgFrontend::Test::Mobile
