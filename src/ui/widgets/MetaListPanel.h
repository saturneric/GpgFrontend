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

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief What a row is, structurally.
 */
// Deliberately untyped: lupdate miscounts braces when a *typed* enum is
// followed by a braced struct ahead of a Q_OBJECT class in the same namespace,
// and registers that class's tr() context without the namespace -- which loses
// every translation in it at runtime. MetaListRow below is exactly that struct.
// NOLINTNEXTLINE(performance-enum-size)
enum class MetaRowKind {
  kValue,    ///< a named value, the only kind that carries anything
  kSection,  ///< a heading that groups the rows under it
  kRule,     ///< a hairline, for the line above a total
};

/**
 * @brief One named value, and everything about how firmly it is known.
 *
 * A plain struct with no widgets in it, so the wording and the shape of a list
 * can be built and asserted by a test -- gtest bodies run off the GUI thread,
 * where constructing a widget is a crash. Every list in the application is
 * produced by a pure function returning these and rendered by the one panel
 * below.
 */
struct GF_UI_EXPORT MetaListRow {
  MetaRowKind kind = MetaRowKind::kValue;

  QString caption;  ///< the left column, or a section's heading
  QString value;    ///< the right column
  /// A sentence about the value. Shown under it when it must stay on screen and
  /// behind a marked tooltip otherwise; ShowsDetailInline() decides which.
  QString detail;
  QString icon;  ///< optional `:/icons/…`, drawn at 16px in a column of its own

  /// A size, right-aligned in a column of its own so sizes can be compared.
  /// Negative means the row has none, which is the usual case.
  qint64 bytes = -1;

  bool emphasis = false;  ///< bold: the file's own name, a total
  bool dimmed = false;    ///< muted: carries nothing, or was left out
  bool degraded = false;  ///< a fallback state rather than an intended one
  bool danger = false;    ///< a state that costs the user something
  /// The value came from somewhere this application cannot vouch for -- in
  /// practice, a package's unencrypted header. Never rendered without its
  /// caveat; see MetaListPanel.
  bool unverified = false;
  /// An unbreakable single token, such as a path: elided in the middle rather
  /// than at the end, and always readable in full on hover.
  bool path = false;

  bool checkable = false;  ///< the caption cell is a checkbox
  bool checked = false;

  /// A URL the value points at. Opened only when the reader clicks it; nothing
  /// is ever fetched to render the row.
  QString link;
};

/// Why a value marked unverified is only a claim. Rendered by the panel under
/// any list containing one, so a claim has no path to the screen that leaves
/// its caveat behind.
auto GF_UI_EXPORT UnverifiedRowsCaveat() -> QString;

/**
 * @brief Whether a list contains anything this application cannot vouch for.
 *
 * @param rows the rows
 * @return true when at least one row is unverified
 */
auto GF_UI_EXPORT MetaListNeedsCaveat(const QVector<MetaListRow>& rows) -> bool;

/**
 * @brief The same list as plain text, for the clipboard.
 *
 * Sections become headings, details are indented under their value, and the
 * caveat is included when the list carries a claim -- what is copied says the
 * same thing as what was read.
 *
 * @param rows the rows
 * @return the text
 */
auto GF_UI_EXPORT MetaListSummaryText(const QVector<MetaListRow>& rows)
    -> QString;

/**
 * @brief Adapt an info board card's fields to rows.
 *
 * The report document has to render its own cards -- it paints into a
 * QTextDocument rather than into widgets -- but the two need not disagree about
 * what a field is.
 *
 * @param fields caption/value pairs, as InfoBoardCard carries them
 * @return the same fields as rows
 */
auto GF_UI_EXPORT ToMetaListRows(
    const QContainer<QPair<QString, QString>>& fields) -> QVector<MetaListRow>;

/**
 * @brief The one key/value list in the application.
 *
 * Always used as the content of a CreateCard(), or dropped straight into a
 * dialog's column; never a container in its own right. It knows nothing about
 * what it is listing: callers hand it rows from a pure builder, which is what
 * keeps the wording testable and stops each dialog from growing its own
 * vocabulary for the same facts.
 */
class GF_UI_EXPORT MetaListPanel : public QWidget {
  Q_OBJECT

 public:
  explicit MetaListPanel(QWidget* parent = nullptr);

  /**
   * @brief Build the list.
   *
   * @param rows what to show
   */
  void SetRows(const QVector<MetaListRow>& rows);

  /**
   * @brief Update the values in place, keeping the widgets that are there.
   *
   * For a list that reacts to a control inside itself: rebuilding would destroy
   * the checkbox whose signal is being handled. Falls back to a rebuild when
   * the shape of the list has actually changed.
   *
   * @param rows the same rows, with new values
   */
  void RefreshValues(const QVector<MetaListRow>& rows);

  /// @brief The rows as last set.
  [[nodiscard]] auto Rows() const -> const QVector<MetaListRow>&;

 signals:
  /**
   * @brief A checkable row was ticked or unticked.
   *
   * @param index the row's index in the vector it was built from
   * @param checked its new state
   */
  void SignalRowToggled(int index, bool checked);

 private:
  /// @brief Drop every item and build the list again.
  void rebuild();

  /// @brief Fill one item from its row: text, colour, weight, hover.
  void apply_row(QTreeWidgetItem* item, const MetaListRow& row);

  /// @brief Size the caption column, capped at its share of the width.
  void fit_caption_column();

  /// @brief Ask for exactly the height the rows need.
  ///
  /// The list never scrolls inside itself: it sits in a card, on a page that
  /// scrolls as a whole, and a small scrolling box inside a scrolling page is
  /// how a reader loses track of which one they are in.
  void fit_to_contents();

  /// @brief Copy the selected rows, in the same words the list shows them.
  void copy_selection() const;

  QVector<MetaListRow> rows_;
  int caption_width_ = 0;  ///< what the captions want, before any capping
  QTreeWidget* tree_{};
  QLabel* caveat_{};
};

}  // namespace GpgFrontend::UI
