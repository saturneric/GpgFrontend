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

#include <QClipboard>
#include <QGuiApplication>
#include <QSignalSpy>
#include <atomic>

#include "MobileTest.h"
#include "crypto/OutputActions.h"
#include "plat/common/PlatformClipboard.h"
#include "plat/common/PlatformPrivacy.h"
#include "plat/common/PlatformShare.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::OutputActions;

namespace {

/// Everything one share request reported.
struct Completions {
  std::atomic<int> count{0};
  ShareResult last;
};

auto Begin(Completions* into, int never_shown_ms = ShareSession::kNeverShownMs)
    -> int {
  int id = 0;
  RunOnMainThread([&id, into, never_shown_ms]() {
    id = ShareSession::Instance().Begin(
        [into](const ShareResult& result) {
          into->last = result;
          ++into->count;
        },
        never_shown_ms);
  });
  return id;
}

void SetState(Qt::ApplicationState state) {
  RunOnMainThread([state]() {
    ShareSession::Instance().OnApplicationStateChanged(state);
  });
}

auto OpenCount() -> int {
  int count = 0;
  RunOnMainThread([&count]() { count = ShareSession::Instance().OpenCount(); });
  return count;
}

/// Longer than the reconcile grace, for "nothing more happens".
void PastTheGrace() {
  std::this_thread::sleep_for(
      std::chrono::milliseconds(ShareSession::kReconcileGraceMs + 300));
}

auto ClipboardText() -> QString {
  QString text;
  RunOnMainThread(
      [&text]() { text = QGuiApplication::clipboard()->text(); });
  return text;
}

}  // namespace

class ShareSessionTest : public ::testing::Test {
 protected:
  void TearDown() override {
    SetState(Qt::ApplicationActive);
    PastTheGrace();
    EXPECT_EQ(OpenCount(), 0);
  }
};

TEST_F(ShareSessionTest, ATargetChosenBeforeTheReturnIsHandedOff) {
  Completions done;
  const auto id = Begin(&done);
  SetState(Qt::ApplicationInactive);
  SetState(Qt::ApplicationSuspended);
  ShareSession::Instance().TargetChosen(id);
  SetState(Qt::ApplicationActive);

  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  EXPECT_EQ(done.last.outcome, ShareOutcome::kHandedOff);
  EXPECT_TRUE(done.last.target_chosen);
}

TEST_F(ShareSessionTest, AReturnWithNoTargetIsDismissed) {
  Completions done;
  Begin(&done);
  SetState(Qt::ApplicationSuspended);
  SetState(Qt::ApplicationActive);

  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  EXPECT_EQ(done.last.outcome, ShareOutcome::kDismissed);
  EXPECT_FALSE(done.last.target_chosen);
}

TEST_F(ShareSessionTest, ATargetReportedJustAfterTheReturnStillCounts) {
  Completions done;
  const auto id = Begin(&done);
  SetState(Qt::ApplicationSuspended);
  SetState(Qt::ApplicationActive);
  ShareSession::Instance().TargetChosen(id);

  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  EXPECT_EQ(done.last.outcome, ShareOutcome::kHandedOff);
}

TEST_F(ShareSessionTest, ALateEventChangesNothing) {
  Completions done;
  const auto id = Begin(&done);
  SetState(Qt::ApplicationSuspended);
  SetState(Qt::ApplicationActive);
  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  ASSERT_EQ(done.last.outcome, ShareOutcome::kDismissed);

  ShareSession::Instance().TargetChosen(id);
  ShareSession::Instance().Failed(id, "late");
  SetState(Qt::ApplicationSuspended);
  SetState(Qt::ApplicationActive);
  PastTheGrace();
  EXPECT_EQ(done.count, 1);
  EXPECT_EQ(done.last.outcome, ShareOutcome::kDismissed);
}

TEST_F(ShareSessionTest, NothingIsConcludedBeforeTheApplicationLeaves) {
  Completions done;
  const auto id = Begin(&done);
  // Back to the foreground without ever leaving: the sheet is not up yet.
  SetState(Qt::ApplicationActive);
  PastTheGrace();
  EXPECT_EQ(done.count, 0);

  ShareSession::Instance().Failed(id, "no share sheet");
  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  EXPECT_EQ(done.last.outcome, ShareOutcome::kFailed);
  EXPECT_EQ(done.last.detail, "no share sheet");
}

TEST_F(ShareSessionTest, ASheetThatNeverAppearedFails) {
  // Nothing reported, and the application never left: the sheet was never
  // shown. The request ends instead of staying open for the process's life.
  Completions done;
  Begin(&done, 100);
  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  EXPECT_EQ(done.last.outcome, ShareOutcome::kFailed);

  // One that was shown is not ended by that clock.
  Completions shown;
  Begin(&shown, 100);
  SetState(Qt::ApplicationInactive);
  EXPECT_FALSE(WaitFor([&shown]() { return shown.count > 0; }, 500));
  SetState(Qt::ApplicationActive);
  ASSERT_TRUE(WaitFor([&shown]() { return shown.count > 0; }));
  EXPECT_EQ(shown.last.outcome, ShareOutcome::kDismissed);
}

TEST_F(ShareSessionTest, LeavingAgainWithinTheGraceWaitsForTheNextReturn) {
  Completions done;
  const auto id = Begin(&done);
  SetState(Qt::ApplicationSuspended);
  SetState(Qt::ApplicationActive);
  // Away again before the grace ran out: the first return's check must not
  // conclude anything.
  SetState(Qt::ApplicationSuspended);
  PastTheGrace();
  EXPECT_EQ(done.count, 0);

  ShareSession::Instance().TargetChosen(id);
  SetState(Qt::ApplicationActive);
  ASSERT_TRUE(WaitFor([&done]() { return done.count > 0; }));
  EXPECT_EQ(done.last.outcome, ShareOutcome::kHandedOff);
  PastTheGrace();
  EXPECT_EQ(done.count, 1);
}

// Linux has no share sheet: the page is told so, and offers Copy.
TEST(PlatformStubTest, SharingIsReportedUnsupported) {
  EXPECT_FALSE(CanShare());

  std::unique_ptr<OutputActions> actions;
  std::unique_ptr<QSignalSpy> shared;
  RunOnMainThread([&]() {
    actions = std::make_unique<OutputActions>();
    shared = std::make_unique<QSignalSpy>(actions.get(),
                                          &OutputActions::SignalShared);
    EXPECT_FALSE(actions->CanShare());
    actions->share("ciphertext");
  });
  ASSERT_TRUE(WaitFor([&shared]() {
    int count = 0;
    RunOnMainThread([&]() { count = static_cast<int>(shared->count()); });
    return count > 0;
  }));
  RunOnMainThread([&]() {
    EXPECT_EQ(shared->takeFirst().at(0).toString(), "unsupported");
    // Nothing was put on the clipboard instead.
    shared.reset();
    actions.reset();
  });
  EXPECT_NE(ClipboardText(), "ciphertext");
}

TEST(PlatformStubTest, NoWindowIsSecuredWhereThePlatformCannot) {
  EXPECT_FALSE(CanSecureWindow());
  // Harmless, with or without a window.
  RunOnMainThread([]() {
    SecureWindowScope outer(PrimaryWindow());
    SecureWindowScope inner(nullptr);
  });
}

TEST(SensitiveClipboardTest, ExpiresWhenItsTimeIsUp) {
  // Read back on the interface thread before the timer can run there.
  QString copied;
  RunOnMainThread([&copied]() {
    CopySensitiveText("decrypted", std::chrono::seconds(1));
    copied = QGuiApplication::clipboard()->text();
  });
  EXPECT_EQ(copied, "decrypted");
  ASSERT_TRUE(WaitFor([]() { return ClipboardText().isEmpty(); }));
}

TEST(SensitiveClipboardTest, NeverClearsSomethingNewer) {
  RunOnMainThread([]() {
    CopySensitiveText("decrypted", std::chrono::seconds(60));
    // The person copies something else meanwhile.
    QGuiApplication::clipboard()->setText("a shopping list");
    ExpireSensitiveCopy(true);
  });
  EXPECT_EQ(ClipboardText(), "a shopping list");
}

TEST(SensitiveClipboardTest, NotBeforeItsTimeUnlessQuitting) {
  RunOnMainThread([]() {
    CopySensitiveText("decrypted", std::chrono::seconds(60));
    ExpireSensitiveCopy();
  });
  EXPECT_EQ(ClipboardText(), "decrypted");

  RunOnMainThread([]() { ExpireSensitiveCopy(true); });
  EXPECT_TRUE(ClipboardText().isEmpty());
}

TEST(SensitiveClipboardTest, ACopyThatIsNotSensitiveStays) {
  RunOnMainThread([]() {
    OutputActions actions;
    actions.copy("-----BEGIN PGP MESSAGE-----", false);
    ExpireSensitiveCopy(true);
  });
  EXPECT_EQ(ClipboardText(), "-----BEGIN PGP MESSAGE-----");
}

}  // namespace GpgFrontend::Test::Mobile
