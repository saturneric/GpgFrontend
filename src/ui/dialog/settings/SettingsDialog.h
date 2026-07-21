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
   * @brief Preselect the tab hosting @p page (e.g. im_tab_).
   *
   * A no-op when that page has no tab: several are conditional on the sandbox
   * and on engine support, so tab indices are not fixed and cannot be assumed.
   */
  void SelectTabFor(QWidget* page);

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

  QTabWidget* tab_widget_;                    ///<
  QHash<QWidget*, int> tab_index_of_page_;    ///< page widget -> tab index
  QDialogButtonBox* button_box_;              ///<
  int restart_mode_{kNonRestartCode};         ///<
  QStringList restart_pages_;                 ///< pages with such a change
  bool suppress_restart_declaration_{false};  ///< set while reverting
};

}  // namespace GpgFrontend::UI
