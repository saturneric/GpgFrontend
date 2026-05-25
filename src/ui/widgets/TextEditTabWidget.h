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
#include "core/typedef/CoreTypedef.h"

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
                      const QIcon& icon, const QString& icon_name);

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
   */
  void SlotRestoreTextEditorsCacheNow();

  /**
   * @brief
   *
   */
  void SlotTabClosedForRecovery();

  /**
   * @brief
   *
   */
  void SlotRefreshRecoveryCache();

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

 private:
  int count_page_ = 0;
  int text_page_data_modified_count_ = 0;

  QTimer* recovery_cache_timer_ = nullptr;
  QList<QPointer<PlainTextEditorPage>> recovery_dirty_pages_;
  QPointer<PlainTextEditorPage> last_current_text_page_;
  bool recovery_restoring_ = false;
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

  /**
   * @brief
   *
   */
  void init_tab_style();

  /**
   * @brief
   *
   * @param page
   * @param modified
   */
  void update_tab_modified_mark(QWidget* page, bool modified);

  /**
   * @brief Create a Plain Text Tab object
   *
   * @param title
   * @param file_path
   * @param icon
   * @return PlainTextEditorPage*
   */
  auto create_plain_text_tab(const QString& title, const QString& file_path,
                             const QIcon& icon, const QString& icon_name)
      -> PlainTextEditorPage*;

  /**
   * @brief
   *
   * @param path
   * @return int
   */
  [[nodiscard]] auto find_tab_by_file_path(const QString& path) const -> int;

  /**
   * @brief
   *
   * @param url
   */
  void open_dropped_url(const QUrl& url);

  /**
   * @brief
   *
   * @param path
   * @return QString
   */
  auto compact_path_for_tab(const QString& path) -> QString;

  /**
   * @brief
   *
   * @param path
   * @return QString
   */
  auto workspace_title_from_path(const QString& path) -> QString;

  /**
   * @brief
   *
   * @param page
   * @param path
   */
  void update_file_page_tab_title(QWidget* page, const QString& path);

  /**
   * @brief
   *
   * @param path
   * @return int
   */
  [[nodiscard]] auto find_file_page_by_path(const QString& path) const -> int;

  /**
   * @brief
   *
   * @param file_info
   * @param error_message
   * @return true
   * @return false
   */
  static auto can_open_as_text_file(const QFileInfo& file_info,
                                    QString* error_message) -> bool;

  /**
   * @brief
   *
   * @param page
   */
  void schedule_recovery_cache(PlainTextEditorPage* page);

  /**
   * @brief
   *
   * @param force
   */
  void flush_recovery_cache(bool force = false);
};

}  // namespace GpgFrontend::UI
