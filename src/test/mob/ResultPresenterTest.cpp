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

// A decryption no key here could open: why, as a kind the pages word, and
// none of the message's own facts, which nobody read; the recipients its
// outer packets name stay.
TEST(ResultPresenterTest, AFailedDecryptionSaysWhyAndNoMessageFacts) {
  auto info = Info(-1);
  GpgRecipientInfo recipient;
  recipient.keyId = "0123456789ABCDEF";
  info.recipients.append(recipient);
  GpgOpStageError error;
  error.scope = GpgOperation::kDECRYPT;
  error.error = {GFErrorCode::kNO_SECRET_KEY, GFErrorOrigin::kRPGP, 0};
  error.codeText = "Secret key not found";
  info.stageErrors.append(error);

  const auto failed = PresentResult(info);
  EXPECT_EQ(failed.value("reason").toString(), "no_secret_key");
  // The engine's words are kept for the details.
  EXPECT_TRUE(failed.value("errors").toStringList().contains(
      "Secret key not found"));
  QStringList titles;
  for (const auto& card : failed.value("cards").toList()) {
    titles << card.toMap().value("title").toString();
  }
  EXPECT_FALSE(titles.contains("Message Metadata")) << titles.join(", ").toStdString();
  EXPECT_TRUE(titles.contains("Decryption Recipient")) << titles.join(", ").toStdString();

  // Decrypted: its facts are known, and shown.
  auto ok = Info(1);
  ok.recipients.append(recipient);
  ok.messageIntegrityProtected = true;
  titles.clear();
  const auto done = PresentResult(ok);
  EXPECT_TRUE(done.value("reason").toString().isEmpty());
  for (const auto& card : done.value("cards").toList()) {
    titles << card.toMap().value("title").toString();
  }
  EXPECT_TRUE(titles.contains("Message Metadata")) << titles.join(", ").toStdString();
}

}  // namespace GpgFrontend::Test::Mobile
