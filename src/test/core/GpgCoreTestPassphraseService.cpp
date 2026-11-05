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

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "GFCoreTest.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/PassphraseService.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend::Test {

namespace {

constexpr auto kTimeoutSettingKey = "engine/passphrase_prompt_timeout";
constexpr auto kGraceSettingKey = "engine/passphrase_requester_grace_ms";

// A channel of this suite's own. Asking for a passphrase reaches into the
// channel's key repository, which brings that channel's OpenPGP context into
// existence; doing that on the default channel leaves a context behind that the
// engine suites running later cannot use. Nothing else touches this number.
constexpr int kPassphraseTestChannel = 4096;

// Any 40-hex string works: no such key exists in the test keyring, so
// PassphraseService just logs and carries on with a null key. What matters is
// that the fingerprint takes part in the in-flight key.
constexpr auto kFprA = "1111111111111111111111111111111111111111";
constexpr auto kFprB = "2222222222222222222222222222222222222222";

/**
 * @brief Stands in for the real passphrase dialog.
 *
 * Connects to SignalNeedUserInputPassphrase exactly as PassphrasePrompt does
 * — with the application object as context, so the handler runs on the main
 * thread — and answers after a delay instead of opening a window. The delay is
 * the point: it holds the "prompt" open long enough for a second request to
 * appear if the service ever lets one through.
 */
class FakePassphrasePrompt : public QObject {
 public:
  explicit FakePassphrasePrompt(int answer_delay_ms = 200,
                                QString answer = u"secret"_s)
      : answer_delay_ms_(answer_delay_ms), answer_(std::move(answer)) {
    // The real handler in ui/function/PassphrasePrompt.h is live in the test
    // runner too, but it opens no prompt in unit-test mode and leaves the
    // request unanswered, so this stand-in is the only thing that ever
    // answers one here.
    connection_ = connect(
        CoreSignalStation::GetInstance(),
        &CoreSignalStation::SignalNeedUserInputPassphrase,
        QCoreApplication::instance(),
        [this](const QSharedPointer<GpgPassphraseContext>& c) { Open(c); });

    close_connection_ =
        connect(CoreSignalStation::GetInstance(),
                &CoreSignalStation::SignalCloseUserInputPassphrase,
                QCoreApplication::instance(),
                [this](const QSharedPointer<GpgPassphraseContext>& c) {
                  close_requests_.append(c);
                  Answer(c, GFBuffer(), true);
                });
  }

  ~FakePassphrasePrompt() override {
    disconnect(connection_);
    disconnect(close_connection_);
  }

  [[nodiscard]] auto PromptCount() const -> int { return prompt_count_; }
  [[nodiscard]] auto MaxConcurrentPrompts() const -> int {
    return max_concurrent_;
  }
  [[nodiscard]] auto CloseRequestCount() const -> int {
    return static_cast<int>(close_requests_.size());
  }
  [[nodiscard]] auto Contexts() const
      -> QList<QSharedPointer<GpgPassphraseContext>> {
    return contexts_;
  }

  /// Stop answering, so a request has to run into its own deadline.
  void StopAnswering() { answering_ = false; }

 private:
  void Open(const QSharedPointer<GpgPassphraseContext>& c) {
    ++prompt_count_;
    contexts_.append(c);
    open_ = open_ + 1;
    max_concurrent_ = std::max(max_concurrent_, open_);

    if (!answering_) return;

    QTimer::singleShot(answer_delay_ms_, QCoreApplication::instance(),
                       [this, c]() { Answer(c, GFBuffer(answer_), false); });
  }

  void Answer(const QSharedPointer<GpgPassphraseContext>& c,
              const GFBuffer& passphrase, bool cancelled) {
    if (answered_.contains(c)) return;
    answered_.append(c);
    open_ = open_ - 1;

    c->SetPassphrase(passphrase);
    c->SetCancelled(cancelled);
    emit CoreSignalStation::GetInstance() -> SignalUserInputPassphraseReady(c);
  }

  int answer_delay_ms_;
  QString answer_;
  bool answering_ = true;
  int prompt_count_ = 0;
  int open_ = 0;
  int max_concurrent_ = 0;
  QList<QSharedPointer<GpgPassphraseContext>> contexts_;
  QList<QSharedPointer<GpgPassphraseContext>> answered_;
  QList<QSharedPointer<GpgPassphraseContext>> close_requests_;
  QMetaObject::Connection connection_;
  QMetaObject::Connection close_connection_;
};

/// Outcome of one RequestPassphrase() call made on a worker thread.
struct RequestOutcome {
  QString passphrase;
  PassphraseRequestStatus status = PassphraseRequestStatus::kFailed;
  qint64 started_ms = 0;
  qint64 finished_ms = 0;
};

auto MakeState(const QString& info, const QString& fpr, bool retry = false)
    -> PassphraseState {
  PassphraseState state;
  state.info = info;
  state.fpr = fpr;
  state.retry = retry;
  return state;
}

/**
 * @brief Run RequestPassphrase() on the given runner and record what came back.
 */
void PostRequest(Thread::TaskRunnerGetter::TaskRunnerType runner_type,
                 const QString& name, const PassphraseState& state,
                 RequestOutcome* outcome, std::atomic<int>* remaining,
                 const QElapsedTimer* clock) {
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(runner_type)
      ->PostTask(
          name,
          [state, outcome, remaining, clock](const DataObjectPtr&) -> int {
            outcome->started_ms = clock->elapsed();
            PassphraseRequestStatus status = PassphraseRequestStatus::kFailed;
            auto pwd = PassphraseService::GetInstance(kPassphraseTestChannel)
                           .RequestPassphrase(state, &status);
            outcome->passphrase = QString::fromUtf8(pwd.ConvertToQByteArray());
            outcome->status = status;
            outcome->finished_ms = clock->elapsed();
            --*remaining;
            return 0;
          },
          nullptr, nullptr);
}

/**
 * @brief Pump the main thread until every posted request has returned.
 *
 * The main thread plays the UI here: it has to keep running an event loop for
 * the fake prompt to be reached at all.
 */
auto PumpUntilDone(const std::atomic<int>& remaining, int budget_ms) -> bool {
  QElapsedTimer deadline;
  deadline.start();
  while (remaining > 0 && deadline.elapsed() < budget_ms) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  }
  return remaining == 0;
}

/// Keep pumping for a while after the requests are done, so signals they
/// emitted on their way out still reach the main thread.
void Drain(int ms) {
  QElapsedTimer deadline;
  deadline.start();
  while (deadline.elapsed() < ms) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  }
}

/// Restores a process-global setting when it goes out of scope.
class ScopedSetting {
 public:
  ScopedSetting(const char* key, const QVariant& value) : key_(key) {
    auto settings = GetSettings();
    previous_ = settings.value(key_);
    settings.setValue(key_, value);
  }

  ~ScopedSetting() {
    auto settings = GetSettings();
    if (previous_.isValid()) {
      settings.setValue(key_, previous_);
    } else {
      settings.remove(key_);
    }
  }

 private:
  const char* key_;
  QVariant previous_;
};

/// Restores the prompt timeout setting, which is global to the process.
class ScopedPromptTimeout : public ScopedSetting {
 public:
  explicit ScopedPromptTimeout(int seconds)
      : ScopedSetting(kTimeoutSettingKey, seconds) {}
};

/// Restores the requester grace, which is global to the process.
///
/// The requester deliberately outlives the prompt's own countdown so the
/// ordinary outcome is "the prompt answered". Nothing runs that countdown in a
/// unit test, so the give-up path pays the whole grace as dead time -- five
/// seconds of it at the shipped default.
class ScopedRequesterGrace : public ScopedSetting {
 public:
  explicit ScopedRequesterGrace(int ms) : ScopedSetting(kGraceSettingKey, ms) {}
};

}  // namespace

// A fresh context says nothing about earlier tries, whatever memory it was
// made in: it comes from the secure allocator, whose recycled blocks once made
// a first prompt open with "the passphrase you entered was incorrect".
TEST_F(GFCoreTest, AFreshPassphraseContextCarriesNoEarlierState) {
  alignas(GpgPassphraseContext)
      std::array<unsigned char, sizeof(GpgPassphraseContext)>
          dirty;
  dirty.fill(0xFF);
  auto* ctx = new (dirty.data()) GpgPassphraseContext();
  EXPECT_FALSE(ctx->IsPreWasBad());
  EXPECT_FALSE(ctx->IsAskForNew());
  EXPECT_FALSE(ctx->ShouldConfirm());
  EXPECT_EQ(ctx->GetChannel(), -1);
  ctx->~GpgPassphraseContext();
}

// An interface answers a request one way: with what was typed, or as an
// explicit refusal; either way the request is settled once.
TEST_F(GFCoreTest, AnsweringAPassphraseRequestSettlesIt) {
  QObject receiver;
  int settled = 0;
  QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalUserInputPassphraseReady, &receiver,
      [&](const QSharedPointer<GpgPassphraseContext>&) { ++settled; },
      Qt::DirectConnection);

  auto typed = QSharedPointer<GpgPassphraseContext>::create();
  AnswerPassphraseRequest(typed, GFBuffer(QStringLiteral("secret")));
  EXPECT_EQ(typed->GetPassphrase(), GFBuffer(QStringLiteral("secret")));
  EXPECT_FALSE(typed->IsCancelled());

  auto declined = QSharedPointer<GpgPassphraseContext>::create();
  AnswerPassphraseRequest(declined, std::nullopt);
  EXPECT_TRUE(declined->GetPassphrase().Empty());
  EXPECT_TRUE(declined->IsCancelled());

  EXPECT_EQ(settled, 2);
}

/**
 * Two operations queued on the same runner must not overlap. Waiting for a
 * passphrase used to spin an event loop on the runner thread, which pumped that
 * runner's own queue and started the next operation inside the wait — the
 * second operation then asked for a passphrase that had not been cached yet and
 * a second prompt appeared on top of the first.
 */
TEST_F(GFCoreTest, PassphraseServiceDoesNotReenterTaskRunner) {
  ScopedPromptTimeout timeout(0);
  FakePassphrasePrompt prompt(200);

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(2);
  RequestOutcome first;
  RequestOutcome second;

  // Distinct info, so single-flight cannot collapse them: what is under test is
  // that the second task does not even start while the first one waits.
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_first",
              MakeState("Unlock key for signing #1", kFprA), &first, &remaining,
              &clock);
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_second",
              MakeState("Unlock key for signing #2", kFprA), &second,
              &remaining, &clock);

  ASSERT_TRUE(PumpUntilDone(remaining, 10000));

  ASSERT_EQ(prompt.MaxConcurrentPrompts(), 1);
  ASSERT_EQ(prompt.PromptCount(), 2);

  // Strict handover: the second task begins only after the first has returned.
  ASSERT_GE(second.started_ms, first.finished_ms);

  ASSERT_EQ(first.status, PassphraseRequestStatus::kProvided);
  ASSERT_EQ(second.status, PassphraseRequestStatus::kProvided);
  ASSERT_EQ(first.passphrase, QString("secret"));
  ASSERT_EQ(second.passphrase, QString("secret"));
}

/**
 * SignalUserInputPassphraseReady is broadcast to every waiting request, so a
 * request must consume only the answer to its own prompt. Without that check,
 * answering one prompt handed its passphrase to every other pending request and
 * left their prompts stranded on screen.
 */
TEST_F(GFCoreTest, PassphraseServiceIgnoresForeignAnswers) {
  ScopedPromptTimeout timeout(0);
  FakePassphrasePrompt prompt(300);

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(1);
  RequestOutcome outcome;

  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_identity",
              MakeState("Unlock key for signing", kFprA), &outcome, &remaining,
              &clock);

  // While the real prompt is open, answer a completely unrelated context.
  auto foreign = QSharedPointer<GpgPassphraseContext>::create();
  foreign->SetPassphrase(GFBuffer(QString("wrong-passphrase")));

  QElapsedTimer wait_for_prompt;
  wait_for_prompt.start();
  while (prompt.PromptCount() == 0 && wait_for_prompt.elapsed() < 5000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  }
  ASSERT_EQ(prompt.PromptCount(), 1);

  emit CoreSignalStation::GetInstance()
      -> SignalUserInputPassphraseReady(foreign);

  // The request must still be waiting for its own prompt, not finished with the
  // foreign passphrase.
  ASSERT_EQ(remaining.load(), 1);

  ASSERT_TRUE(PumpUntilDone(remaining, 10000));

  ASSERT_EQ(outcome.status, PassphraseRequestStatus::kProvided);
  ASSERT_EQ(outcome.passphrase, QString("secret"));
}

/**
 * Two threads needing the same unlock at the same time share one prompt.
 */
TEST_F(GFCoreTest, PassphraseServiceSharesOnePromptForTheSameKey) {
  ScopedPromptTimeout timeout(0);
  FakePassphrasePrompt prompt(400);

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(2);
  RequestOutcome from_gpg;
  RequestOutcome from_io;

  const auto state = MakeState("Unlock key for signing", kFprA);

  // Different runners, so these really do overlap; two tasks on one runner
  // would be serialised by the fix under test.
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_shared_gpg",
              state, &from_gpg, &remaining, &clock);
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_IO, "pp_shared_io",
              state, &from_io, &remaining, &clock);

  ASSERT_TRUE(PumpUntilDone(remaining, 10000));

  ASSERT_EQ(prompt.PromptCount(), 1);
  ASSERT_EQ(from_gpg.status, PassphraseRequestStatus::kProvided);
  ASSERT_EQ(from_io.status, PassphraseRequestStatus::kProvided);
  ASSERT_EQ(from_gpg.passphrase, QString("secret"));
  ASSERT_EQ(from_io.passphrase, QString("secret"));
}

/**
 * A retry means the last passphrase was rejected, so it must never be answered
 * from another request that is still on screen — the user has to see the
 * "incorrect passphrase" prompt and type again.
 */
TEST_F(GFCoreTest, PassphraseServiceNeverSharesARetryPrompt) {
  ScopedPromptTimeout timeout(0);
  FakePassphrasePrompt prompt(400);

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(2);
  RequestOutcome plain;
  RequestOutcome retry;

  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_plain",
              MakeState("Unlock key for signing", kFprA), &plain, &remaining,
              &clock);
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_IO, "pp_retry",
              MakeState("Unlock key for signing", kFprA, true), &retry,
              &remaining, &clock);

  ASSERT_TRUE(PumpUntilDone(remaining, 10000));

  ASSERT_EQ(prompt.PromptCount(), 2);
  ASSERT_EQ(plain.status, PassphraseRequestStatus::kProvided);
  ASSERT_EQ(retry.status, PassphraseRequestStatus::kProvided);
}

/**
 * A different key is a different prompt, even at the same moment.
 */
TEST_F(GFCoreTest, PassphraseServicePromptsPerKey) {
  ScopedPromptTimeout timeout(0);
  FakePassphrasePrompt prompt(400);

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(2);
  RequestOutcome key_a;
  RequestOutcome key_b;

  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_key_a",
              MakeState("Unlock key for signing", kFprA), &key_a, &remaining,
              &clock);
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_IO, "pp_key_b",
              MakeState("Unlock key for signing", kFprB), &key_b, &remaining,
              &clock);

  ASSERT_TRUE(PumpUntilDone(remaining, 10000));

  ASSERT_EQ(prompt.PromptCount(), 2);
}

/**
 * The configured timeout reaches the prompt, so the countdown it shows and the
 * deadline the operation waits on are the same number.
 */
TEST_F(GFCoreTest, PassphraseServiceStampsConfiguredTimeoutOnTheContext) {
  ScopedPromptTimeout timeout(45);
  FakePassphrasePrompt prompt(0);

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(1);
  RequestOutcome outcome;

  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_timeout_value",
              MakeState("Unlock key for signing", kFprA), &outcome, &remaining,
              &clock);

  ASSERT_TRUE(PumpUntilDone(remaining, 10000));

  ASSERT_EQ(prompt.Contexts().size(), 1);
  ASSERT_EQ(prompt.Contexts().front()->GetTimeoutSeconds(), 45);
}

/**
 * When nobody answers, the request gives up *and* dismisses its prompt. A
 * prompt that outlives its request is the frozen window this whole change is
 * about: still on screen, still holding the modal stack, no longer connected to
 * anything.
 */
TEST_F(GFCoreTest, PassphraseServiceClosesAnUnansweredPrompt) {
  ScopedPromptTimeout timeout(1);
  // Nothing here runs the prompt's countdown, so the whole grace is dead wait.
  // 200 ms still proves the requester waits past the prompt's own deadline
  // before giving up, which is the property; the shipped 5000 ms only made the
  // test the slowest in the suite.
  ScopedRequesterGrace grace(200);
  FakePassphrasePrompt prompt(0);
  prompt.StopAnswering();

  QElapsedTimer clock;
  clock.start();
  std::atomic<int> remaining(1);
  RequestOutcome outcome;

  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_timeout",
              MakeState("Unlock key for signing", kFprA), &outcome, &remaining,
              &clock);

  // 1 s countdown plus the requester's grace, with room to spare.
  ASSERT_TRUE(PumpUntilDone(remaining, 30000));

  // The close is emitted just before the request returns, so drain the queue
  // before looking for it.
  Drain(200);

  ASSERT_GE(outcome.finished_ms - outcome.started_ms, 1000);
  ASSERT_EQ(prompt.PromptCount(), 1);
  ASSERT_EQ(static_cast<int>(outcome.status),
            static_cast<int>(PassphraseRequestStatus::kFailed));
  ASSERT_EQ(prompt.CloseRequestCount(), 1);
  ASSERT_TRUE(outcome.passphrase.isEmpty());
}

/**
 * When the interface goes away, an operation waiting for a passphrase is let
 * go at once instead of holding its thread until the prompt times out (never,
 * at timeout 0); its prompt is told to close; a later request is refused
 * without asking; and after the core starts again, requests are asked again.
 */
TEST_F(GFCoreTest, ClosingPromptsReleasesWaitersAndRefusesNewRequests) {
  ScopedPromptTimeout timeout(0);
  // Prompts are process-global: reopen them whatever happens here, or every
  // later test in this process would be refused.
  struct Reopen {
    ~Reopen() { PassphraseService::ReopenPrompts(); }
  } reopen;

  QElapsedTimer clock;
  clock.start();

  {
    FakePassphrasePrompt prompt;
    prompt.StopAnswering();

    std::atomic<int> remaining = 1;
    RequestOutcome waiting;
    PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_close_wait",
                MakeState(u"close wait"_s, QString::fromLatin1(kFprA)),
                &waiting, &remaining, &clock);

    QElapsedTimer shown;
    shown.start();
    while (prompt.PromptCount() == 0 && shown.elapsed() < 5000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    ASSERT_EQ(prompt.PromptCount(), 1);

    PassphraseService::ClosePrompts();
    ASSERT_TRUE(PumpUntilDone(remaining, 5000));
    EXPECT_EQ(waiting.status, PassphraseRequestStatus::kCancelled);
    EXPECT_TRUE(waiting.passphrase.isEmpty());

    // The prompt was told to close; its answer, arriving after, is ignored.
    Drain(100);
    EXPECT_EQ(prompt.CloseRequestCount(), 1);

    remaining = 1;
    RequestOutcome refused;
    PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_refused",
                MakeState(u"refused"_s, QString::fromLatin1(kFprB)), &refused,
                &remaining, &clock);
    ASSERT_TRUE(PumpUntilDone(remaining, 5000));
    EXPECT_EQ(refused.status, PassphraseRequestStatus::kCancelled);
    EXPECT_EQ(prompt.PromptCount(), 1) << "a closed service asked anyway";
  }

  PassphraseService::ReopenPrompts();

  FakePassphrasePrompt prompt(50);
  std::atomic<int> remaining = 1;
  RequestOutcome asked;
  PostRequest(Thread::TaskRunnerGetter::kTaskRunnerType_GPG, "pp_reopened",
              MakeState(u"reopened"_s, QString::fromLatin1(kFprB)), &asked,
              &remaining, &clock);
  ASSERT_TRUE(PumpUntilDone(remaining, 5000));
  EXPECT_EQ(asked.status, PassphraseRequestStatus::kProvided);
  EXPECT_EQ(asked.passphrase, u"secret"_s);
}

// A task's callback runs on the application's thread, unless the thread that
// made it asked for it with DeliverCallbacksToThisThread, as a caller waiting
// in an event loop of its own does. It used to be queued to the QThread object
// of the constructing thread, which lives on whichever thread created that
// object, so where it ran was nobody's choice.
TEST_F(GFCoreTest, ATaskCallbackRunsWhereItWasAskedFor) {
  auto io = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
      Thread::TaskRunnerGetter::kTaskRunnerType_IO);

  auto called_on = std::make_shared<std::atomic<QThread*>>(nullptr);
  auto make = [called_on]() {
    return new Thread::Task([](const DataObjectPtr&) -> int { return 0; },
                            "callback_thread", TransferParams(),
                            [called_on](int, const DataObjectPtr&) {
                              called_on->store(QThread::currentThread());
                            });
  };

  // Asked for: waited for in a loop of this thread's own.
  {
    Thread::Task* task = nullptr;
    {
      const Thread::DeliverCallbacksToThisThread here;
      task = make();
    }
    QEventLoop loop;
    QObject::connect(task, &Thread::Task::SignalTaskEnd, &loop,
                     &QEventLoop::quit);
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    io->PostTask(task);
    loop.exec();
    EXPECT_EQ(called_on->load(), QThread::currentThread());
  }

  // Not asked for: the application's thread, waited for without a loop.
  called_on->store(nullptr);
  io->PostTask(make());
  for (int waited = 0; called_on->load() == nullptr && waited < 10000;
       ++waited) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  EXPECT_EQ(called_on->load(), QCoreApplication::instance()->thread());
}

}  // namespace GpgFrontend::Test
