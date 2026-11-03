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
#include <QVariant>
#include <deque>
#include <future>
#include <mutex>

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend::Mobile {

/**
 * @brief Questions a worker thread needs a person to answer.
 *
 * The profile loader asks for a PIN or a confirmation from the thread opening
 * the profile, and waits for the answer. Ask() publishes the question to the
 * interface and blocks only that thread; the interface thread never waits.
 * QML shows current, one question at a time in the order asked, and answers
 * with answer() or decline().
 *
 * CancelAll() gives every pending question, and every later one, its safe
 * answer -- the one that changes nothing -- so a shutdown never waits on a
 * person.
 */
class InteractionBridge : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  /// The question on screen: {id, kind, ...details}; empty when there is none.
  Q_PROPERTY(QVariantMap current READ Current NOTIFY SignalCurrentChanged)

 public:
  enum class Kind {
    kAPP_KEY_PIN,          ///< {creating, failures} -> {action, pin}
    kCONFIRM_KEY_RESET,    ///< {reason} -> bool
    kCONFIRM_FORCE_UNLOCK  ///< {pid, host, application} -> bool
  };
  Q_ENUM(Kind)

  // Deliberately no default constructor: given one, QML constructs a
  // singleton of its own and never calls create(), so the interface would
  // talk to a different object than the application.
  explicit InteractionBridge(QObject* parent);
  ~InteractionBridge() override;

  /// The instance QML is given; set by whoever creates it.
  static void SetInstance(InteractionBridge* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> InteractionBridge*;

  /**
   * @brief Ask and wait for the answer. Never call on the interface thread.
   *
   * @return the answer, or an invalid QVariant once cancelled (and when called
   * on the interface thread, where waiting would deadlock)
   */
  auto Ask(Kind kind, const QVariantMap& details) -> QVariant;

  /// Answer every pending and later question safely. Any thread.
  void CancelAll();

  /// Whether CancelAll() has run.
  [[nodiscard]] auto Cancelled() const -> bool;

  [[nodiscard]] auto Current() const -> QVariantMap;

  /// Answer the question @p id.
  Q_INVOKABLE void answer(int id, const QVariant& value);

  /// Answer the question @p id with its safe answer.
  Q_INVOKABLE void decline(int id);

  /// Ask whatever shows a secret being typed to forget it (going to the
  /// background); the question itself stays.
  void WipeSecretInputs();

 signals:
  void SignalCurrentChanged();
  void SignalPending(int id, GpgFrontend::Mobile::InteractionBridge::Kind kind);
  void SignalWipeSecretInputs();

 private:
  struct Pending {
    int id;
    Kind kind;
    QVariantMap details;
    std::promise<QVariant> answer;
  };

  mutable std::mutex mutex_;
  std::deque<Pending> pending_;
  int next_id_ = 1;
  bool cancelled_ = false;

  void resolve(int id, const QVariant& value);
  void notify_changed();
};

}  // namespace GpgFrontend::Mobile
