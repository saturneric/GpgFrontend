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

#include "PassphraseService.h"

#include <utility>

#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

namespace {

// Seconds a prompt may stay unanswered before it gives up. 0 disables the
// countdown entirely and the prompt waits for the user indefinitely.
constexpr int kDefaultPromptTimeoutSeconds = 60;

// The prompt owns the countdown and rejects itself when it expires; its answer
// then travels back as a queued signal. The requester waits a little longer
// than the prompt does so the ordinary outcome is always "the prompt answered",
// never "the requester walked away from a prompt that was about to answer".
// Only if this grace elapses too does the requester force the prompt closed.
constexpr int kRequesterGraceMs = 5000;

// Settable so a test can drive the give-up path without paying five seconds of
// dead wait for it; unset -- which is every real run -- keeps the constant.
auto ReadRequesterGraceMs() -> int {
  auto grace =
      GetSettings()
          .value("engine/passphrase_requester_grace_ms", kRequesterGraceMs)
          .toInt();
  return grace > 0 ? grace : kRequesterGraceMs;
}

auto ReadPromptTimeoutSeconds() -> int {
  auto timeout = GetSettings()
                     .value("engine/passphrase_prompt_timeout",
                            kDefaultPromptTimeoutSeconds)
                     .toInt();
  return timeout > 0 ? timeout : 0;
}

/**
 * @brief The answer slot shared between the thread that needs a passphrase and
 * the UI thread that collects it.
 *
 * Refcounted on purpose: the answering slot runs on the UI thread and may fire
 * after the requester has already given up, so it must never write through a
 * pointer into the requester's stack frame.
 */
class PassphraseRequest {
 public:
  /**
   * @brief Record the outcome and wake everyone waiting on it. The first answer
   * wins; anything arriving later is stale and ignored.
   *
   * @return true if this call is the one that settled the request. Callers that
   * need to act only when they won -- rather than test-then-set, which is a
   * race across two statements -- branch on this.
   */
  auto Settle(const GFBuffer& passphrase, PassphraseRequestStatus status)
      -> bool {
    QMutexLocker locker(&mutex_);
    if (settled_) return false;
    passphrase_ = passphrase;
    status_ = status;
    settled_ = true;
    settled_cv_.wakeAll();
    return true;
  }

  /**
   * @brief Block the calling thread until the request is settled.
   *
   * Deliberately not a QEventLoop: this runs on a task-runner thread, and an
   * event loop here would pump that runner's own queue and start the next
   * queued operation inside this wait.
   *
   * @param timeout_ms milliseconds to wait; <= 0 waits indefinitely
   * @return true if the request was settled, false if the deadline passed
   */
  auto WaitUntilSettled(int timeout_ms) -> bool {
    QMutexLocker locker(&mutex_);
    if (timeout_ms <= 0) {
      while (!settled_) settled_cv_.wait(&mutex_);
      return true;
    }

    QDeadlineTimer deadline(timeout_ms);
    while (!settled_ && !deadline.hasExpired()) {
      settled_cv_.wait(&mutex_, deadline);
    }
    return settled_;
  }

  auto IsSettled() -> bool {
    QMutexLocker locker(&mutex_);
    return settled_;
  }

  auto Passphrase() -> GFBuffer {
    QMutexLocker locker(&mutex_);
    return passphrase_;
  }

  auto Status() -> PassphraseRequestStatus {
    QMutexLocker locker(&mutex_);
    return status_;
  }

 private:
  QMutex mutex_;
  QWaitCondition settled_cv_;
  bool settled_ = false;
  GFBuffer passphrase_;
  PassphraseRequestStatus status_ = PassphraseRequestStatus::kFailed;
};

using PassphraseRequestPtr = QSharedPointer<PassphraseRequest>;

// Identifies a prompt that is currently on screen. This is the same tuple the
// rPGP engine caches passphrases under, so "the same unlock, asked for twice at
// once" collapses into a single prompt here exactly as it collapses into a
// single cache entry there.
struct InFlightKey {
  int channel = -1;
  QString fpr;
  QString info;

  auto operator==(const InFlightKey& other) const -> bool {
    return channel == other.channel && fpr == other.fpr && info == other.info;
  }
};

auto qHash(const InFlightKey& key, uint seed = 0) -> uint {
  return ::qHash(key.channel, seed) ^ ::qHash(key.fpr, seed) ^
         ::qHash(key.info, seed);
}

QMutex g_in_flight_mutex;
QHash<InFlightKey, PassphraseRequestPtr> g_in_flight;

/**
 * @brief Wait for the request on the GUI thread, where blocking outright would
 * deadlock: the prompt is shown by this very thread, so it has to keep running
 * an event loop to reach it.
 *
 * No core code should ask for a passphrase from the GUI thread — the operation
 * belongs on a task runner — but a module or a future call site might, and a
 * frozen application is a far worse outcome than a nested loop.
 */
void WaitOnGuiThread(const PassphraseRequestPtr& request, int timeout_ms) {
  if (request->IsSettled()) return;

  LOG_W() << "passphrase requested on the GUI thread; falling back to a nested "
             "event loop. Crypto operations belong on a task runner.";

  QEventLoop loop;
  QTimer deadline;
  deadline.setSingleShot(true);
  QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);

  auto connection = QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalUserInputPassphraseReady, &loop,
      [&loop, request](const QSharedPointer<GpgPassphraseContext>&) {
        if (request->IsSettled()) loop.quit();
      });

  if (timeout_ms > 0) deadline.start(timeout_ms);
  if (!request->IsSettled()) loop.exec();

  QObject::disconnect(connection);
}

/**
 * @brief Block until the request is settled, on whichever thread we are.
 */
void WaitForRequest(const PassphraseRequestPtr& request, int wait_ms) {
  if (QCoreApplication::instance() != nullptr &&
      QThread::currentThread() == QCoreApplication::instance()->thread()) {
    WaitOnGuiThread(request, wait_ms);
    return;
  }
  request->WaitUntilSettled(wait_ms);
}

/**
 * @brief Join the prompt already on screen for this key, or claim the right to
 * open one.
 *
 * @return the request to wait on, and whether this caller owns its prompt
 */
auto AcquireRequest(const InFlightKey& key, bool shareable)
    -> std::pair<PassphraseRequestPtr, bool> {
  if (!shareable) return {PassphraseRequestPtr::create(), true};

  QMutexLocker locker(&g_in_flight_mutex);

  auto it = g_in_flight.find(key);
  if (it != g_in_flight.end() && !(*it)->IsSettled()) return {*it, false};

  auto request = PassphraseRequestPtr::create();
  g_in_flight.insert(key, request);
  return {request, true};
}

void ReleaseRequest(const InFlightKey& key,
                    const PassphraseRequestPtr& request) {
  QMutexLocker locker(&g_in_flight_mutex);
  if (g_in_flight.value(key) == request) g_in_flight.remove(key);
}

/**
 * @brief Ask the UI for a passphrase and wait for the answer that belongs to
 * this request.
 */
void DrivePrompt(const PassphraseRequestPtr& request,
                 const QSharedPointer<GpgPassphraseContext>& c, int wait_ms) {
  // Direct connection: the answer is emitted on the UI thread and is recorded
  // there, so the waiting thread needs no event loop to receive it. Running one
  // there is precisely what broke batch operations — the nested loop pumped the
  // worker's own task queue, starting the next file's task inside the current
  // file's passphrase wait, which asked for a passphrase that had not been
  // cached yet and so opened a second prompt.
  auto connection = QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalUserInputPassphraseReady,
      CoreSignalStation::GetInstance(),
      [request, c](const QSharedPointer<GpgPassphraseContext>& ctx) -> void {
        // The signal is a broadcast to every waiting request; only the one it
        // belongs to may consume it. Without this check, answering a single
        // prompt hands that passphrase to every other request at once and
        // abandons their prompts on screen.
        if (ctx != c) return;
        request->Settle(ctx->GetPassphrase(),
                        ctx->IsCancelled()
                            ? PassphraseRequestStatus::kCancelled
                            : PassphraseRequestStatus::kProvided);
      },
      Qt::DirectConnection);

  emit CoreSignalStation::GetInstance() -> SignalNeedUserInputPassphrase(c);

  WaitForRequest(request, wait_ms);

  // Settle first, then dismiss -- never the other way round, and never behind
  // an IsSettled() check.
  //
  // SignalCloseUserInputPassphrase is queued to the UI thread, so it returns
  // here immediately having only posted an event. The UI thread then answers
  // the close by cancelling the context, which runs the direct-connected lambda
  // above and stamps kCancelled. If the dismissal were emitted before this
  // Settle, that whole sequence could land in the gap between the two
  // statements and win -- reporting "the user cancelled" for a prompt no user
  // ever saw. Under load that is not theoretical: it was observed on 14 of 32
  // runs with 8 test processes competing for 12 cores.
  //
  // Settle is first-wins and reports whether it won, so this is a single atomic
  // decision: a prompt that genuinely answered in the meantime keeps its
  // answer, and only the requester that actually gave up sends the dismissal.
  if (request->Settle(GFBuffer(), PassphraseRequestStatus::kFailed)) {
    // The prompt outlived its own countdown. Dismiss it, so it cannot linger on
    // screen holding the modal stack with nobody waiting for its answer.
    emit CoreSignalStation::GetInstance() -> SignalCloseUserInputPassphrase(c);
  }

  QObject::disconnect(connection);
}

}  // namespace

PassphraseService::PassphraseService(int channel)
    : SingletonFunctionObject(channel) {}

auto PassphraseService::RequestPassphrase(const PassphraseState& state,
                                          PassphraseRequestStatus* out_status)
    -> GFBuffer {
  // Default to a non-cancellation failure; only the explicit paths below
  // upgrade this to provided or cancelled.
  if (out_status != nullptr) *out_status = PassphraseRequestStatus::kFailed;

  GpgAbstractKeyPtr key = nullptr;

  if (!state.ask_for_new && state.info.trimmed().isEmpty()) {
    LOG_W()
        << "Passphrase request with empty fingerprint and not asking for new "
           "passphrase. This may lead to incorrect key association.";
    return {};
  }

  auto t_state = state;  // Make a mutable copy of the state
  t_state.fpr = t_state.fpr.trimmed().toUpper();

  if (!t_state.fpr.isEmpty()) {
    key = AbstractKeyRepository::GetInstance(GetChannel()).GetKey(t_state.fpr);
    if (key == nullptr) {
      LOG_W()
          << "No key found for empty fingerprint. This may lead to incorrect "
             "passphrase association.";
    }
  }

  // A prompt that has to be answered on its own terms is never shared: setting
  // a new passphrase, confirming one, and retrying after a rejection each need
  // the user in front of that specific prompt.
  const auto shareable = !t_state.fpr.isEmpty() && !t_state.retry &&
                         !t_state.ask_for_new && !t_state.should_confirm;
  const InFlightKey in_flight_key{GetChannel(), t_state.fpr, t_state.info};

  const auto [request, owns_prompt] = AcquireRequest(in_flight_key, shareable);

  const auto timeout_seconds = ReadPromptTimeoutSeconds();
  const auto wait_ms = timeout_seconds > 0
                           ? (timeout_seconds * 1000) + ReadRequesterGraceMs()
                           : 0;

  if (owns_prompt) {
    auto c = QSharedPointer<GpgPassphraseContext>::create(GetChannel(), key);
    c->SetPassphraseInfo(t_state.info);
    c->SetPrevWasBad(t_state.retry);
    c->SetAskForNew(t_state.ask_for_new);
    c->SetShouldConfirm(t_state.should_confirm);
    c->SetTimeoutSeconds(timeout_seconds);

    DrivePrompt(request, c, wait_ms);
    if (shareable) ReleaseRequest(in_flight_key, request);
  } else {
    // Someone else is already asking the user for this exact unlock; their
    // answer is ours too.
    WaitForRequest(request, wait_ms);
  }

  if (out_status != nullptr) *out_status = request->Status();
  return request->Passphrase();
}

}  // namespace GpgFrontend
