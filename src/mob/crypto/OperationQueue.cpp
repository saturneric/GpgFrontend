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

#include "crypto/OperationQueue.h"

#include <QJSEngine>
#include <algorithm>
#include <vector>

#include "crypto/PassphraseBroker.h"
#include "lifecycle/AppLifecycle.h"

namespace GpgFrontend::Mobile {

namespace {

QPointer<OperationQueue> g_instance;

auto Cancelled() -> QVariantMap {
  return {{QStringLiteral("cancelled"), true}};
}

}  // namespace

OperationQueue::OperationQueue(PassphraseBroker* passphrases,
                               AppLifecycle* lifecycle, QObject* parent)
    : QObject(parent), passphrases_(passphrases) {
  if (passphrases != nullptr) {
    connect(passphrases, &PassphraseBroker::SignalCurrentChanged, this,
            &OperationQueue::prompt_changed);
  }
  if (lifecycle != nullptr) {
    // A prompt declined by going to the background ends its operation as
    // cancelled; the record says why.
    connect(lifecycle, &AppLifecycle::SignalSuspended, this, [this]() {
      if (running_.has_value()) suspended_while_running_ = true;
    });
  }
}

OperationQueue::~OperationQueue() {
  for (auto* owner : std::as_const(owners_)) {
    disconnect(owner, nullptr, this, nullptr);
  }
}

void OperationQueue::SetInstance(OperationQueue* instance) {
  g_instance = instance;
}

auto OperationQueue::create(QQmlEngine*, QJSEngine*) -> OperationQueue* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

auto OperationQueue::Instance() -> OperationQueue* { return g_instance; }

auto OperationQueue::Log() -> OperationLog* { return &log_; }

auto OperationQueue::CurrentId() const -> int {
  return running_.has_value() ? running_->id : 0;
}

auto OperationQueue::Busy() const -> bool {
  return running_.has_value() || !queue_.empty();
}

auto OperationQueue::Submit(Request request) -> int {
  if (closed_ || request.owner == nullptr || !request.run) return 0;

  OperationRecord record;
  record.id = next_id_++;
  record.batch_id = request.batch_id > 0 ? request.batch_id : record.id;
  record.surface = request.surface;
  record.params = std::move(request.params);
  record.targets = std::move(request.targets);
  record.submitted = QDateTime::currentDateTimeUtc();
  const auto id = record.id;
  log_.Add(std::move(record));

  // A page that goes away without saying so still takes its operations.
  if (!owners_.contains(request.owner)) {
    owners_.insert(request.owner);
    connect(request.owner, &QObject::destroyed, this, [this](QObject* owner) {
      owners_.remove(owner);
      ForgetOwner(owner);
    });
  }

  Item item{id, request.owner, request.surface, std::move(request.run)};
  // Text before the files still waiting: a message is quick, a batch is not.
  auto at = queue_.end();
  if (item.surface == OperationSurface::kTEXT) {
    at = std::find_if(queue_.begin(), queue_.end(), [](const Item& waiting) {
      return waiting.surface == OperationSurface::kFILES;
    });
  }
  queue_.insert(at, std::move(item));
  start_next();
  busy_changed();
  return id;
}

void OperationQueue::Complete(int id, OperationPhase phase,
                              const QVariantMap& report,
                              const QList<int>& outputs,
                              const QString& reason) {
  if (!running_.has_value() || running_->id != id) return;
  auto why = reason;
  if (why.isEmpty() && phase == OperationPhase::kCANCELLED &&
      suspended_while_running_) {
    why = QStringLiteral("background");
  }
  running_.reset();
  suspended_while_running_ = false;
  log_.SetFinished(id, phase, why, report, outputs);
  emit SignalCurrentChanged();
  emit SignalFinished(id);
  start_next();
  busy_changed();
}

auto OperationQueue::Cancel(int id, const QString& reason) -> bool {
  const auto it =
      std::find_if(queue_.begin(), queue_.end(),
                   [id](const Item& item) { return item.id == id; });
  if (it == queue_.end()) return false;
  const auto item = *it;
  queue_.erase(it);
  cancel_item(item, reason);
  busy_changed();
  return true;
}

void OperationQueue::ForgetOwner(QObject* owner) {
  std::vector<Item> gone;
  for (auto it = queue_.begin(); it != queue_.end();) {
    if (it->owner == owner) {
      gone.push_back(*it);
      it = queue_.erase(it);
    } else {
      ++it;
    }
  }
  for (const auto& item : gone) cancel_item(item, QStringLiteral("closed"));
  // What it was running can no longer report its end; whatever the engine
  // still does for it is not waited for.
  if (running_.has_value() && running_->owner == owner) {
    Complete(running_->id, OperationPhase::kCANCELLED, Cancelled(), {},
             QStringLiteral("closed"));
  }
  busy_changed();
}

void OperationQueue::Close() {
  closed_ = true;
  while (!queue_.empty()) {
    const auto item = queue_.front();
    queue_.pop_front();
    cancel_item(item, QStringLiteral("closed"));
  }
  busy_changed();
}

void OperationQueue::start_next() {
  // Started from inside a runner that ended at once: the loop below goes on.
  if (starting_) return;
  starting_ = true;
  while (!running_.has_value() && !queue_.empty()) {
    running_ = queue_.front();
    queue_.pop_front();
    suspended_while_running_ = false;
    const auto id = running_->id;
    const auto run = running_->run;
    log_.SetStarted(id);
    emit SignalCurrentChanged();
    emit SignalStarted(id);
    run(id);
    // A prompt already up belongs to it.
    prompt_changed();
  }
  starting_ = false;
}

void OperationQueue::cancel_item(const Item& item, const QString& reason) {
  log_.SetFinished(item.id, OperationPhase::kCANCELLED, reason, Cancelled(),
                   {});
  emit SignalFinished(item.id);
}

void OperationQueue::prompt_changed() {
  if (!running_.has_value()) return;
  const auto asking =
      passphrases_ != nullptr && !passphrases_->Current().isEmpty();
  log_.SetPhase(running_->id, asking ? OperationPhase::kWAITING_PASSPHRASE
                                     : OperationPhase::kRUNNING);
}

void OperationQueue::busy_changed() {
  if (Busy() == was_busy_) return;
  was_busy_ = Busy();
  emit SignalBusyChanged();
}

}  // namespace GpgFrontend::Mobile
