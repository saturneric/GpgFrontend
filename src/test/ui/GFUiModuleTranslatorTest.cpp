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
#include "core/module/ModuleManager.h"
#include "ui/UIModuleManager.h"

namespace GpgFrontend::Test {

// Module translators are reinstalled on every interface language switch and
// again on every restart-loop pass. QTranslator::load(const uchar*, int) does
// not copy its input, so a round that forgets its predecessors leaves them
// installed on qApp reading QM bytes it has just released -- which is what took
// the process down mid-restart after a language change in the wizard.
//
// These run the whole exercise on the main thread: installTranslator() and
// parenting a QTranslator to qApp are not worker-thread business.

namespace {

// A string the key server sync module translates in its own "GTrC" context, so
// the answer can only come from a module translator, never from the
// application's own.
constexpr auto kProbeContext = "GTrC";
constexpr auto kProbeSource = "Public Key Upload Successful";

struct Round {
  QContainer<QPointer<QTranslator>> installed;
  QString probe;
};

// Modules are loaded and registered on a worker task that races the test
// suite, and a module only hands over its translator data reader while
// registering. Without this the whole exercise runs against an empty set.
auto WaitForModules() -> bool {
  return WaitFor(
      []() -> bool {
        return Module::ModuleManager::GetInstance().IsAllModulesRegistered();
      },
      10000);
}

auto Reinstall() -> Round {
  Round round;
  RunOnMainThread([&round]() {
    auto& manager = UI::UIModuleManager::GetInstance();
    manager.RegisterAllModuleTranslators();
    round.installed = manager.InstalledTranslators();
    round.probe = QCoreApplication::translate(kProbeContext, kProbeSource);
  });
  return round;
}

void SetLocale(const QLocale& locale) {
  RunOnMainThread([&locale]() { QLocale::setDefault(locale); });
}

/**
 * @brief Switch the process to a locale the modules actually ship QM data for,
 * and put the previous one back afterwards.
 *
 * The default test locale has no module translations, so a reinstall under it
 * installs nothing and there is no lifetime to test.
 */
class ScopedTranslatedLocale {
 public:
  ScopedTranslatedLocale() { SetLocale(QLocale("zh_CN")); }

  ~ScopedTranslatedLocale() {
    SetLocale(original_);
    Reinstall();
  }

  ScopedTranslatedLocale(const ScopedTranslatedLocale&) = delete;
  auto operator=(const ScopedTranslatedLocale&)
      -> ScopedTranslatedLocale& = delete;

 private:
  QLocale original_;
};

}  // namespace

TEST(ModuleTranslatorTest, ReinstallReleasesPreviousTranslators) {
  ASSERT_TRUE(WaitForModules());
  const ScopedTranslatedLocale locale;

  const auto first = Reinstall();
  if (first.installed.isEmpty()) {
    GTEST_SKIP() << "no module registered a translator data reader here";
  }
  for (const auto& translator : first.installed) {
    ASSERT_FALSE(translator.isNull());
  }

  const auto second = Reinstall();

  // Gone, not merely forgotten: a translator that is only dropped from the
  // manager's list stays installed on qApp and keeps reading the QM bytes this
  // very call released.
  for (const auto& translator : first.installed) {
    EXPECT_TRUE(translator.isNull());
  }

  EXPECT_EQ(second.installed.size(), first.installed.size());
  for (const auto& translator : second.installed) {
    EXPECT_FALSE(translator.isNull());
  }
}

TEST(ModuleTranslatorTest, RepeatedReinstallKeepsTranslationsResolving) {
  ASSERT_TRUE(WaitForModules());
  const ScopedTranslatedLocale locale;

  const auto first = Reinstall();
  if (first.installed.isEmpty()) {
    GTEST_SKIP() << "no module registered a translator data reader here";
  }

  // Otherwise the probe below proves nothing: an untranslated string comes back
  // unchanged whether a translator is consulted or not.
  ASSERT_NE(first.probe, QString(kProbeSource));

  // Three more switches, the way a user clicking through the wizard's language
  // box produces them. The answer must not drift or come back garbled.
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(Reinstall().probe, first.probe);
  }
}

}  // namespace GpgFrontend::Test
