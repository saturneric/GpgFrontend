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

/**
 * @brief What the editor shows once its last tab is closed: a quiet welcome
 *        with a few ways to start again, and nothing else.
 */
class GF_UI_EXPORT EditorEmptyView : public QWidget {
  Q_OBJECT
 public:
  explicit EditorEmptyView(QWidget* parent = nullptr);

  /// The ways to start (New Text, a module's Note Editor...), asked each time
  /// the view is shown, so actions a module adds later appear too. Without
  /// one, the view offers New Text itself.
  void SetStartActions(std::function<QList<QAction*>()> provider);

 signals:
  void SignalNewTextRequested();
  void SignalOpenFileRequested();

 protected:
  void showEvent(QShowEvent* event) override;
  void changeEvent(QEvent* event) override;

 private:
  QVBoxLayout* rows_ = nullptr;
  QLabel* logo_ = nullptr;
  QLabel* title_ = nullptr;
  QLabel* hint_ = nullptr;
  std::function<QList<QAction*>()> start_actions_;

  void rebuild();
  void apply_palette();
};

}  // namespace GpgFrontend::UI
