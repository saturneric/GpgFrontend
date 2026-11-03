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

#include <QObject>
#include <QHash>
#include <functional>

/**
 * @file PlatformShare.h
 * @brief Handing text to another application through the system share sheet.
 *
 * Two things are reported, and they are not the same:
 * - whether a target was chosen. Android says so through the chooser's
 *   callback (EXTRA_CHOSEN_COMPONENT). It means a person picked an app, not
 *   that the app received or read anything.
 * - when the round trip through the share sheet is over: the application is
 *   in the foreground again. Android reports no result for ACTION_SEND that
 *   could be relied on, so a request still open then is finished as handed
 *   off when a target was chosen and as dismissed otherwise -- "not
 *   confirmed", which may be a cancel or a target that did not report.
 *
 * Every request completes exactly once, on the interface thread.
 */

namespace GpgFrontend {

enum class ShareOutcome {
  kHandedOff,    ///< a target was chosen
  kDismissed,    ///< back without a chosen target; not confirmed either way
  kFailed,       ///< the share sheet could not be shown
  kUnsupported,  ///< this platform cannot share; offer another way
};

struct ShareResult {
  ShareOutcome outcome = ShareOutcome::kUnsupported;
  bool target_chosen = false;
  QString detail;
};

using ShareCallback = std::function<void(const ShareResult&)>;

/// Whether this platform has a share sheet.
auto CanShare() -> bool;

/// Share @p text; @p done is called once, on the interface thread.
void ShareText(const QString& text, const QString& title, ShareCallback done);

/**
 * @brief Where files to be shared are kept: <cache>/share.
 *
 * Only files under it can be shared (on Android it is the one directory the
 * FileProvider exposes), each in a directory of its own.
 */
auto SharedFilesRoot() -> QString;

/**
 * @brief Share the file at @p path, which is under SharedFilesRoot(), as
 * @p mime_type; @p done is called once, on the interface thread.
 *
 * The receiving application is granted read access to that one file, until
 * RevokeSharedFile().
 */
void ShareFile(const QString& path, const QString& mime_type,
               const QString& title, ShareCallback done);

/// Withdraw every grant to read @p path. Harmless when there was none.
void RevokeSharedFile(const QString& path);

/**
 * @brief The open share requests and their one completion each.
 *
 * Platform backends report into it; it decides when a request is over.
 */
class ShareSession : public QObject {
  Q_OBJECT

 public:
  static auto Instance() -> ShareSession&;

  /// How long after coming back a request waits for a late "target chosen".
  static constexpr int kReconcileGraceMs = 750;

  /// How long a share sheet may take to appear. Even a split screen makes the
  /// application inactive while the sheet is up, so a request that never saw
  /// that was never shown: it ends as failed rather than staying open.
  static constexpr int kNeverShownMs = 30000;

  /// A new request; its id is what the backend reports with.
  auto Begin(ShareCallback done, int never_shown_ms = kNeverShownMs) -> int;

  /// The platform says a target was chosen for @p id. Any thread.
  void TargetChosen(int id);

  /// The share sheet could not be shown. Any thread.
  void Failed(int id, const QString& detail);

  /// Follow the application's state; a request is reconciled once the
  /// application has left and come back.
  void OnApplicationStateChanged(Qt::ApplicationState state);

  /// Open requests, for tests.
  [[nodiscard]] auto OpenCount() const -> int;

 private:
  struct Request {
    ShareCallback done;
    bool target_chosen = false;
    bool left = false;
  };

  QHash<int, Request> open_;
  int next_id_ = 1;
  bool active_ = true;

  ShareSession();
  void complete(int id, ShareOutcome outcome, const QString& detail = {});
  void reconcile();
};

}  // namespace GpgFrontend
