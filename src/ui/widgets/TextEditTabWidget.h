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

class PlainTextEditorPage;
class EMailEditorPage;
class FilePage;

class TextEditTabWidget : public QTabWidget {
  Q_OBJECT
 public:
  explicit TextEditTabWidget(QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @return PlainTextEditorPage*
   */
  [[nodiscard]] auto CurTextPage() const -> PlainTextEditorPage*;

  /**
   * @brief
   *
   * @return PlainTextEditorPage*
   */
  [[nodiscard]] auto CurEMailPage() const -> EMailEditorPage*;

  /**
   * @brief
   *
   * @return PlainTextEditorPage*
   */
  [[nodiscard]] auto CurPageTextEdit() const -> PlainTextEditorPage*;

  /**
   * @brief
   *
   * @return FilePage*
   */
  [[nodiscard]] auto CurFilePage() const -> FilePage*;

 public slots:

  /**
   * @brief
   *
   */
  void SlotNewTab();

  /**
   * @brief
   *
   */
  void SlotNewEMailTab();

  /**
   * @brief
   *
   * @param title
   * @param content
   */
  void SlotNewTabWithContent(QString title, const QString& content);

  /**
   * @details Adds a new tab with opening file by path
   */
  void SlotOpenFile(const QString& path);

  /**
   * @brief
   *
   * @param path
   */
  void SlotOpenEMLFile(const QString& path);

  /**
   * @brief
   *
   */
  void SlotOpenDirectory(const QString& target_directory);

  /**
   * @details put a * in front of current tabs title, if current textedit is
   * modified
   */
  void SlotShowModified();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void dragEnterEvent(QDragEnterEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void dropEvent(QDropEvent* event) override;

 private slots:
  /**
   * @brief
   *
   */
  void slot_save_status_to_cache_for_recovery();

  /**
   * @brief
   *
   * @param path
   */
  void slot_file_page_path_changed(const QString& path);

 private:
  int count_page_ = 0;
  int text_page_data_modified_count_ = 0;

  static auto stripped_name(const QString& full_file_name) -> QString;
};

}  // namespace GpgFrontend::UI