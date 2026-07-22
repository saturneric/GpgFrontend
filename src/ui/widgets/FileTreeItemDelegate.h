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
 * @brief Item delegate giving the file tree its OpenPGP-aware look.
 *
 * The delegate does two things the plain view cannot:
 *
 * - it marks OpenPGP output with a small painted badge on the file icon, so an
 *   encrypted file or a detached signature is recognizable without reading the
 *   suffix;
 * - it renders the metadata columns in a muted colour, so the name column
 *   leads the row instead of competing with size, type and date.
 *
 * Everything is derived from the widget palette, so the result follows the
 * active light or dark theme without a stylesheet.
 */
class FileTreeItemDelegate : public QStyledItemDelegate {
  Q_OBJECT

 public:
  /**
   * @brief Constructs the delegate.
   *
   * @param parent Parent object, usually the file tree view.
   */
  explicit FileTreeItemDelegate(QObject* parent = nullptr);

  /**
   * @brief Paints one cell.
   *
   * @param painter Painter of the view viewport.
   * @param option Style option prepared by the view.
   * @param index Index of the cell.
   */
  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;

  /**
   * @brief Returns the size of one cell.
   *
   * The height is lifted a little above the default so rows breathe and the
   * badge has room next to the icon.
   *
   * @param option Style option prepared by the view.
   * @param index Index of the cell.
   * @return Cell size.
   */
  [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option,
                              const QModelIndex& index) const -> QSize override;

  /**
   * @brief Explains the badge of an OpenPGP file as a tooltip.
   *
   * The badge is abbreviated by necessity, so hovering the row spells out what
   * it means.
   *
   * @param event Help event carrying the position.
   * @param view View asking for the tooltip.
   * @param option Style option prepared by the view.
   * @param index Index of the cell.
   * @return true if the event was handled.
   */
  auto helpEvent(QHelpEvent* event, QAbstractItemView* view,
                 const QStyleOptionViewItem& option, const QModelIndex& index)
      -> bool override;

 private:
  /**
   * @brief What an entry is, as far as the badge is concerned.
   */
  enum class BadgeKind : uint8_t {
    kNONE,       ///< Not OpenPGP output; no badge.
    kENCRYPTED,  ///< Encrypted or armored message file.
    kSIGNATURE,  ///< Detached signature file.
  };

  /// Extra height added to every row, split above and below the content.
  static constexpr int kRowPadding = 12;

  /// Height no row goes below, whatever the font and icon ask for.
  static constexpr int kMinimumRowHeight = 30;

  /// How far the metadata columns are faded towards the row background.
  static constexpr double kMutedTextRatio = 0.5;

  /// Gap between the end of the file name and its badge.
  static constexpr int kBadgeTextGap = 8;

  /// Horizontal space between the badge text and the chip edge.
  static constexpr int kBadgePaddingX = 5;

  /// Extra chip height on top of the badge text height.
  static constexpr int kBadgePaddingY = 2;

  /// Corner radius of the chip.
  static constexpr int kBadgeRadius = 4;

  /// Badge text size relative to the row font.
  static constexpr double kBadgeFontRatio = 0.75;

  /// Point size the badge text never goes below.
  static constexpr double kBadgeMinPointSize = 6.5;

  /**
   * @brief Classifies an entry for badging purposes.
   *
   * @param info Entry to inspect.
   * @return Which badge the entry deserves, if any.
   */
  static auto openpgp_badge_kind(const QFileInfo& info) -> BadgeKind;

  /**
   * @brief Paints the OpenPGP badge after the name of a name cell.
   *
   * The badge trails the file name instead of sitting on the file icon,
   * because a platform without an installed icon theme leaves the decoration
   * rect empty and a badge anchored there would never appear.
   *
   * @param painter Painter of the view viewport.
   * @param option Style option already initialized for the cell.
   * @param info File the cell stands for.
   */
  static void paint_openpgp_badge(QPainter* painter,
                                  const QStyleOptionViewItem& option,
                                  const QFileInfo& info);

  /**
   * @brief Fades the text colours of a style option towards the background.
   *
   * Only the inactive and normal groups are touched; a selected row keeps the
   * full contrast of the highlight colours.
   *
   * @param option Style option to adjust in place.
   */
  static void mute_text_colors(QStyleOptionViewItem& option);
};

}  // namespace GpgFrontend::UI
