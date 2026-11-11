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

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend::Mobile {

class InteractionBridge;
class PassphraseBroker;

/**
 * @brief What the application does when it goes to the background and back.
 *
 * Suspended -- the activity stopped, by the Home button, the lock screen, or
 * our own file picker or share sheet covering it -- always declines every
 * passphrase prompt (its operation ends as cancelled), forgets every cached
 * passphrase and wipes any secret being typed. Nothing is exempt: a
 * process in the background may be killed without another word, and the
 * request that was open, a picker for instance, keeps its own state and
 * finishes on its own once the application is back. A question still waiting
 * for an answer (a PIN at startup) stays; nothing destructive is answered on
 * anyone's behalf.
 *
 * Inactive -- a notification shade, a permission dialog -- changes nothing.
 */
class AppLifecycle : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(bool suspended READ IsSuspended NOTIFY SignalSuspendedChanged)
  /// The system's font scale (SystemFontScale()), read again on each
  /// return: the setting can only change while the application is away.
  Q_PROPERTY(qreal fontScale READ FontScale NOTIFY SignalFontScaleChanged)

 public:
  /// @p passphrases may be null: no passphrase prompts to cancel.
  explicit AppLifecycle(InteractionBridge* bridge,
                        PassphraseBroker* passphrases = nullptr,
                        QObject* parent = nullptr);

  static void SetInstance(AppLifecycle* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> AppLifecycle*;

  /// Follow the application's state; called with every change.
  void OnApplicationStateChanged(Qt::ApplicationState state);

  /**
   * @brief Leave the screen with everything as it is: what was typed, the
   * files made and not saved, operations under way. What Back from the
   * first screen does, where the platform can (SendToBackground()).
   * @return false where it cannot; the window then closes.
   */
  Q_INVOKABLE bool toBackground();

  [[nodiscard]] auto IsSuspended() const -> bool;
  [[nodiscard]] auto FontScale() const -> qreal;

 signals:
  void SignalSuspendedChanged();
  void SignalFontScaleChanged();
  /// Going to the background: the prompts on screen are about to be
  /// declined, cached passphrases about to be dropped.
  void SignalSuspending();
  /// Went to the background; cached passphrases are already gone.
  void SignalSuspended();
  /// Back in the foreground.
  void SignalResumed();

 private:
  InteractionBridge* bridge_;
  PassphraseBroker* passphrases_;
  bool suspended_ = false;
  qreal font_scale_ = 1.0;

  void read_font_scale();
};

}  // namespace GpgFrontend::Mobile
