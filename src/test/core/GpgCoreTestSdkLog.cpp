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

#include <QLoggingCategory>
#include <QThread>

#include "core/GFCoreLog.h"
#include "core/SdkTestContext.h"
#include "core/module/ModuleLogCategory.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKLog.h"

namespace GpgFrontend::Test {

namespace {

/// One granted context for this file, minted the way the module loader
/// mints one. Never under a real module's id: minting a different grant for
/// an id revokes the live one, which used to take the bundled modules' grants
/// away from every test that ran after this file. Process-lifetime on purpose:
/// a grant is retired, never freed, so that a stale caller is refused rather
/// than following a dangling pointer.
auto Ctx() -> GFSDKContext* {
  static SdkTestContext context("com.example.logtest.email");
  return context.get();
}

/// A second module, because "two modules are distinguishable" needs two.
auto OtherCtx() -> GFSDKContext* {
  static SdkTestContext context("com.example.logtest.key_server_sync");
  return context.get();
}

}  // namespace

namespace {

constexpr auto kEmailId = "com.example.logtest.email";
constexpr auto kOtherId = "com.example.logtest.key_server_sync";

/// What one intercepted message carried.
struct Captured {
  QString category;
  QString file;
  int line = 0;
  QString function;
  QString message;
  QtMsgType type = QtDebugMsg;
};

QList<Captured>* g_captured = nullptr;
QtMessageHandler g_previous = nullptr;

void Capture(QtMsgType type, const QMessageLogContext& context,
             const QString& message) {
  if (g_captured != nullptr) {
    g_captured->append(Captured{
        QString::fromUtf8(context.category == nullptr ? "" : context.category),
        QString::fromUtf8(context.file == nullptr ? "" : context.file),
        context.line,
        QString::fromUtf8(context.function == nullptr ? "" : context.function),
        message, type});
  }
}

/// Intercepts Qt's message pipeline for the duration of one test.
///
/// The filter rules are widened and restored too: these tests assert on what
/// the SDK emits, and inheriting whatever level the harness happened to be run
/// at would make them pass or fail for a reason that has nothing to do with the
/// code under test.
class LogCaptureFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    previous_rules_ = QLoggingCategory::defaultCategory()->isDebugEnabled();
    QLoggingCategory::setFilterRules("module.*=true\n");
    captured_.clear();
    g_captured = &captured_;
    g_previous = qInstallMessageHandler(&Capture);
  }

  void TearDown() override {
    qInstallMessageHandler(g_previous);
    g_captured = nullptr;
    QLoggingCategory::setFilterRules({});
    Q_UNUSED(previous_rules_);
  }

  [[nodiscard]] auto Only() const -> Captured {
    EXPECT_EQ(captured_.size(), 1);
    return captured_.isEmpty() ? Captured{} : captured_.first();
  }

  QList<Captured> captured_;
  bool previous_rules_ = false;
};

}  // namespace

TEST_F(LogCaptureFixture,
       AModuleIdBecomesItsOwnCategoryRatherThanTheSharedOne) {
  GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "something");

  EXPECT_EQ(Only().category, "module.email");
  EXPECT_NE(Only().category, "module");
}

TEST_F(LogCaptureFixture, TwoModulesAreDistinguishableRatherThanAnonymous) {
  GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "a");
  GFLogAt(OtherCtx(), GF_LOG_WARN, nullptr, 0, nullptr, "b");

  ASSERT_EQ(captured_.size(), 2);
  EXPECT_NE(captured_[0].category, captured_[1].category);
}

TEST_F(LogCaptureFixture,
       ASourceLocationSurvivesTheAbiRatherThanNamingTheShim) {
  // The defect this replaces: every module line reported GFSDKLog.cpp, because
  // the qC* macros capture the context of wherever they are written.
  GFLogAt(Ctx(), GF_LOG_ERROR, "EMailImapController.cpp", 412, "connect()",
          "imap connect failed");

  const auto only = Only();
  EXPECT_EQ(only.file, "EMailImapController.cpp");
  EXPECT_EQ(only.line, 412);
  EXPECT_EQ(only.function, "connect()");
  EXPECT_FALSE(only.file.contains("GFSDKLog"));
}

TEST_F(LogCaptureFixture, ANullLocationIsOmittedRatherThanCrashing) {
  GFLogAt(Ctx(), GF_LOG_INFO, nullptr, 0, nullptr, nullptr);

  EXPECT_EQ(captured_.size(), 1);
  EXPECT_TRUE(Only().file.isEmpty());
}

TEST_F(LogCaptureFixture, LoggingBeforeActivationIsSilentRatherThanACrash) {
  // The pre-activation window: a module that logs from a static initializer
  // has no context yet. It must not crash, and it must not invent a category.
  //
  // This replaces a test that asserted an unattributable message came out
  // under `module.unknown`. That case is gone, and not because the behaviour
  // changed: a module id is no longer something a caller supplies, so there
  // is nothing left to be unattributable. A caller either has a context, in
  // which case it names a module, or has none. Forging one is a question
  // about the HOST token rather than about this layer, and
  // ModuleApiTest.AForgedContextIsRefusedRatherThanFollowed covers it.
  GFSDKContext unbound{};
  unbound.struct_size = sizeof(GFSDKContext);
  unbound.host = nullptr;

  GFLogAt(&unbound, GF_LOG_WARN, nullptr, 0, nullptr, "too early");
  GFLogAt(nullptr, GF_LOG_WARN, nullptr, 0, nullptr, "even earlier");

  EXPECT_TRUE(captured_.isEmpty());
  EXPECT_EQ(GFLogEnabled(&unbound, GF_LOG_WARN), 0);
  EXPECT_EQ(GFLogEnabled(nullptr, GF_LOG_WARN), 0);
}

TEST_F(LogCaptureFixture, TheContextDecidesTheCategoryNotTheCallStack) {
  // There is no module id argument any more, and no preference rule between
  // one and a thread-local: the context says who is logging, and it says the
  // same thing wherever the call is made from. Here the host is "inside"
  // another module entirely, and the line is still attributed to the context
  // that produced it.
  const Module::ModuleAttributionScope attributed(kOtherId);
  GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "who am i");

  EXPECT_EQ(Only().category, "module.email");
}

TEST_F(LogCaptureFixture, TheContextIsUsedOffAHostCalledThreadRatherThanLost) {
  // A module's own worker thread: the host never entered it, so a
  // thread-local answer would be empty. The context carries the identity, so
  // there is nothing to lose. This is the case the argument exists for.
  auto* thread = QThread::create([&]() {
    GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "from a worker");
  });
  thread->start();
  ASSERT_TRUE(thread->wait(5000));
  delete thread;

  EXPECT_EQ(Only().category, "module.email");
}

TEST_F(LogCaptureFixture, TraceIsADistinctCategoryRatherThanAnAliasOfDebug) {
  GFLogAt(Ctx(), GF_LOG_TRACE, nullptr, 0, nullptr, "chatter");
  GFLogAt(Ctx(), GF_LOG_DEBUG, nullptr, 0, nullptr, "detail");

  ASSERT_EQ(captured_.size(), 2);
  EXPECT_EQ(captured_[0].category, "module.email.trace");
  EXPECT_EQ(captured_[1].category, "module.email");
}

TEST_F(LogCaptureFixture, EverySeverityKeepsItsOwnQtTypeRatherThanCollapsing) {
  GFLogAt(Ctx(), GF_LOG_DEBUG, nullptr, 0, nullptr, "d");
  GFLogAt(Ctx(), GF_LOG_INFO, nullptr, 0, nullptr, "i");
  GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "w");
  GFLogAt(Ctx(), GF_LOG_ERROR, nullptr, 0, nullptr, "e");

  ASSERT_EQ(captured_.size(), 4);
  EXPECT_EQ(captured_[0].type, QtDebugMsg);
  EXPECT_EQ(captured_[1].type, QtInfoMsg);
  EXPECT_EQ(captured_[2].type, QtWarningMsg);
  EXPECT_EQ(captured_[3].type, QtCriticalMsg);
}

TEST_F(LogCaptureFixture, EverySeverityEmitsOneMessageWithoutACallSite) {
  GFLogAt(Ctx(), GF_LOG_TRACE, nullptr, 0, nullptr, "t");
  GFLogAt(Ctx(), GF_LOG_DEBUG, nullptr, 0, nullptr, "d");
  GFLogAt(Ctx(), GF_LOG_INFO, nullptr, 0, nullptr, "i");
  GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "w");
  GFLogAt(Ctx(), GF_LOG_ERROR, nullptr, 0, nullptr, "e");

  EXPECT_EQ(captured_.size(), 5);
}

TEST_F(LogCaptureFixture, ASuppressedMessageIsNotEmittedRatherThanFiltered) {
  QLoggingCategory::setFilterRules("module.email.debug=false\n");

  GFLogAt(Ctx(), GF_LOG_DEBUG, nullptr, 0, nullptr, "quiet");
  EXPECT_EQ(GFLogEnabled(Ctx(), GF_LOG_DEBUG), 0);
  EXPECT_TRUE(captured_.isEmpty());

  // ...and the check agrees with what actually happens, which is the whole
  // value of it: a module skipping formatting must not skip a message that
  // would have been emitted.
  EXPECT_NE(GFLogEnabled(Ctx(), GF_LOG_ERROR), 0);
  GFLogAt(Ctx(), GF_LOG_ERROR, nullptr, 0, nullptr, "loud");
  EXPECT_EQ(captured_.size(), 1);
}

TEST_F(LogCaptureFixture, TraceIsOffAtDebugLevelRatherThanFloodingIt) {
  // The rule BuildQtLoggingFilterRules installs for every level above trace.
  QLoggingCategory::setFilterRules(
      BuildQtLoggingFilterRules(static_cast<int>(GFLogLevel::kDEBUG)));

  EXPECT_EQ(GFLogEnabled(Ctx(), GF_LOG_TRACE), 0);
  EXPECT_NE(GFLogEnabled(Ctx(), GF_LOG_DEBUG), 0);

  GFLogAt(Ctx(), GF_LOG_TRACE, nullptr, 0, nullptr, "chatter");
  EXPECT_TRUE(captured_.isEmpty());
}

TEST_F(LogCaptureFixture, TraceIsOnAtTraceLevelRatherThanUnreachable) {
  QLoggingCategory::setFilterRules(
      BuildQtLoggingFilterRules(static_cast<int>(GFLogLevel::kTRACE)));

  EXPECT_NE(GFLogEnabled(Ctx(), GF_LOG_TRACE), 0);

  GFLogAt(Ctx(), GF_LOG_TRACE, nullptr, 0, nullptr, "chatter");
  EXPECT_EQ(captured_.size(), 1);
}

TEST_F(LogCaptureFixture, AMessageIsNotDoubleQuotedRatherThanEscaped) {
  // What arrives at GFModuleLogAt is a finished message. Emitting it through
  // QDebug's default quoting wrapped every module line in quotes it never
  // asked for, and escaped any quote the message legitimately contained.
  GFLogAt(Ctx(), GF_LOG_WARN, nullptr, 0, nullptr, "he said \"hello\"");

  EXPECT_EQ(Only().message, "he said \"hello\"");
  EXPECT_FALSE(Only().message.startsWith('"'));
}

TEST(SdkLogLevelTest, TraceParsesAsALevelRatherThanAsUnspecified) {
  const auto trace = ParseLogLevelName("trace");
  ASSERT_TRUE(trace.has_value());
  EXPECT_EQ(*trace, static_cast<int>(GFLogLevel::kTRACE));

  // kDEBUG must stay 0: an unset advanced/log_level setting reads back as 0,
  // and the resolution path depends on that meaning debug.
  EXPECT_EQ(static_cast<int>(GFLogLevel::kDEBUG), 0);
  EXPECT_LT(static_cast<int>(GFLogLevel::kTRACE), 0);
}

}  // namespace GpgFrontend::Test
