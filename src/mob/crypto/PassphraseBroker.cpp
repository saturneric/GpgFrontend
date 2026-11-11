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

#include "crypto/PassphraseBroker.h"

#include <QGuiApplication>
#include <QJSEngine>
#include <QWindow>
#include <atomic>

#include "core/function/CoreSignalStation.h"
#include "core/function/openpgp/PassphraseService.h"
#include "core/model/GpgPassphraseContext.h"
#include "crypto/OperationQueue.h"
#include "plat/common/PlatformPrivacy.h"
#include "shell/rules/PassphraseEntryRules.h"

namespace GpgFrontend::Mobile {

namespace {

QPointer<PassphraseBroker> g_instance;

}  // namespace

PassphraseBroker::PassphraseBroker(QObject* parent)
    : QObject(parent), countdown_(new QTimer(this)) {
  countdown_->setSingleShot(true);
  connect(countdown_, &QTimer::timeout, this, &PassphraseBroker::cancel);
  // Restarted for whichever request is on screen now.
  connect(this, &PassphraseBroker::SignalCurrentChanged, this, [this]() {
    const auto front = pending_.empty() ? QSharedPointer<GpgPassphraseContext>()
                                        : pending_.front();
    if (front == counting_for_) return;  // one queued behind it: same clock
    counting_for_ = front;
    countdown_->stop();
    if (front == nullptr) return;
    const auto seconds = front->GetTimeoutSeconds();
    if (seconds > 0) countdown_->start(std::chrono::seconds(seconds));
  });

  auto* station = CoreSignalStation::GetInstance();
  connect(station, &CoreSignalStation::SignalNeedUserInputPassphrase, this,
          &PassphraseBroker::on_need, Qt::QueuedConnection);
  connect(station, &CoreSignalStation::SignalCloseUserInputPassphrase, this,
          &PassphraseBroker::on_close, Qt::QueuedConnection);
}

PassphraseBroker::~PassphraseBroker() { Close(); }

void PassphraseBroker::SetInstance(PassphraseBroker* instance) {
  g_instance = instance;
}

auto PassphraseBroker::create(QQmlEngine*, QJSEngine*) -> PassphraseBroker* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

auto PassphraseBroker::Current() const -> QVariantMap {
  if (pending_.empty()) return {};
  const auto& context = pending_.front();
  const auto key = context->GetKey();
  // The operation it is for: the one running, as only one runs at a time
  // (OperationQueue). A prompt from outside the queue -- making a key --
  // cannot be told apart, and names whatever runs meanwhile, if anything.
  auto* queue = OperationQueue::Instance();
  const auto operation = queue != nullptr ? queue->CurrentId() : 0;
  const auto* record = operation != 0 ? queue->Log()->Find(operation) : nullptr;
  const auto name = record != nullptr ? record->params.op : QString();
  return {
      {QStringLiteral("key"), key != nullptr ? key->UID() : QString()},
      {QStringLiteral("keyId"), key != nullptr ? key->ID() : QString()},
      {QStringLiteral("info"), context->GetPassphraseInfo()},
      {QStringLiteral("retry"), context->IsPreWasBad()},
      {QStringLiteral("askForNew"), context->IsAskForNew()},
      {QStringLiteral("confirm"), context->ShouldConfirm()},
      {QStringLiteral("timeout"), context->GetTimeoutSeconds()},
      {QStringLiteral("operationId"), operation},
      {QStringLiteral("operation"), name},
  };
}

void PassphraseBroker::submit(const QString& passphrase) {
  answer_front(&passphrase);
}

namespace {
std::atomic<int> g_declined{0};
}  // namespace

void PassphraseBroker::cancel() {
  if (!pending_.empty()) ++g_declined;
  answer_front(nullptr);
}

auto PassphraseBroker::Declined() -> int { return g_declined.load(); }

void PassphraseBroker::SetSuspended(bool suspended) {
  suspended_ = suspended;
  if (suspended) CancelAll();
}

auto PassphraseBroker::entryProblem(const QString& passphrase,
                                    const QString& confirmation,
                                    bool confirm) const -> QString {
  return Shell::DescribePassphraseEntryProblem(
             Shell::CheckPassphraseEntry(passphrase, confirmation, confirm))
      .text;
}

void PassphraseBroker::CancelAll() {
  while (!pending_.empty()) answer_front(nullptr);
}

void PassphraseBroker::Close() {
  closed_ = true;
  CancelAll();
  // What is still queued to this thread will never be delivered once its
  // loop has stopped; the core settles those itself.
  PassphraseService::ClosePrompts();
}

void PassphraseBroker::on_need(
    const QSharedPointer<GpgPassphraseContext>& context) {
  if (context == nullptr) return;
  pending_.push_back(context);
  // Asked while the application is away: declined, as one on screen was
  // when it left. Nobody is there to answer, and it would come up later
  // for an operation whose moment has passed.
  if (closed_ || suspended_) {
    answer_front(nullptr);
    return;
  }
  update_secure_window();
  emit SignalCurrentChanged();
}

void PassphraseBroker::on_close(
    const QSharedPointer<GpgPassphraseContext>& context) {
  // Withdrawn by the core: whoever asked has stopped waiting, so it is not
  // answered.
  for (auto it = pending_.begin(); it != pending_.end(); ++it) {
    if (*it != context) continue;
    const bool shown = it == pending_.begin();
    pending_.erase(it);
    if (shown) emit SignalWipeInput();
    update_secure_window();
    emit SignalCurrentChanged();
    return;
  }
}

void PassphraseBroker::answer_front(const QString* passphrase) {
  if (pending_.empty()) return;
  auto context = pending_.front();
  pending_.pop_front();

  emit SignalWipeInput();
  update_secure_window();
  emit SignalCurrentChanged();
  AnswerPassphraseRequest(context, passphrase != nullptr
                                       ? std::optional(GFBuffer(*passphrase))
                                       : std::nullopt);
}

void PassphraseBroker::update_secure_window() {
  if (pending_.empty()) {
    secure_.reset();
    return;
  }
  if (secure_ == nullptr) {
    secure_ = std::make_unique<SecureWindowScope>(PrimaryWindow());
  }
}

}  // namespace GpgFrontend::Mobile
