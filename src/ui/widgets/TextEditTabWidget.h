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

#include "core/model/GFBuffer.h"

namespace GpgFrontend::UI {

class PlainTextEditorPage;
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
  [[nodiscard]] auto CurPageTextEdit() const -> PlainTextEditorPage*;

  /**
   * @brief
   *
   * @return FilePage*
   */
  [[nodiscard]] auto CurFilePage() const -> FilePage*;

  /**
   * @brief
   *
   * @return QWidget*
   */
  [[nodiscard]] auto CurPage() -> QWidget*;

  /**
   * @brief
   *
   */
  void InitTabStyle();

  /**
   * @brief
   *
   * @param page
   * @param modified
   */
  void UpdateTabModifiedMark(QWidget* page, bool modified);

  /**
   * @brief Create a Plain Text Tab object
   *
   * @param title
   * @param file_path
   * @param icon
   * @return PlainTextEditorPage*
   */
  auto CreatePlainTextTab(const QString& title, const QString& file_path,
                          const QIcon& icon) -> PlainTextEditorPage*;

  /**
   * @brief
   *
   * @param path
   * @return int
   */
  [[nodiscard]] auto FindTabByFilePath(const QString& path) const -> int;

  /**
   * @brief
   *
   * @param url
   */
  void OpenDroppedUrl(const QUrl& url);

  /**
   * @brief
   *
   * @param path
   * @return QString
   */
  auto CompactPathForTab(const QString& path) -> QString;

  /**
   * @brief
   *
   * @param path
   * @return QString
   */
  auto WorkspaceTitleFromPath(const QString& path) -> QString;

  /**
   * @brief
   *
   * @param page
   * @param path
   */
  void UpdateFilePageTabTitle(QWidget* page, const QString& path);

  /**
   * @brief
   *
   * @param title
   * @return QString
   */
  auto NormalizeTabTitle(QString title) -> QString;

  /**
   * @brief
   *
   * @param path
   * @return int
   */
  [[nodiscard]] auto FindFilePageByPath(const QString& path) const -> int;

 public slots:

  /**
   * @brief
   *
   * @param type
   * @param icon
   * @param title
   */
  // NOLINTNEXTLINE
  QWidget* SlotNewTab(const QString& type, const QString& title,
                      const QIcon& icon);

  /**
   * @brief
   *
   * @return QWidget*
   */
  QWidget* SlotNewPlainTextTab();  // NOLINT

  /**
   * @brief
   *
   * @param title
   * @param content
   */
  void SlotNewTabWithContent(QString title, const QString& content);

  /**
   * @brief
   *
   * @param title
   * @param buffer
   */
  void SlotNewTabWithGFBuffer(QString title, const GFBuffer& buffer);

  /**
   * @brief
   *
   */
  void SlotOpenDefaultPath();

  /**
   * @details Adds a new tab with opening file by path
   */
  void SlotOpenFile(const QString& path);

  /**
   * @brief
   *
   */
  void SlotOpenPath(const QString& target_path);

  /**
   * @details put a * in front of current tabs title, if current textedit is
   * modified
   */
  void SlotShowModified(bool changed);

  /**
   * @brief
   *
   */
  void SlotCacheTextEditors();

  /**
   * @brief
   *
   */
  void SlotRestoreTextEditorsCache();

  /**
   * @brief
   *
   * @param file_info
   * @param error_message
   * @return true
   * @return false
   */
  // NOLINTNEXTLINE
  bool CanOpenAsTextFile(const QFileInfo& file_info,
                         QString* error_message) const;

  /**
   * @brief
   *
   * @param index
   * @return QString
   */
  // NOLINTNEXTLINE
  QString PathForTab(int index) const;

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

 private:
  int count_page_ = 0;
  int text_page_data_modified_count_ = 0;

  /**
   * @brief
   *
   * @param full_file_name
   * @return QString
   */
  static auto stripped_name(const QString& full_file_name) -> QString;

  /**
   * @brief
   *
   * @param prefix
   * @return QString
   */
  auto generate_new_title(const QString& prefix, const QString& suffix)
      -> QString;
};

}  // namespace GpgFrontend::UI