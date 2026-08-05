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

namespace GpgFrontend::UI {

class GF_UI_EXPORT GpgFrontendApplication : public QApplication {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new GpgFrontend Application object
   *
   * @param argc
   * @param argv
   */
  explicit GpgFrontendApplication(int &argc, char *argv[]);

  /**
   * @brief Destroy the GpgFrontend Application object
   *
   */
  ~GpgFrontendApplication() override = default;

  /**
   * @brief Take the first profile package the system has handed over, if any.
   *
   * Destructive on purpose: the queue is the only record that a document is
   * outstanding, so whoever takes one has taken it and nobody else can act on
   * it again. Two very different callers race for it — the profile resolver
   * during startup, and the main window once it exists — and this is what stops
   * both opening the same file.
   *
   * Anything that is not a package is left in the queue rather than dropped, so
   * registering a second document type later does not silently break.
   *
   * @return the path, empty when there is nothing to take
   */
  auto TakePendingProfilePackage() -> QString;

  /**
   * @brief Give a document handed over at launch a moment to arrive.
   *
   * macOS does not put a double-clicked file on the command line: it sends an
   * open-documents Apple Event, which is only dispatched once the Cocoa loop is
   * pumped. The profile has to be chosen *before* that loop starts — it decides
   * where the settings live — so the loop is pumped here, briefly, and bounded
   * twice over: a hard ceiling, and an early exit once it has gone quiet,
   * because the overwhelmingly common launch carries no document at all and
   * must not pay for one.
   *
   * Missing the window is survivable rather than fatal: the event lands in the
   * queue a moment later and the main window opens the package in a second
   * window, so the user gets one window they did not ask for instead of a file
   * that silently did nothing.
   *
   * Does nothing and returns false anywhere but macOS.
   *
   * @param timeout_ms the longest this may delay startup
   * @return whether something arrived
   */
  auto WaitForLaunchDocument(int timeout_ms) -> bool;

 signals:
  /**
   * @brief Something was handed over and is waiting in the queue.
   *
   * Carries no path on purpose: the queue is authoritative, and a handler that
   * trusted an argument could act on a document the resolver had already taken.
   */
  void SignalDocumentPending();

 protected:
  /**
   * @brief
   *
   * @param event
   * @return bool
   */
  bool notify(QObject *receiver, QEvent *event) override;

  /**
   * @brief Catch the documents Launch Services hands over.
   *
   * @param event
   * @return bool
   */
  bool event(QEvent *event) override;

 private:
  QStringList pending_documents_;  ///< handed over, not yet acted on
};

}  // namespace GpgFrontend::UI
