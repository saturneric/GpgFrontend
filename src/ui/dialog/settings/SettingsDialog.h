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

#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

class GeneralTab;
class AppearanceTab;
class KeyserverTab;
class NetworkTab;
class KeyDatabasesTab;
class GnuPGTab;
class RpgpTab;
class InstantMessagingTab;
class AdvancedTab;

/**
 * @brief
 *
 */
class SettingsDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Settings Dialog object
   *
   * @param parent
   */
  explicit SettingsDialog(QWidget* parent = nullptr);

  GeneralTab* general_tab_;        ///<
  AppearanceTab* appearance_tab_;  ///<
  KeyserverTab* key_server_tab_;   ///<
  NetworkTab* network_tab_;        ///<
  KeyDatabasesTab* key_dbs_tab_;   ///<
  GnuPGTab* gnupg_tab_;            ///<
  RpgpTab* rpgp_tab_;              ///<
  InstantMessagingTab* im_tab_;    ///<
  AdvancedTab* advanced_tab_;      ///<

  /**
   * @brief
   *
   * @return QHash<QString, QString>
   */
  static QHash<QString, QString> ListLanguages();

  /**
   * @brief Preselect the navigation entry hosting @p page (e.g. im_tab_).
   *
   * Clears any active search first, so a page filtered out of the list is
   * still reachable. A no-op when that page has no entry: several are
   * conditional on the sandbox and on engine support, so rows are not fixed
   * and cannot be assumed.
   */
  void SelectPageFor(QWidget* page);

 public slots:

  /**
   * @brief
   *
   */
  void SlotAccept();

 signals:

  /**
   * @brief
   *
   * @param needed
   */
  void SignalRestartNeeded(int);

  /**
   * @brief Emitted after appearance settings are applied.
   */
  void SignalAppearanceChanged();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void showEvent(QShowEvent* event) override;

  /**
   * @brief Keep the search field's keys from reaching the dialog.
   *
   * QLineEdit ignores Return, so without this Enter would fall through to the
   * default button and close the dialog on someone who was only searching. The
   * arrow keys are forwarded to the list, so a search can be finished with the
   * keyboard alone.
   *
   * @param watched the search field
   * @param event the event being delivered
   * @return true when the event was consumed here
   */
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;

 private:
  /**
   * @brief Record that an edit on a page only takes effect after a restart.
   *
   * Keeps the deepest restart requested — a later shallow change must never
   * downgrade a pending deep restart.
   *
   * @param mode restart depth this change needs
   * @param page tab title, listed back to the user on confirmation
   */
  void declare_restart(int mode, const QString& page);

  /**
   * @brief Ask the user whether to save the changes that need a restart.
   *
   * @return true to go ahead and save, false to discard the pending changes
   */
  auto confirm_restart() -> bool;

  /**
   * @brief Reload every tab from the settings store, dropping pending edits.
   */
  void revert_all_tabs();

  /**
   * @brief One navigable page of the dialog.
   *
   * Pages are registered in display order; the section a page belongs to is
   * carried here rather than in a separate structure so a section header can
   * be emitted lazily, when its first available page shows up. Which pages are
   * available depends on the sandbox and on engine support.
   */
  struct SettingsPage {
    QWidget* page;         ///< the page widget itself
    QString title;         ///< sidebar row, heading, restart confirmation
    QString section;       ///< group header this page lives under
    QStringList keywords;  ///< extra search terms, beyond the title
  };

  /**
   * @brief Register @p page and give it a row in the navigation list.
   *
   * Inserts the section header first if this is the section's first page.
   *
   * @param page the page widget, wrapped in a scroll area for the stack
   * @param title display name
   * @param section group header text
   * @param keywords additional terms the search should match on
   */
  void add_page(QWidget* page, const QString& title, const QString& section,
                const QStringList& keywords);

  /**
   * @brief Show only the rows matching @p text, hiding emptied sections.
   *
   * Only the list is filtered: every page stays alive in the stack, so edits
   * made on a page that later gets filtered out are still applied on OK.
   *
   * @param text what the user typed, empty to show everything
   */
  void filter_pages(const QString& text);

  /**
   * @brief Select the first page row that is currently visible.
   */
  void select_first_visible_page();

  QListWidget* nav_list_;                     ///< sections + page rows
  QStackedWidget* page_stack_;                ///< scroll-wrapped pages
  QLineEdit* search_edit_;                    ///< filters nav_list_
  QLabel* page_title_label_;                  ///< heading above the page
  QVector<SettingsPage> pages_;               ///< registered pages, in order
  QHash<QWidget*, int> nav_row_of_page_;      ///< page widget -> nav row
  QDialogButtonBox* button_box_;              ///<
  int restart_mode_{kNonRestartCode};         ///<
  QStringList restart_pages_;                 ///< pages with such a change
  bool suppress_restart_declaration_{false};  ///< set while reverting
};

}  // namespace GpgFrontend::UI
