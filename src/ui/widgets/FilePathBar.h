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
 * @brief One segment of a breadcrumb path.
 */
struct PathSegment {
  QString label;  ///< Text shown on the crumb button.
  QString path;   ///< Absolute path the segment points at.
  bool home;      ///< Whether the segment is the home directory.
};

/**
 * @brief Path control of the file page.
 *
 * The bar has two faces sharing one slot in the toolbar:
 *
 * - a breadcrumb strip, which is what the user sees almost all of the time.
 *   Every path segment is a flat button, so going up several levels is one
 *   click instead of repeated use of the up button;
 * - a line editor with directory completion, for typing or pasting a path.
 *
 * The editor is reached by clicking the empty area after the last crumb, or
 * with Ctrl+L / F6, and is left again on Return, Escape or focus loss. The bar
 * never navigates by itself: it only reports the path the user asked for and
 * lets the owner decide whether that path is usable.
 */
class FilePathBar : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Constructs a path bar showing an empty path.
   *
   * @param parent Parent widget.
   */
  explicit FilePathBar(QWidget* parent = nullptr);

  /**
   * @brief Shows a directory as the current path.
   *
   * The breadcrumb strip is rebuilt and the editor text is reset to the
   * user-facing form of the path, with the home directory shortened to "~".
   *
   * @param path Absolute directory path.
   */
  void SetPath(const QString& path);

  /**
   * @brief Returns the path currently displayed.
   *
   * @return Absolute directory path, as last given to SetPath().
   */
  [[nodiscard]] auto GetPath() const -> QString;

  /**
   * @brief Switches to the line editor and selects its content.
   */
  void BeginEdit();

  /**
   * @brief Switches back to the breadcrumb strip, discarding any edit.
   */
  void EndEdit();

  /**
   * @brief Shows an error state for a path the owner rejected.
   *
   * The bar stays in edit mode with the offending text selected, so the user
   * can correct it directly.
   *
   * @param message Message shown as a tooltip next to the editor.
   */
  void ShowPathError(const QString& message);

 signals:
  /**
   * @brief Emitted when the user asks to open a path.
   *
   * The path is already normalized: "~" is expanded and relative input is
   * resolved against the current path. It is not guaranteed to exist; the
   * receiver is responsible for validating it.
   *
   * @param path Absolute path requested by the user.
   */
  void SignalPathRequested(const QString& path);

 protected:
  /**
   * @brief Enters edit mode when the empty area of the strip is clicked.
   */
  void mousePressEvent(QMouseEvent* event) override;

  /**
   * @brief Rebuilds the strip when the available width changes.
   */
  void resizeEvent(QResizeEvent* event) override;

  /**
   * @brief Handles Escape and focus loss of the editor.
   */
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;

 private:
  /// Width kept free after the last crumb, so there is always somewhere to
  /// click to reach the editor.
  static constexpr int kEditHitAreaWidth = 24;

  /// Fixed gutter each separator occupies between two crumbs.
  static constexpr int kChevronWidth = 14;

  /// Horizontal breathing room added to every crumb label.
  static constexpr int kCrumbPadding = 14;

  /// Edge length of the always-present button that opens the editor.
  static constexpr int kEditButtonSide = 24;

  QStackedLayout* stack_{};      ///< Switches between strip and editor.
  QWidget* crumb_page_{};        ///< Breadcrumb strip page.
  QHBoxLayout* crumb_layout_{};  ///< Layout holding the crumb buttons.
  QLineEdit* path_edit_{};       ///< Path editor page.

  QCompleter* path_edit_completer_{};        ///< Completer for path input.
  QStringListModel* path_complete_model_{};  ///< Model of the completer.

  QString current_path_;  ///< Absolute path currently displayed.

  /**
   * @brief Builds the breadcrumb strip for the current path.
   *
   * Segments are laid out from the root, or from the home directory when the
   * path is below it. When they do not fit, the leading ones collapse into a
   * single overflow button carrying them as a menu.
   */
  void rebuild_crumbs();

  /**
   * @brief Creates one crumb button.
   *
   * @param text Label of the segment.
   * @param path Absolute path the segment points at.
   * @param current Whether this is the last, non-clickable segment.
   * @return The button, parented to the crumb page.
   */
  auto make_crumb_button(const QString& text, const QString& path, bool current)
      -> QToolButton*;

  /**
   * @brief Creates one separator between two crumbs.
   *
   * @return The label, parented to the crumb page.
   */
  auto make_chevron() -> QLabel*;

  /**
   * @brief Creates the button that switches to the path editor.
   *
   * The button sits at the end of the strip and is always present: clicking
   * the empty area only works while a short path leaves some, and the
   * shortcut alone is not discoverable.
   *
   * @return The button, parented to the crumb page.
   */
  auto make_edit_button() -> QToolButton*;

  /**
   * @brief Width one separator occupies, used while measuring the strip.
   *
   * @return Separator width in pixels.
   */
  auto make_chevron_width() -> int;

  /**
   * @brief Creates the button collapsing the leading segments.
   *
   * @param segments All segments of the current path.
   * @param count Number of leading segments to put into its menu.
   * @return The button, parented to the crumb page.
   */
  auto make_overflow_button(const QContainer<PathSegment>& segments, int count)
      -> QToolButton*;

  /**
   * @brief Removes every widget from the crumb strip.
   */
  void clear_crumbs();

  /**
   * @brief Requests the path currently typed in the editor.
   */
  void commit_edit();

  /**
   * @brief Updates completion candidates for the path editor.
   *
   * Candidates are readable subdirectories of the resolved base directory, and
   * keep the style of the input: "~" paths stay abbreviated, relative input
   * stays relative.
   *
   * @param input Current text entered by the user.
   */
  void update_path_completion(const QString& input);
};

}  // namespace GpgFrontend::UI
