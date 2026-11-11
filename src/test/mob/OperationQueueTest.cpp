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

#include <QSignalSpy>

#include "MobileTest.h"
#include "crypto/OperationQueue.h"
#include "lifecycle/AppLifecycle.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::AppLifecycle;
using GpgFrontend::Mobile::OperationLog;
using GpgFrontend::Mobile::OperationParams;
using GpgFrontend::Mobile::OperationPhase;
using GpgFrontend::Mobile::OperationQueue;
using GpgFrontend::Mobile::OperationRecord;
using GpgFrontend::Mobile::OperationSurface;
using GpgFrontend::Mobile::TextVersion;

namespace {

auto Ended(int id, int batch = 0) -> OperationRecord {
  OperationRecord record;
  record.id = id;
  record.batch_id = batch > 0 ? batch : id;
  record.params.op = QStringLiteral("encrypt");
  record.phase = OperationPhase::kDONE;
  return record;
}

auto Ids(const OperationLog& log) -> QList<int> {
  QList<int> ids;
  for (int row = 0; row < log.rowCount(); ++row) {
    ids << log.data(log.index(row), OperationLog::kIdRole).toInt();
  }
  return ids;
}

/// A queue with no prompts and no lifecycle, and runners that only note
/// that they were started: the test ends each operation itself.
struct Fixture {
  OperationQueue queue{nullptr, nullptr};
  QObject text_page;
  QObject files_page;
  QList<int> started;

  auto Submit(QObject* owner, OperationSurface surface,
              const QString& op = QStringLiteral("encrypt"), int batch = 0)
      -> int {
    OperationQueue::Request request;
    request.owner = owner;
    request.surface = surface;
    request.params.op = op;
    request.batch_id = batch;
    request.run = [this](int id) { started << id; };
    return queue.Submit(std::move(request));
  }

  auto Phase(int id) -> OperationPhase {
    const auto* record = queue.Log()->Find(id);
    return record != nullptr ? record->phase : OperationPhase::kFAILED;
  }

  void Done(int id) { queue.Complete(id, OperationPhase::kDONE, {}); }
};

}  // namespace

TEST(OperationLogTest, NewestFirstAndOnlyItsCourseChanges) {
  RunOnMainThread([]() {
    OperationLog log;
    auto first = Ended(1);
    first.phase = OperationPhase::kQUEUED;
    first.params.recipients = {"AAAA"};
    log.Add(first);
    log.Add(Ended(2));
    EXPECT_EQ(Ids(log), (QList<int>{2, 1}));

    log.SetStarted(1);
    EXPECT_EQ(log.Find(1)->phase, OperationPhase::kRUNNING);
    EXPECT_TRUE(log.Find(1)->started.isValid());
    log.SetFinished(1, OperationPhase::kFAILED, "stopped", {{"status", -1}},
                    {7});
    const auto record = log.record(1);
    EXPECT_EQ(record.value("phase").toString(), "failed");
    EXPECT_EQ(record.value("reason").toString(), "stopped");
    EXPECT_EQ(record.value("outputs").toList(), (QVariantList{7}));
    // What it was asked to do stays as it was asked.
    EXPECT_EQ(record.value("recipients").toStringList(), QStringList{"AAAA"});
    EXPECT_TRUE(log.record(99).isEmpty());
  });
}

TEST(OperationLogTest, TheOldestEndedGoFirstButNothingActive) {
  RunOnMainThread([]() {
    OperationLog log;
    log.SetCapacity(2);
    auto running = Ended(1);
    running.phase = OperationPhase::kRUNNING;
    log.Add(running);
    log.Add(Ended(2));
    log.Add(Ended(3));
    log.Add(Ended(4));
    // Two ended ones kept, the newest; the running one however old it is.
    EXPECT_EQ(Ids(log), (QList<int>{4, 3, 1}));
  });
}

TEST(OperationLogTest, ABatchGoesOnlyAsAWholeOnceItIsOver) {
  RunOnMainThread([]() {
    OperationLog log;
    log.SetCapacity(1);
    log.Add(Ended(1, 1));
    auto second = Ended(2, 1);
    second.phase = OperationPhase::kQUEUED;
    log.Add(second);
    log.Add(Ended(3));
    // Batch 1 still has one waiting: it stays whole, the newer one too.
    EXPECT_EQ(Ids(log), (QList<int>{3, 2, 1}));

    log.SetFinished(2, OperationPhase::kDONE, {}, {}, {});
    // Over now, and the one that ended last: all of it stays, and what
    // ended before it goes.
    EXPECT_EQ(Ids(log), (QList<int>{2, 1}));
    log.Add(Ended(4));
    EXPECT_EQ(Ids(log), (QList<int>{4}));
  });
}

TEST(OperationLogTest, TheBatchJustFinishedStaysHoweverLarge) {
  RunOnMainThread([]() {
    OperationLog log;
    log.SetCapacity(2);
    log.Add(Ended(1, 1));
    // A batch larger than the log holds, ended last: still all there, for
    // its outputs and for Retry; the older one goes.
    for (int id = 2; id <= 4; ++id) log.Add(Ended(id, 2));
    EXPECT_EQ(Ids(log), (QList<int>{4, 3, 2}));
  });
}

TEST(OperationLogTest, TheBatchThatEndedLastStaysThoughAskedForEarlier) {
  RunOnMainThread([]() {
    OperationLog log;
    log.SetCapacity(2);
    log.Add(Ended(1, 1));
    // A large batch asked for, still going ...
    for (int id = 2; id <= 4; ++id) {
      auto going = Ended(id, 2);
      going.phase = OperationPhase::kQUEUED;
      log.Add(going);
    }
    // ... a text asked for after it ends first ...
    log.Add(Ended(5, 5));
    // ... then the batch ends: the one just finished, all of it, stays,
    // and what ended before it goes (the text too: the log holds two).
    for (int id = 2; id <= 4; ++id) {
      log.SetFinished(id, OperationPhase::kDONE, {}, {}, {});
    }
    EXPECT_EQ(Ids(log), (QList<int>{4, 3, 2}));
  });
}

TEST(OperationLogTest, ADecryptionThatVerifiesSaysSo) {
  RunOnMainThread([]() {
    OperationLog log;
    auto both = Ended(1);
    both.params.op = QStringLiteral("decrypt");
    both.params.verify = true;
    log.Add(both);
    EXPECT_EQ(log.record(1).value("operation").toString(), "decrypt");
    EXPECT_TRUE(log.record(1).value("verify").toBool());
  });
}

// A batch is read as the desktop reads a report of several items: a
// failure anywhere makes it critical and stamps the failure seal; files
// stopped beside finished ones leave it partly done; all stopped, cancelled.
TEST(OperationLogTest, ABatchIsSummedUpAsTheDesktopSumsAReport) {
  OperationLog log;
  const auto item = [](const char* phase, const char* outcome = nullptr) {
    QVariantMap report;
    if (outcome != nullptr) report.insert("outcome", outcome);
    return QVariant(QVariantMap{{"phase", phase}, {"report", report}});
  };

  const auto all_done = log.summarize({item("done", "succeeded"), item("done")});
  EXPECT_EQ(all_done.value("severity"), "ok");
  EXPECT_EQ(all_done.value("seal"), "success");
  EXPECT_EQ(all_done.value("status").toInt(), 1);

  const auto part_failed =
      log.summarize({item("done", "succeeded"), item("failed")});
  EXPECT_EQ(part_failed.value("severity"), "critical");
  EXPECT_EQ(part_failed.value("seal"), "failure");
  EXPECT_EQ(part_failed.value("status").toInt(), -1);

  const auto part_stopped =
      log.summarize({item("done", "succeeded"), item("cancelled")});
  EXPECT_EQ(part_stopped.value("severity"), "warn");
  EXPECT_EQ(part_stopped.value("seal"), "warning");
  EXPECT_FALSE(part_stopped.value("cancelled").toBool());

  const auto all_stopped = log.summarize({item("cancelled"), item("cancelled")});
  EXPECT_TRUE(all_stopped.value("cancelled").toBool());
  EXPECT_EQ(all_stopped.value("seal"), "cancelled");

  // The engine's own word wins over how the item ended.
  const auto warned = log.summarize({item("done", "warning"), item("done")});
  EXPECT_EQ(warned.value("severity"), "warn");
  EXPECT_EQ(warned.value("seal"), "warning");
}

// The session's history: one row per batch, shown by its first operation
// with how many it holds, the batch added last first.
TEST(OperationLogTest, RecentListsEachBatchOnceNewestFirst) {
  RunOnMainThread([]() {
    OperationLog log;
    EXPECT_TRUE(log.recent().isEmpty());
    log.Add(Ended(1));
    log.Add(Ended(2, 2));
    log.Add(Ended(3, 2));
    log.Add(Ended(4, 2));
    log.Add(Ended(5));
    const auto recent = log.recent();
    ASSERT_EQ(recent.size(), 3);
    EXPECT_EQ(recent.at(0).toMap().value("operationId").toInt(), 5);
    EXPECT_EQ(recent.at(0).toMap().value("count").toInt(), 1);
    EXPECT_EQ(recent.at(1).toMap().value("operationId").toInt(), 2);
    EXPECT_EQ(recent.at(1).toMap().value("count").toInt(), 3);
    EXPECT_EQ(recent.at(2).toMap().value("operationId").toInt(), 1);
  });
}

TEST(OperationLogTest, TheNewestOfAPageOrOfAFile) {
  RunOnMainThread([]() {
    OperationLog log;
    EXPECT_EQ(log.latest(), 0);
    auto text = Ended(1);
    text.surface = OperationSurface::kTEXT;
    log.Add(text);
    auto file = Ended(2);
    file.surface = OperationSurface::kFILES;
    file.targets = {{7, "a.txt", "1"}};
    log.Add(file);
    auto check = Ended(3);
    check.surface = OperationSurface::kFILES;
    check.targets = {{8, "a.txt.sig", "1"}, {7, "a.txt", "1"}};
    log.Add(check);

    EXPECT_EQ(log.latest(), 3);
    EXPECT_EQ(log.latest("text"), 1);
    EXPECT_EQ(log.latest("files"), 3);
    // A file a signature was checked over counts as worked on.
    EXPECT_EQ(log.latestOnFile(7), 3);
    EXPECT_EQ(log.latestOnFile(8), 3);
    EXPECT_EQ(log.latestOnFile(9), 0);
  });
}

TEST(OperationLogTest, TextVersionsCompareWithoutSayingWhatTheTextIs) {
  EXPECT_EQ(TextVersion("hello"), TextVersion("hello"));
  EXPECT_NE(TextVersion("hello"), TextVersion("hello "));
  EXPECT_FALSE(TextVersion("hello").contains("hello"));
  EXPECT_EQ(TextVersion("").size(), 64);
}

TEST(OperationQueueTest, OneAtATimeAcrossPages) {
  RunOnMainThread([]() {
    Fixture f;
    QSignalSpy busy(&f.queue, &OperationQueue::SignalBusyChanged);
    const auto a = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto b = f.Submit(&f.text_page, OperationSurface::kTEXT);
    EXPECT_EQ(f.started, QList<int>{a});
    EXPECT_EQ(f.queue.CurrentId(), a);
    EXPECT_EQ(f.Phase(b), OperationPhase::kQUEUED);
    EXPECT_TRUE(f.queue.Busy());

    f.Done(a);
    EXPECT_EQ(f.started, (QList<int>{a, b}));
    EXPECT_EQ(f.Phase(a), OperationPhase::kDONE);
    f.Done(b);
    EXPECT_FALSE(f.queue.Busy());
    EXPECT_EQ(f.queue.CurrentId(), 0);
    EXPECT_EQ(busy.count(), 2);  // busy once, idle once
  });
}

TEST(OperationQueueTest, TextGoesBeforeWaitingFilesButNeverInterrupts) {
  RunOnMainThread([]() {
    Fixture f;
    const auto running = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto file2 = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto text1 = f.Submit(&f.text_page, OperationSurface::kTEXT);
    const auto text2 = f.Submit(&f.text_page, OperationSurface::kTEXT);
    EXPECT_EQ(f.queue.CurrentId(), running);
    f.Done(running);
    f.Done(text1);
    f.Done(text2);
    f.Done(file2);
    EXPECT_EQ(f.started, (QList<int>{running, text1, text2, file2}));
  });
}

TEST(OperationQueueTest, ARunnerThatEndsAtOnceLetsTheNextStart) {
  RunOnMainThread([]() {
    Fixture f;
    QList<int> order;
    for (int i = 0; i < 3; ++i) {
      OperationQueue::Request request;
      request.owner = &f.files_page;
      request.surface = OperationSurface::kFILES;
      request.params.op = "verify";
      request.run = [&f, &order](int id) {
        order << id;
        f.queue.Complete(id, OperationPhase::kFAILED, {{"status", -1}});
      };
      f.queue.Submit(std::move(request));
    }
    EXPECT_EQ(order.size(), 3);
    EXPECT_FALSE(f.queue.Busy());
  });
}

TEST(OperationQueueTest, ABatchSharesItsFirstId) {
  RunOnMainThread([]() {
    Fixture f;
    const auto first = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto second =
        f.Submit(&f.files_page, OperationSurface::kFILES, "encrypt", first);
    EXPECT_EQ(f.queue.Log()->Find(first)->batch_id, first);
    EXPECT_EQ(f.queue.Log()->Find(second)->batch_id, first);
  });
}

TEST(OperationQueueTest, OnlyWhatWaitsCanBeCancelled) {
  RunOnMainThread([]() {
    Fixture f;
    QSignalSpy finished(&f.queue, &OperationQueue::SignalFinished);
    const auto a = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto b = f.Submit(&f.files_page, OperationSurface::kFILES);
    EXPECT_FALSE(f.queue.Cancel(a, "stopped"));
    EXPECT_TRUE(f.queue.Cancel(b, "stopped"));
    EXPECT_EQ(f.Phase(b), OperationPhase::kCANCELLED);
    EXPECT_EQ(f.queue.Log()->Find(b)->reason, "stopped");
    ASSERT_EQ(finished.count(), 1);
    EXPECT_EQ(finished.at(0).at(0).toInt(), b);
    f.Done(a);
    EXPECT_EQ(f.started, QList<int>{a});  // b never ran
  });
}

TEST(OperationQueueTest, AnUnknownOrWaitingOperationCannotBeCompleted) {
  RunOnMainThread([]() {
    Fixture f;
    const auto a = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto b = f.Submit(&f.files_page, OperationSurface::kFILES);
    f.Done(b);
    f.Done(42);
    EXPECT_EQ(f.Phase(b), OperationPhase::kQUEUED);
    EXPECT_EQ(f.queue.CurrentId(), a);
  });
}

TEST(OperationQueueTest, APageThatGoesTakesItsOperations) {
  RunOnMainThread([]() {
    Fixture f;
    auto page = std::make_unique<QObject>();
    const auto running = f.Submit(page.get(), OperationSurface::kFILES);
    const auto waiting = f.Submit(page.get(), OperationSurface::kFILES);
    const auto other = f.Submit(&f.text_page, OperationSurface::kTEXT);
    page.reset();
    EXPECT_EQ(f.Phase(running), OperationPhase::kCANCELLED);
    EXPECT_EQ(f.Phase(waiting), OperationPhase::kCANCELLED);
    EXPECT_EQ(f.queue.Log()->Find(running)->reason, "closed");
    // The other page's goes on.
    EXPECT_EQ(f.queue.CurrentId(), other);
  });
}

TEST(OperationQueueTest, ClosedItRefusesAndDropsWhatWaits) {
  RunOnMainThread([]() {
    Fixture f;
    const auto a = f.Submit(&f.files_page, OperationSurface::kFILES);
    const auto b = f.Submit(&f.files_page, OperationSurface::kFILES);
    f.queue.Close();
    EXPECT_EQ(f.Phase(b), OperationPhase::kCANCELLED);
    EXPECT_EQ(f.Submit(&f.text_page, OperationSurface::kTEXT), 0);
    // What runs ends on its own.
    f.Done(a);
    EXPECT_EQ(f.Phase(a), OperationPhase::kDONE);
    EXPECT_FALSE(f.queue.Busy());
  });
}

TEST(OperationQueueTest, NothingWithoutAnOwnerOrARunner) {
  RunOnMainThread([]() {
    Fixture f;
    OperationQueue::Request request;
    request.run = [](int) {};
    EXPECT_EQ(f.queue.Submit(request), 0);
    request.owner = &f.text_page;
    request.run = nullptr;
    EXPECT_EQ(f.queue.Submit(request), 0);
    EXPECT_EQ(f.queue.Log()->rowCount(), 0);
  });
}

TEST(OperationQueueTest, ACancelAfterTheBackgroundSaysSo) {
  RunOnMainThread([]() {
    auto* lifecycle = AppLifecycle::create(nullptr, nullptr);
    ASSERT_NE(lifecycle, nullptr);
    OperationQueue queue(nullptr, lifecycle);
    QObject page;
    const auto submit = [&]() {
      OperationQueue::Request request;
      request.owner = &page;
      request.params.op = "decrypt";
      request.run = [](int) {};
      return queue.Submit(std::move(request));
    };

    // Waiting at its prompt when the application left: declined by it.
    const auto a = submit();
    queue.Log()->SetPhase(a, OperationPhase::kWAITING_PASSPHRASE);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
    EXPECT_EQ(queue.EndReason(a, OperationPhase::kCANCELLED), "background");
    queue.Complete(a, OperationPhase::kCANCELLED, {{"cancelled", true}});
    EXPECT_EQ(queue.Log()->Find(a)->reason, "background");

    // Only for the one that was running then.
    const auto b = submit();
    queue.Complete(b, OperationPhase::kCANCELLED, {{"cancelled", true}});
    EXPECT_TRUE(queue.Log()->Find(b)->reason.isEmpty());

    // Running, not yet asking, while the application was away: a prompt
    // it asks later, and cancelled then, is not the background's doing.
    const auto c = submit();
    lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
    queue.Log()->SetPhase(c, OperationPhase::kWAITING_PASSPHRASE);
    queue.Complete(c, OperationPhase::kCANCELLED, {{"cancelled", true}});
    EXPECT_TRUE(queue.Log()->Find(c)->reason.isEmpty());

    // Recorded by the page first, then completed: once, with its reason.
    const auto d = submit();
    queue.Log()->SetPhase(d, OperationPhase::kWAITING_PASSPHRASE);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
    lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
    queue.Log()->SetFinished(d, OperationPhase::kCANCELLED,
                             queue.EndReason(d, OperationPhase::kCANCELLED),
                             {{"cancelled", true}}, {});
    QSignalSpy changed(queue.Log(), &OperationLog::dataChanged);
    queue.Complete(d, OperationPhase::kCANCELLED, {{"cancelled", true}});
    EXPECT_EQ(changed.count(), 0);
    EXPECT_EQ(queue.Log()->Find(d)->reason, "background");
  });
}

}  // namespace GpgFrontend::Test::Mobile
