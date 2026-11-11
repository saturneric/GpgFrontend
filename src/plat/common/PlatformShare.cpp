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

#include "plat/common/PlatformShare.h"

#include <QGuiApplication>
#include <QStandardPaths>
#include <QTimer>

namespace GpgFrontend {

auto ShareSession::Instance() -> ShareSession& {
  static auto* session = new ShareSession();
  return *session;
}

auto SharedFilesRoot() -> QString {
  return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
         QStringLiteral("/share");
}

ShareSession::ShareSession() {
  moveToThread(QCoreApplication::instance()->thread());
  if (auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
      app != nullptr) {
    connect(app, &QGuiApplication::applicationStateChanged, this,
            &ShareSession::OnApplicationStateChanged);
  }
}

auto ShareSession::Begin(ShareCallback done, int never_shown_ms) -> int {
  const auto id = next_id_++;
  open_.insert(id, Request{std::move(done)});
  QTimer::singleShot(never_shown_ms, this, [this, id]() {
    auto it = open_.find(id);
    // Shown: the application left, or a target was chosen (a split screen
    // can keep it active throughout).
    if (it == open_.end() || it->left || it->target_chosen) return;
    complete(id, ShareOutcome::kFailed,
             QStringLiteral("the share sheet never appeared"));
  });
  return id;
}

void ShareSession::TargetChosen(int id) {
  QMetaObject::invokeMethod(
      this,
      [this, id]() {
        auto it = open_.find(id);
        if (it == open_.end()) return;
        it->target_chosen = true;
        // Chosen while the application never left (a split screen): no
        // return will come to conclude it, and the target reads now.
        if (active_ && !it->left) complete(id, ShareOutcome::kHandedOff);
      },
      Qt::QueuedConnection);
}

void ShareSession::Failed(int id, const QString& detail) {
  QMetaObject::invokeMethod(
      this, [this, id, detail]() { complete(id, ShareOutcome::kFailed, detail); },
      Qt::QueuedConnection);
}

void ShareSession::OnApplicationStateChanged(Qt::ApplicationState state) {
  active_ = state == Qt::ApplicationActive;
  if (!active_) {
    for (auto& request : open_) request.left = true;
    return;
  }
  QTimer::singleShot(kReconcileGraceMs, this, &ShareSession::reconcile);
}

auto ShareSession::OpenCount() const -> int {
  return static_cast<int>(open_.size());
}

void ShareSession::reconcile() {
  // Gone again before the grace ran out: the share sheet may be back on
  // screen, so nothing is concluded until the next return.
  if (!active_) return;
  for (const auto id : open_.keys()) {
    const auto& request = open_.value(id);
    if (!request.left) continue;  // not shown yet, or still on screen
    complete(id, request.target_chosen ? ShareOutcome::kHandedOff
                                       : ShareOutcome::kDismissed);
  }
}

void ShareSession::complete(int id, ShareOutcome outcome,
                            const QString& detail) {
  auto it = open_.find(id);
  if (it == open_.end()) return;  // already completed: once only
  auto request = *it;
  open_.erase(it);
  if (request.done) {
    request.done(ShareResult{outcome, request.target_chosen, detail});
  }
}

}  // namespace GpgFrontend
