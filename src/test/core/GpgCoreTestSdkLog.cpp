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
#include "core/module/ModuleLogCategory.h"
#include "sdk/GFSDKLog.h"
#include "sdk/GFSDKModuleAttribution.h"

namespace GpgFrontend::Test {

namespace {

constexpr auto kEmailId = "com.bktus.gpgfrontend.module.email";
constexpr auto kOtherId = "com.bktus.gpgfrontend.module.key_server_sync";

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
  GFModuleLogAt(kEmailId, GF_LOG_WARN, nullptr, 0, nullptr, "something");

  EXPECT_EQ(Only().category, "module.email");
  EXPECT_NE(Only().category, "module");
}

TEST_F(LogCaptureFixture, TwoModulesAreDistinguishableRatherThanAnonymous) {
  GFModuleLogAt(kEmailId, GF_LOG_WARN, nullptr, 0, nullptr, "a");
  GFModuleLogAt(kOtherId, GF_LOG_WARN, nullptr, 0, nullptr, "b");

  ASSERT_EQ(captured_.size(), 2);
  EXPECT_NE(captured_[0].category, captured_[1].category);
}

TEST_F(LogCaptureFixture,
       ASourceLocationSurvivesTheAbiRatherThanNamingTheShim) {
  // The defect this replaces: every module line reported GFSDKLog.cpp, because
  // the qC* macros capture the context of wherever they are written.
  GFModuleLogAt(kEmailId, GF_LOG_ERROR, "EMailImapController.cpp", 412,
                "connect()", "imap connect failed");

  const auto only = Only();
  EXPECT_EQ(only.file, "EMailImapController.cpp");
  EXPECT_EQ(only.line, 412);
  EXPECT_EQ(only.function, "connect()");
  EXPECT_FALSE(only.file.contains("GFSDKLog"));
}

TEST_F(LogCaptureFixture, ANullLocationIsOmittedRatherThanCrashing) {
  GFModuleLogAt(kEmailId, GF_LOG_INFO, nullptr, 0, nullptr, nullptr);

  EXPECT_EQ(captured_.size(), 1);
  EXPECT_TRUE(Only().file.isEmpty());
}

TEST_F(LogCaptureFixture, AnUnattributableMessageIsUnknownRatherThanDropped) {
  GFModuleLogAt(nullptr, GF_LOG_WARN, nullptr, 0, nullptr, "from nowhere");

  EXPECT_EQ(Only().category, "module.unknown");
}

TEST_F(LogCaptureFixture,
       TheThreadLocalWinsOverTheSuppliedIdRatherThanTheOtherWayRound) {
  // The half a module does not control is preferred wherever it exists. A
  // module naming itself only decides the answer where the host never entered.
  const auto* previous = GFSdkEnterModule(kOtherId);
  GFModuleLogAt(kEmailId, GF_LOG_WARN, nullptr, 0, nullptr, "who am i");
  GFSdkLeaveModule(previous);

  EXPECT_EQ(Only().category, "module.key-server-sync");
}

TEST_F(LogCaptureFixture,
       TheSuppliedIdIsUsedOffAHostCalledThreadRatherThanLost) {
  // A module's own worker thread: the host never entered it, so the
  // thread-local is empty and the argument is the only attribution there is.
  // This is the case the whole module_id parameter exists for.
  QString category;
  auto* thread = QThread::create([&]() {
    GFModuleLogAt(kEmailId, GF_LOG_WARN, nullptr, 0, nullptr, "from a worker");
  });
  thread->start();
  ASSERT_TRUE(thread->wait(5000));
  delete thread;

  EXPECT_EQ(Only().category, "module.email");
}

TEST_F(LogCaptureFixture, TraceIsADistinctCategoryRatherThanAnAliasOfDebug) {
  GFModuleLogAt(kEmailId, GF_LOG_TRACE, nullptr, 0, nullptr, "chatter");
  GFModuleLogAt(kEmailId, GF_LOG_DEBUG, nullptr, 0, nullptr, "detail");

  ASSERT_EQ(captured_.size(), 2);
  EXPECT_EQ(captured_[0].category, "module.email.trace");
  EXPECT_EQ(captured_[1].category, "module.email");
}

TEST_F(LogCaptureFixture, EverySeverityKeepsItsOwnQtTypeRatherThanCollapsing) {
  GFModuleLogAt(kEmailId, GF_LOG_DEBUG, nullptr, 0, nullptr, "d");
  GFModuleLogAt(kEmailId, GF_LOG_INFO, nullptr, 0, nullptr, "i");
  GFModuleLogAt(kEmailId, GF_LOG_WARN, nullptr, 0, nullptr, "w");
  GFModuleLogAt(kEmailId, GF_LOG_ERROR, nullptr, 0, nullptr, "e");

  ASSERT_EQ(captured_.size(), 4);
  EXPECT_EQ(captured_[0].type, QtDebugMsg);
  EXPECT_EQ(captured_[1].type, QtInfoMsg);
  EXPECT_EQ(captured_[2].type, QtWarningMsg);
  EXPECT_EQ(captured_[3].type, QtCriticalMsg);
}

TEST_F(LogCaptureFixture, TheLegacyEntryPointsStillEmitRatherThanBreaking) {
  GFModuleLogTrace("t");
  GFModuleLogDebug("d");
  GFModuleLogInfo("i");
  GFModuleLogWarn("w");
  GFModuleLogError("e");

  EXPECT_EQ(captured_.size(), 5);
}

TEST_F(LogCaptureFixture, ASuppressedMessageIsNotEmittedRatherThanFiltered) {
  QLoggingCategory::setFilterRules("module.email.debug=false\n");

  GFModuleLogAt(kEmailId, GF_LOG_DEBUG, nullptr, 0, nullptr, "quiet");
  EXPECT_EQ(GFModuleLogEnabled(kEmailId, GF_LOG_DEBUG), 0);
  EXPECT_TRUE(captured_.isEmpty());

  // ...and the check agrees with what actually happens, which is the whole
  // value of it: a module skipping formatting must not skip a message that
  // would have been emitted.
  EXPECT_NE(GFModuleLogEnabled(kEmailId, GF_LOG_ERROR), 0);
  GFModuleLogAt(kEmailId, GF_LOG_ERROR, nullptr, 0, nullptr, "loud");
  EXPECT_EQ(captured_.size(), 1);
}

TEST_F(LogCaptureFixture, TraceIsOffAtDebugLevelRatherThanFloodingIt) {
  // The rule BuildQtLoggingFilterRules installs for every level above trace.
  QLoggingCategory::setFilterRules(
      BuildQtLoggingFilterRules(static_cast<int>(GFLogLevel::kDEBUG)));

  EXPECT_EQ(GFModuleLogEnabled(kEmailId, GF_LOG_TRACE), 0);
  EXPECT_NE(GFModuleLogEnabled(kEmailId, GF_LOG_DEBUG), 0);

  GFModuleLogAt(kEmailId, GF_LOG_TRACE, nullptr, 0, nullptr, "chatter");
  EXPECT_TRUE(captured_.isEmpty());
}

TEST_F(LogCaptureFixture, TraceIsOnAtTraceLevelRatherThanUnreachable) {
  QLoggingCategory::setFilterRules(
      BuildQtLoggingFilterRules(static_cast<int>(GFLogLevel::kTRACE)));

  EXPECT_NE(GFModuleLogEnabled(kEmailId, GF_LOG_TRACE), 0);

  GFModuleLogAt(kEmailId, GF_LOG_TRACE, nullptr, 0, nullptr, "chatter");
  EXPECT_EQ(captured_.size(), 1);
}

TEST_F(LogCaptureFixture, AMessageIsNotDoubleQuotedRatherThanEscaped) {
  // What arrives at GFModuleLogAt is a finished message. Emitting it through
  // QDebug's default quoting wrapped every module line in quotes it never
  // asked for, and escaped any quote the message legitimately contained.
  GFModuleLogAt(kEmailId, GF_LOG_WARN, nullptr, 0, nullptr,
                "he said \"hello\"");

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
