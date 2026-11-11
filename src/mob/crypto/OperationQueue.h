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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QPointer>
#include <QSet>
#include <deque>
#include <functional>
#include <optional>

#include "crypto/OperationLog.h"

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend::Mobile {

class AppLifecycle;
class PassphraseBroker;

/**
 * @brief Runs the Text and Files pages' operations one at a time, for the
 * whole application, and keeps their records (OperationLog).
 *
 * One at a time because a passphrase request names no operation
 * (GpgPassphraseContext): only while a single operation runs can the prompt
 * on screen be said to be that operation's. Text goes ahead of Files items
 * that are still waiting, but nothing that runs is ever interrupted.
 *
 * An operation is submitted with what starts it; the queue calls that when
 * its turn comes, and the page reports the end with Complete(). A page that
 * goes away takes what it still had waiting with it (ForgetOwner()).
 *
 * Prompts asked by anything else -- making a key, say -- reach the same
 * PassphraseBroker and are not told apart here.
 */
class OperationQueue : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(GpgFrontend::Mobile::OperationLog* log READ Log CONSTANT)
  Q_PROPERTY(int current READ CurrentId NOTIFY SignalCurrentChanged)
  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)

 public:
  /// Starts the operation @p id; it ends with Complete(@p id, ...), at
  /// once or later.
  using Runner = std::function<void(int id)>;

  struct Request {
    QObject* owner = nullptr;  ///< the page's object; required
    OperationSurface surface = OperationSurface::kTEXT;
    OperationParams params;
    QList<OperationTarget> targets;
    int batch_id = 0;  ///< 0: a batch of its own
    Runner run;
  };

  /// @p passphrases and @p lifecycle may be null.
  explicit OperationQueue(PassphraseBroker* passphrases,
                          AppLifecycle* lifecycle, QObject* parent = nullptr);
  ~OperationQueue() override;

  static void SetInstance(OperationQueue* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> OperationQueue*;
  /// The application's queue; null before and after AppServices.
  static auto Instance() -> OperationQueue*;

  [[nodiscard]] auto Log() -> OperationLog*;
  [[nodiscard]] auto CurrentId() const -> int;
  [[nodiscard]] auto Busy() const -> bool;

  /**
   * @brief Queue @p request; its id, or 0 when it was refused (closed, or
   * no owner or runner). It may have started, and even ended, by the time
   * this returns.
   */
  auto Submit(Request request) -> int;

  /// The running operation @p id ended in @p phase (done, failed or
  /// cancelled); the next starts.
  void Complete(int id, OperationPhase phase, const QVariantMap& report,
                const QList<int>& outputs = {}, const QString& reason = {});

  /**
   * @brief Why the running operation @p id ended in @p phase when no engine
   * says: "background" for one whose prompt was declined by leaving the
   * application; empty otherwise. For a page that records the end before
   * Complete() (OperationLog::SetFinished()), so the record says it at once.
   */
  [[nodiscard]] auto EndReason(int id, OperationPhase phase) const -> QString;

  /// Take the waiting operation @p id out of the queue, as cancelled;
  /// false when it is not waiting (running, ended or unknown).
  auto Cancel(int id, const QString& reason) -> bool;

  /// End everything @p owner has here, waiting or running, as cancelled
  /// ("closed"). Owners call it in their destructor, before they are gone.
  void ForgetOwner(QObject* owner);

  /// Cancel what waits and refuse what comes; for quitting.
  void Close();

 signals:
  void SignalCurrentChanged();
  void SignalBusyChanged();
  void SignalStarted(int id);
  /// @p id ended, however it did: run to its end, or cancelled waiting.
  void SignalFinished(int id);

 private:
  struct Item {
    int id = 0;
    QObject* owner = nullptr;
    OperationSurface surface = OperationSurface::kTEXT;
    Runner run;
  };

  QPointer<PassphraseBroker> passphrases_;
  OperationLog log_;
  std::deque<Item> queue_;
  std::optional<Item> running_;
  QSet<QObject*> owners_;
  int next_id_ = 1;
  bool starting_ = false;
  bool closed_ = false;
  bool was_busy_ = false;
  /// The running operation was in the background at some point.
  bool suspended_while_running_ = false;

  void start_next();
  void cancel_item(const Item& item, const QString& reason);
  void prompt_changed();
  void busy_changed();
};

}  // namespace GpgFrontend::Mobile
