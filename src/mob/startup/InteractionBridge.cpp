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

#include "startup/InteractionBridge.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QThread>

namespace GpgFrontend::Mobile {

namespace {

QPointer<InteractionBridge> g_instance;

auto KindName(InteractionBridge::Kind kind) -> QString {
  switch (kind) {
    case InteractionBridge::Kind::kAPP_KEY_PIN:
      return QStringLiteral("app_key_pin");
    case InteractionBridge::Kind::kCONFIRM_KEY_RESET:
      return QStringLiteral("confirm_key_reset");
    case InteractionBridge::Kind::kCONFIRM_FORCE_UNLOCK:
      return QStringLiteral("confirm_force_unlock");
  }
  return {};
}

}  // namespace

InteractionBridge::InteractionBridge(QObject* parent) : QObject(parent) {}

InteractionBridge::~InteractionBridge() { CancelAll(); }

void InteractionBridge::SetInstance(InteractionBridge* instance) {
  g_instance = instance;
}

auto InteractionBridge::create(QQmlEngine*, QJSEngine*) -> InteractionBridge* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

auto InteractionBridge::Ask(Kind kind, const QVariantMap& details) -> QVariant {
  if (QThread::currentThread() == thread()) {
    qWarning() << "a question asked on the interface thread cannot be waited "
                  "for; answering it safely";
    return {};
  }

  std::future<QVariant> answer;
  int id = 0;
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (cancelled_) return {};
    id = next_id_++;
    pending_.push_back(Pending{id, kind, details, {}});
    answer = pending_.back().answer.get_future();
  }

  notify_changed();
  QMetaObject::invokeMethod(
      this, [this, id, kind]() { emit SignalPending(id, kind); },
      Qt::QueuedConnection);
  return answer.get();
}

void InteractionBridge::CancelAll() {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    cancelled_ = true;
    for (auto& pending : pending_) pending.answer.set_value({});
    pending_.clear();
  }
  notify_changed();
}

auto InteractionBridge::Cancelled() const -> bool {
  const std::lock_guard<std::mutex> lock(mutex_);
  return cancelled_;
}

auto InteractionBridge::Current() const -> QVariantMap {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (pending_.empty()) return {};

  const auto& front = pending_.front();
  auto current = front.details;
  current.insert(QStringLiteral("id"), front.id);
  current.insert(QStringLiteral("kind"), KindName(front.kind));
  return current;
}

void InteractionBridge::answer(int id, const QVariant& value) {
  resolve(id, value);
}

void InteractionBridge::decline(int id) { resolve(id, {}); }

void InteractionBridge::WipeSecretInputs() {
  QMetaObject::invokeMethod(
      this, [this]() { emit SignalWipeSecretInputs(); }, Qt::QueuedConnection);
}

void InteractionBridge::resolve(int id, const QVariant& value) {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = pending_.begin(); it != pending_.end(); ++it) {
      if (it->id != id) continue;
      it->answer.set_value(value);
      pending_.erase(it);
      break;
    }
  }
  notify_changed();
}

void InteractionBridge::notify_changed() {
  QMetaObject::invokeMethod(
      this, [this]() { emit SignalCurrentChanged(); }, Qt::QueuedConnection);
}

}  // namespace GpgFrontend::Mobile
