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
   * @brief Whether the current tab is an ordinary plain-text document.
   *
   * A module-backed tab IS a PlainTextEditorPage, so CurPageTextEdit() cannot
   * answer this; only the tab type can.
   */
  [[nodiscard]] auto CurPageIsPlainText() const -> bool;

  /**
   * @brief Which crypto operations the current tab's view says apply now.
   *
   * @p has_opinion is false when no view answered, which means the tab type's
   * own capabilities are the whole answer.
   */
  [[nodiscard]] auto CurPageCryptoOperations(bool& has_opinion) const
      -> QStringList;

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
   * @brief The editor page holding document @p id; 0 means the current page.
   *
   * A document id is how anything outside the editor -- a command, a UI
   * script -- names a document without holding a widget. Ids are never
   * reused, so an id that outlived its tab finds nothing rather than finding
   * whatever tab came next.
   *
   * @return the page, or nullptr when there is no such document
   */
  [[nodiscard]] auto PageForDocument(qint64 id) -> PlainTextEditorPage*;

  /// The document id of @p page; 0 when it is not an editor page.
  static auto DocumentIdOf(const QWidget* page) -> qint64;

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

 signals:
  /**
   * @brief Emitted when a text page changes its text direction mode.
   *
   * Re-emitted on behalf of the pages so the main window can follow every tab
   * through one connection instead of one per page.
   */
  void SignalTextDirectionModeChanged();

  /**
   * @brief Emitted when the CURRENT tab's mounted view asks for a crypto
   * operation.
   *
   * Only the current tab: the operations act on whatever tab is in front, so
   * a request from a background one would run against the wrong document.
   */
  void SignalCryptoOperationRequested(const QString& operation);

  /// Emitted when the CURRENT tab's view changed which operations apply.
  void SignalCryptoOperationsChanged();

 private:
  int count_page_ = 0;
  int text_page_data_modified_count_ = 0;

  QTimer* recovery_cache_timer_ = nullptr;
  QList<QPointer<PlainTextEditorPage>> recovery_dirty_pages_;
  QPointer<PlainTextEditorPage> last_current_text_page_;
  bool recovery_restoring_ = false;
  /// Set once the application is quitting. From that point an empty sweep of
  /// the tabs means they have been destroyed, not that there is nothing to
  /// keep, so it must never clear the recovery cache.
  bool recovery_shutting_down_ = false;
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
   * @brief Mounts a module-registered primary view on a freshly created tab.
   *
   * Does nothing when no module has claimed @p type, which is what keeps every
   * pre-existing tab type behaving exactly as it did before.
   *
   * @param page the page just created for the tab
   * @param type the tab type, matched case-insensitively
   */
  void mount_module_view(PlainTextEditorPage* page, const QString& type);

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