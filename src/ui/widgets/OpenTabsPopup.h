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

class TextEdit;

/**
 * @brief Every open tab in one searchable list.
 *
 * Opens with the search field focused and the current tab selected; an empty
 * search lists the tabs most recently used first. Closing an entry goes
 * through TextEdit::CloseTabs() and leaves the list open.
 *
 * A frameless tool window rather than a Qt::Popup, so the save prompt closing
 * an unsaved entry can open without dismissing it.
 */
class GF_UI_EXPORT OpenTabsPopup : public QFrame {
  Q_OBJECT
 public:
  explicit OpenTabsPopup(TextEdit* edit);

  /// Show the list, positioned below the end of the tab strip.
  void Open();

  /// The ids listed now, top to bottom.
  [[nodiscard]] auto ListedIds() const -> QList<quint64>;

  /// The id of the selected entry; 0 for none.
  [[nodiscard]] auto SelectedId() const -> quint64;

  /// Filter the list by @p query, as typing it into the search field does.
  void SetQuery(const QString& query);

  /// Close the selected entry's tab, keeping the list open.
  void CloseSelected();

  /// Switch to the selected entry's tab and hide the list.
  void ActivateSelected();

 protected:
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;
  void changeEvent(QEvent* event) override;

 private:
  TextEdit* edit_;
  QLineEdit* search_ = nullptr;
  QListWidget* list_ = nullptr;
  QLabel* no_match_ = nullptr;
  QPointer<QWidget> restore_focus_;
  bool closing_entry_ = false;

  /// Fill the list for the current query, selecting @p select (or, when it
  /// is not listed, the row at @p fallback_row).
  void populate(quint64 select, int fallback_row);
  void place();
  void dismiss();
  void close_row(int row);
  /// A key in the search field; true when it was the list's to handle.
  auto handle_search_key(QKeyEvent* key) -> bool;
  /// A mouse event on the list; true when consumed.
  auto handle_list_event(QEvent* event) -> bool;
};

}  // namespace GpgFrontend::UI
