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

#include "plat/common/PlatformClipboard.h"

#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QGuiApplication>
#include <QPointer>
#include <QTimer>

namespace GpgFrontend {

namespace {

// What the last sensitive copy put on the clipboard, by digest only, and when
// it may be cleared. Interface thread only, like the clipboard.
struct SensitiveCopy {
  QByteArray digest;
  QDateTime due;
  QPointer<QTimer> timer;
};

auto State() -> SensitiveCopy& {
  static SensitiveCopy state;
  return state;
}

auto Digest(const QString& text) -> QByteArray {
  return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
}

}  // namespace

void CopyText(const QString& text) {
  QGuiApplication::clipboard()->setText(text);
}

void CopySensitiveText(const QString& text, std::chrono::seconds expiry) {
  if (!SetSensitiveClipboardText(text)) {
    QGuiApplication::clipboard()->setText(text);
  }

  auto& state = State();
  state.digest = Digest(text);
  state.due = QDateTime::currentDateTimeUtc().addSecs(expiry.count());

  if (state.timer == nullptr) {
    state.timer = new QTimer(QGuiApplication::instance());
    state.timer->setSingleShot(true);
    state.timer->setTimerType(Qt::PreciseTimer);
    QObject::connect(state.timer, &QTimer::timeout,
                     []() { ExpireSensitiveCopy(); });
  }
  state.timer->start(std::chrono::milliseconds(expiry));
}

void ExpireSensitiveCopy(bool now_or_never) {
  auto& state = State();
  if (state.digest.isEmpty()) return;
  if (!now_or_never) {
    const auto left = QDateTime::currentDateTimeUtc().msecsTo(state.due);
    if (left > 0) {
      // Early (a timer may fire a little ahead of time): wait out the rest.
      if (state.timer != nullptr) state.timer->start(left);
      return;
    }
  }

  auto* clipboard = QGuiApplication::clipboard();
  const auto text = clipboard->text();

  // Android 10 and later read an app without the focus an empty clipboard:
  // judged from that, the copy would look replaced and be forgotten, never
  // cleared. Such a read is checked again in a moment, or on the return to
  // the foreground -- except at quit, when there is no later.
  if (!now_or_never && text.isEmpty() &&
      QGuiApplication::applicationState() != Qt::ApplicationActive) {
    if (state.timer != nullptr) state.timer->start(std::chrono::seconds(5));
    return;
  }

  if (Digest(text) == state.digest) clipboard->clear();

  state.digest.clear();
  if (state.timer != nullptr) state.timer->stop();
}

}  // namespace GpgFrontend
