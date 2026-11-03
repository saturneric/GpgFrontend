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
#include "ui/function/TabManagement.h"

namespace GpgFrontend::UI {

class PlainTextEditorPage;
class FilePage;
class EditorTabBar;

class GF_UI_EXPORT TextEditTabWidget : public QTabWidget {
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
   * @brief The tab holding document @p id; 0 means the current tab,
   *        whatever its type (text, file browser, a module's page).
   *
   * A document id is how anything outside the editor -- a command, a UI
   * script -- names a document without holding a widget. Ids are never
   * reused, so an id that outlived its tab finds nothing rather than finding
   * whatever tab came next.
   *
   * @return the tab, or nullptr when there is no such document
   */
  [[nodiscard]] auto PageForDocument(qint64 id) -> QWidget*;

  /// The document id of @p page; 0 when it is not an editor page.
  static auto DocumentIdOf(const QWidget* page) -> qint64;

  /**
   * @brief The tab id of @p page; 0 when it is not one of the tabs.
   *
   * Every tab has one, whatever its type, and keeps it when it is dragged
   * elsewhere. Never reused. What outlives a dialog, a callback or a Ctrl+Tab
   * session names a tab by this, never by its index.
   */
  static auto TabIdOf(const QWidget* page) -> quint64;

  /// The tab with id @p id, or nullptr when it is gone.
  [[nodiscard]] auto PageForTab(quint64 id) const -> QWidget*;

  /// Every tab's id, in visual order.
  [[nodiscard]] auto TabIds() const -> QList<quint64>;

  /// Every tab's id, most recently used first; tabs never shown come last.
  [[nodiscard]] auto MruTabIds() const -> QList<quint64>;

  /// The name of @p page's document, without any decoration.
  static auto TabTitleOf(const QWidget* page) -> QString;

  /// Whether @p page holds unsaved changes.
  static auto IsPageModified(const QWidget* page) -> bool;

  /// Rename @p page's tab.
  void SetTabTitle(QWidget* page, const QString& title);

  /// The tab's label as shown: its title, and for a title shared with
  /// another tab the end of its folder that tells the two apart.
  [[nodiscard]] auto TabLabel(int index) const -> QString;

  /// Recompute every tab's label, tooltip and unsaved mark.
  void RefreshTabLabels();

  [[nodiscard]] auto Bar() const -> EditorTabBar*;

  /**
   * @brief One Ctrl+Tab (@p delta 1) or Ctrl+Shift+Tab (-1) press.
   *
   * The first press of a session snapshots the most-recently-used order and
   * goes to the tab used before the current one; later presses, while the
   * modifier is held, walk that snapshot. The session ends when the modifier
   * is released, the window loses focus or a dialog opens.
   */
  void StepSwitchSession(int delta);

  /// End the Ctrl+Tab session; the tab it ended on becomes the most recent.
  void EndSwitchSession();

  [[nodiscard]] auto SwitchSessionActive() const -> bool;

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
  void tabInserted(int index) override;
  void tabRemoved(int index) override;
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

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

  /**
   * @brief Emitted when a module view was mounted on the CURRENT tab.
   *
   * A restored tab becomes current before its view is mounted, so what the
   * page asks of the rest of the window (a compact Status Panel, say) is only
   * known after currentChanged has already been and gone.
   */
  void SignalCurrentPageViewMounted();

  /// The user asked to close the tabs @p ids (context menu, middle click).
  void SignalCloseTabsRequested(const QList<quint64>& ids);

  /// The number of tabs changed.
  void SignalTabCountChanged(int count);

  /// The open-tabs list was asked for (the button at the end of the strip).
  void SignalOpenTabsListRequested();

 private:
  int count_page_ = 0;
  quint64 next_tab_id_ = 1;
  TabMruOrder mru_;
  TabSwitchSession switch_session_;
  EditorTabBar* bar_ = nullptr;
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

  /// Show the tab menu for tab @p index at @p pos (tab bar coordinates).
  void show_tab_menu(int index, const QPoint& pos);

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