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
#include "ui/dialog/SecretPrompt.h"

namespace GpgFrontend::Test {

namespace {

using UI::DefaultSecretPromptTexts;
using UI::kMinSecretLength;
using UI::SecretPromptMode;
using UI::SecretPromptSubject;
using UI::SecretPromptTexts;

auto AppKey(SecretPromptMode mode) -> SecretPromptTexts {
  return DefaultSecretPromptTexts(SecretPromptSubject::kAppKey, mode);
}

auto Package(SecretPromptMode mode) -> SecretPromptTexts {
  return DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage, mode);
}

constexpr std::array kAllModes = {
    SecretPromptMode::kSET,
    SecretPromptMode::kUNLOCK,
    SecretPromptMode::kCHANGE,
};

}  // namespace

TEST(SecretPromptTextsTest, TheApplicationKeyTitlesAreUnchanged) {
  // These three prompts were in front of users before this table existed, and
  // their strings are translated into eight languages. Reproducing them exactly
  // is what keeps those translations attached; asserting them here is what
  // turns a reworded prompt into a failing build rather than a silent
  // regression to English.
  EXPECT_EQ(AppKey(SecretPromptMode::kUNLOCK).window_title,
            "Unlock Application Key");
  EXPECT_EQ(AppKey(SecretPromptMode::kCHANGE).window_title,
            "Change Application PIN");
  EXPECT_EQ(AppKey(SecretPromptMode::kSET).window_title,
            "Set an Application PIN");
}

TEST(SecretPromptTextsTest, TheApplicationKeySubtitleSplitsOnChoosingASecret) {
  const auto set = AppKey(SecretPromptMode::kSET).subtitle;
  const auto change = AppKey(SecretPromptMode::kCHANGE).subtitle;
  const auto unlock = AppKey(SecretPromptMode::kUNLOCK).subtitle;

  EXPECT_EQ(set, change);
  EXPECT_NE(set, unlock);
  EXPECT_FALSE(set.isEmpty());
  EXPECT_FALSE(unlock.isEmpty());
}

TEST(SecretPromptTextsTest, OnlyPromptsThatChooseASecretWarn) {
  // The irreversibility warning answers "what happens if I forget this", which
  // is a question only someone choosing a secret still has. Someone unlocking
  // made that choice long ago and does not need to be alarmed again.
  EXPECT_FALSE(AppKey(SecretPromptMode::kSET).warning.isEmpty());
  EXPECT_FALSE(AppKey(SecretPromptMode::kCHANGE).warning.isEmpty());
  EXPECT_TRUE(AppKey(SecretPromptMode::kUNLOCK).warning.isEmpty());
}

TEST(SecretPromptTextsTest, NoPromptNamesAFileByItself) {
  // What a prompt is about is a list of rows a caller builds from
  // BuildProfilePackageRows(), never a string this table invents. Two
  // descriptions of one file are two descriptions that can disagree, and the
  // application-key prompts are about no file at all.
  for (const auto mode : kAllModes) {
    EXPECT_TRUE(AppKey(mode).context_rows.isEmpty());
    EXPECT_TRUE(Package(mode).context_rows.isEmpty());
  }
}

TEST(SecretPromptTextsTest, NoPromptSaysTheSameThingTwice) {
  // The reason this prompt was rebuilt: it once said "nothing can be read until
  // the passphrase opens it" in the subtitle, again in the hint, and a third
  // time in the header note, and three ways of saying one thing is how the
  // sentence that matters stops being read. Each row that survives has to say
  // something the others do not.
  for (const auto mode : kAllModes) {
    const auto texts = Package(mode);
    for (const auto& other : {texts.hint, texts.warning}) {
      if (other.isEmpty()) continue;
      EXPECT_NE(texts.subtitle, other);
    }
  }
}

TEST(SecretPromptTextsTest, EveryPromptLineIsShortEnoughToRead) {
  // These are single wrapped lines in a 470 pixel dialog, above the field the
  // user came here to type in. A sentence that wraps three times is one nobody
  // finishes.
  for (const auto subject :
       {SecretPromptSubject::kAppKey, SecretPromptSubject::kProfilePackage}) {
    for (const auto mode : kAllModes) {
      const auto texts = DefaultSecretPromptTexts(subject, mode);
      EXPECT_LT(texts.subtitle.size(), 120) << texts.subtitle.toStdString();
      EXPECT_LT(texts.hint.size(), 120) << texts.hint.toStdString();
      EXPECT_LT(texts.warning.size(), 140) << texts.warning.toStdString();
    }
  }
}

TEST(SecretPromptTextsTest, EachSubjectKeepsItsOwnWordForTheSecret) {
  // The application PIN and a profile file's passphrase are different secrets
  // protecting different things. A package prompt that called its secret a PIN
  // would suggest, falsely, that the application PIN belongs in the field.
  for (const auto mode : kAllModes) {
    const auto app = AppKey(mode);
    for (const auto& text : {app.current_label, app.new_label, app.reveal_label,
                             app.too_short_message}) {
      EXPECT_FALSE(text.contains("passphrase", Qt::CaseInsensitive))
          << text.toStdString();
    }

    const auto package = Package(mode);
    for (const auto& text :
         {package.current_label, package.new_label, package.reveal_label,
          package.too_short_message, package.window_title, package.subtitle}) {
      EXPECT_FALSE(text.contains("PIN", Qt::CaseSensitive))
          << text.toStdString();
    }
  }
}

TEST(SecretPromptTextsTest, EveryStringAPromptWillRenderIsPopulated) {
  // A missing string renders as a blank label: invisible in review, obvious to
  // a user. Every field the dialog reads unconditionally has to be filled.
  for (const auto subject :
       {SecretPromptSubject::kAppKey, SecretPromptSubject::kProfilePackage}) {
    for (const auto mode : kAllModes) {
      const auto texts = DefaultSecretPromptTexts(subject, mode);

      EXPECT_FALSE(texts.window_title.isEmpty());
      EXPECT_FALSE(texts.subtitle.isEmpty());
      EXPECT_FALSE(texts.accept_button.isEmpty());
      EXPECT_FALSE(texts.reveal_label.isEmpty());
      EXPECT_FALSE(texts.strength_caption.isEmpty());
      EXPECT_FALSE(texts.mismatch_message.isEmpty());
      EXPECT_FALSE(texts.too_short_message.isEmpty());

      // The hint is the one line a prompt is allowed to leave out: the row it
      // fills is always there for error messages, and a prompt with nothing
      // left to add should add nothing. Every prompt that chooses a secret
      // still has something to say about the choice.
      if (mode != SecretPromptMode::kUNLOCK) {
        EXPECT_FALSE(texts.hint.isEmpty());
      }

      if (mode != SecretPromptMode::kSET) {
        EXPECT_FALSE(texts.current_label.isEmpty());
      }
      if (mode != SecretPromptMode::kUNLOCK) {
        EXPECT_FALSE(texts.new_label.isEmpty());
        EXPECT_FALSE(texts.confirm_label.isEmpty());
      }
    }
  }
}

TEST(SecretPromptTextsTest, TheFloorAppearsInItsOwnMessage) {
  // The message carries %1 rather than a baked-in number, so lowering the floor
  // for one caller cannot leave the sentence claiming the old one.
  for (const auto subject :
       {SecretPromptSubject::kAppKey, SecretPromptSubject::kProfilePackage}) {
    const auto texts =
        DefaultSecretPromptTexts(subject, SecretPromptMode::kSET);
    EXPECT_TRUE(texts.too_short_message.contains("%1"));
    EXPECT_TRUE(texts.too_short_message.arg(12).contains("12"));
  }
}

TEST(SecretPromptTextsTest, TheFloorIsEightUnlessACallerLowersIt) {
  // Every default carries the same floor, so the write-back prompt's lower one
  // reads as a caller's deliberate choice rather than a quiet special case.
  for (const auto subject :
       {SecretPromptSubject::kAppKey, SecretPromptSubject::kProfilePackage}) {
    for (const auto mode : kAllModes) {
      EXPECT_EQ(DefaultSecretPromptTexts(subject, mode).min_length,
                kMinSecretLength);
    }
  }
}

}  // namespace GpgFrontend::Test
