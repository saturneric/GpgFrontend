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
#include <QSharedPointer>
#include <QTimer>
#include <QVariantMap>
#include <deque>
#include <memory>

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend {
class GpgPassphraseContext;
class SecureWindowScope;
}  // namespace GpgFrontend

namespace GpgFrontend::Mobile {

/**
 * @brief Answers the core's passphrase requests from the passphrase sheet.
 *
 * The mobile counterpart of the desktop's prompt handler: the core asks with
 * SignalNeedUserInputPassphrase, from the worker running the operation, and
 * blocks that worker until SignalUserInputPassphraseReady carries the same
 * request back. Here the request waits in a queue while the sheet is shown;
 * the interface thread never blocks. A request the core withdraws (its
 * requester timed out) leaves the queue unanswered.
 *
 * While a request is shown the window is kept out of screenshots and the
 * recent-apps preview (SecureWindowScope).
 */
class PassphraseBroker : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  /// {key, keyId, info, retry, askForNew, confirm, timeout (seconds, 0:
  /// none), operationId and operation (the OperationQueue's running one;
  /// 0 and empty when none runs)}; empty when nothing is asked.
  Q_PROPERTY(QVariantMap current READ Current NOTIFY SignalCurrentChanged)

 public:
  // No default constructor: QML would make its own singleton (see
  // InteractionBridge).
  explicit PassphraseBroker(QObject* parent);
  ~PassphraseBroker() override;

  static void SetInstance(PassphraseBroker* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> PassphraseBroker*;

  [[nodiscard]] auto Current() const -> QVariantMap;

  /// Answer the request on screen with @p passphrase.
  Q_INVOKABLE void submit(const QString& passphrase);

  /// Decline the request on screen; the operation ends as cancelled.
  Q_INVOKABLE void cancel();

  /// How many prompts the person has declined so far (cancel()), in every
  /// broker: what an engine call that only says "no" can be told apart by.
  [[nodiscard]] static auto Declined() -> int;

  /// The application went to the background (@p suspended) or came back:
  /// while away, every prompt on screen and every new one is declined.
  void SetSuspended(bool suspended);

  /// Why @p passphrase (typed again as @p confirmation when a new one is
  /// chosen) cannot be sent yet, in the desktop's words; empty when it can
  /// (Shell::CheckPassphraseEntry).
  [[nodiscard]] Q_INVOKABLE QString entryProblem(const QString& passphrase,
                                                 const QString& confirmation,
                                                 bool confirm) const;

  /// Decline every pending request (going to the background).
  void CancelAll();

  /// Decline every pending request and every later one (quitting).
  void Close();

 signals:
  void SignalCurrentChanged();
  /// Whatever shows the passphrase being typed should clear it.
  void SignalWipeInput();

 private:
  std::deque<QSharedPointer<GpgPassphraseContext>> pending_;
  std::unique_ptr<SecureWindowScope> secure_;
  bool suspended_ = false;
  /// The request on screen ends as cancelled at its timeout, as the desktop's
  /// prompt does -- before the core stops waiting and withdraws it, which
  /// would read as a failure rather than as nobody answering.
  QTimer* countdown_;
  QSharedPointer<GpgPassphraseContext> counting_for_;
  bool closed_ = false;

  void on_need(const QSharedPointer<GpgPassphraseContext>& context);
  void on_close(const QSharedPointer<GpgPassphraseContext>& context);
  void answer_front(const QString* passphrase);
  void update_secure_window();
};

}  // namespace GpgFrontend::Mobile
