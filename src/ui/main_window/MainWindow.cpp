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

#include "MainWindow.h"

#include <algorithm>

#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/main_window/GeneralMainWindow.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

MainWindow::MainWindow() : GeneralMainWindow("main_window") {
  this->setWindowTitle(qApp->applicationDisplayName());
  RegisterNamedQObject("main_window", this);
}

void MainWindow::Init() noexcept {
  try {
    /* get path where app was started */
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    edit_ = new TextEdit(this);
    setCentralWidget(edit_);
    RegisterNamedQObject("main_window_edit", edit_);

    /* the list of Keys available*/
    m_key_list_ = new KeyList(
        kGpgFrontendDefaultChannel,
        KeyMenuAbility::kREFRESH | KeyMenuAbility::kCHECK_ALL |
            KeyMenuAbility::kUNCHECK_ALL | KeyMenuAbility::kCOLUMN_FILTER |
            KeyMenuAbility::kSEARCH_BAR | KeyMenuAbility::kKEY_DATABASE |
            KeyMenuAbility::kKEY_GROUP,
        GpgKeyTableColumn::kTYPE | GpgKeyTableColumn::kNAME |
            GpgKeyTableColumn::kKEY_ID | GpgKeyTableColumn::kEMAIL_ADDRESS |
            GpgKeyTableColumn::kUSAGE | GpgKeyTableColumn::kOWNER_TRUST |
            GpgKeyTableColumn::kCOMMENT | GpgKeyTableColumn::kCREATE_DATE,
        this);

    info_board_ = new InfoBoardWidget(this);

    /* List of binary Attachments */
    attachment_dock_created_ = false;

    /* Variable containing if restart is needed */
    this->SlotSetRestartNeeded(0);

    // init menu bar
    this->setMenuBar(new QMenuBar());

    create_actions();
    create_menus();
    create_tool_bars();
    create_status_bar();
    create_dock_windows();
    init_main_window_style();

    // show menu bar
    this->menuBar()->show();

    connect(this, &MainWindow::SignalRestartApplication,
            UISignalStation::GetInstance(),
            &UISignalStation::SignalRestartApplication);

    connect(this, &MainWindow::SignalUIRefresh, UISignalStation::GetInstance(),
            &UISignalStation::SignalUIRefresh);
    connect(this, &MainWindow::SignalKeyDatabaseRefresh,
            UISignalStation::GetInstance(),
            &UISignalStation::SignalKeyDatabaseRefresh);

    connect(edit_->TabWidget(), &TextEditTabWidget::currentChanged, this,
            &MainWindow::slot_switch_menu_control_mode);
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalRefreshStatusBar, this,
            [=](const QString& message, int timeout) {
              statusBar()->showMessage(message, timeout);
            });
    connect(
        UISignalStation::GetInstance(),
        &UISignalStation::SignalMainWindowUpdateBasicOperaMenu, this,
        [=](unsigned int mask) {
          operations_menu_mask_ = mask;
          slot_update_operations_menu_by_checked_keys(operations_menu_mask_);
        });
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalMainWindowOpenFile, this,
            &MainWindow::SlotOpenFile);

#ifndef Q_OS_WINDOWS
    // check if GnuPG is configured to use a GUI pinentry, and if not, show a
    // warning message to user
    if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
      connect(this, &MainWindow::SignalLoaded, this, [=]() {
        QTimer::singleShot(3000, [self = QPointer<MainWindow>(this)]() -> void {
          if (self && DecidePinentry().isEmpty()) {
            QMessageBox::warning(
                self, tr("GUI Pinentry Not Found"),
                tr("No suitable *graphical* Pinentry program was found on your "
                   "system.\n\n"
                   "Please install a GUI-based Pinentry (e.g., 'pinentry-qt', "
                   "'pinentry-gnome3', or 'pinentry-mac' on macOS).\n\n"
                   "Without a GUI Pinentry, GnuPG cannot prompt you for "
                   "passwords or passphrases.\n\n"
                   "After installing it, please restart GpgFrontend. The "
                   "configuration file will be updated automatically."));
          }
        });
      });
    }
#endif

    popup_menu_ = new QMenu(this);

    popup_menu_->addAction(append_selected_keys_act_);
    popup_menu_->addAction(append_key_create_date_to_editor_act_);
    popup_menu_->addAction(append_key_expire_date_to_editor_act_);
    popup_menu_->addAction(append_key_fingerprint_to_editor_act_);
    popup_menu_->addSeparator();
    popup_menu_->addAction(copy_mail_address_to_clipboard_act_);
    popup_menu_->addAction(copy_key_default_uid_to_clipboard_act_);
    popup_menu_->addAction(copy_key_id_to_clipboard_act_);
    popup_menu_->addAction(set_owner_trust_of_key_act_);
    popup_menu_->addAction(add_key_2_favourite_act_);
    popup_menu_->addAction(remove_key_from_favourtie_act_);

    popup_menu_->addSeparator();
    popup_menu_->addAction(show_key_details_act_);

    connect(m_key_list_, &KeyList::SignalRequestContextMenu, this,
            &MainWindow::slot_popup_menu_by_key_list);
    connect(m_key_list_, &KeyList::SignalRefreshDatabase, this,
            [=]() { slot_update_engine_status(); });

    restore_settings();

    const bool state_restored = restoreWindowState();
    if (!state_restored) {
      QTimer::singleShot(0, this, [this]() -> void { apply_default_layout(); });
    }

    info_board_->AssociateTabWidget(edit_->TabWidget());

    slot_switch_menu_control_mode(0);

    // check if need to open wizard window
    if (GetSettings().value("wizard/show_wizard", true).toBool()) {
      slot_start_wizard();
    }

    // check if there are invalid key databases and notify user
    check_and_notify_invalid_key_dbs();

    // update the OpenPGP engine status in the status bar
    slot_update_engine_status();

    // loading process is done
    emit SignalLoaded();

    // notify other modules that application is loaded
    Module::TriggerEvent("APPLICATION_LOADED",
                         {
                             {"main_window", GFBuffer(RegisterQObject(this))},
                         });
  } catch (...) {
    LOG_W() << tr("Critical error occur while loading GpgFrontend.");
    QMessageBox::critical(
        nullptr, tr("Loading Failed"),
        tr("Critical error occur while loading GpgFrontend."));
    QCoreApplication::quit();
    exit(0);
  }
}

void MainWindow::restore_settings() {
  auto settings = GetSettings();
  if (!settings.contains("gnupg/non_ascii_at_file_operation")) {
    settings.setValue("gnupg/non_ascii_at_file_operation", true);
  }

  prohibit_update_checking_ =
      settings.value("network/prohibit_update_check").toBool();

  AppearanceSO const appearance(SettingsObject("general_settings_state"));

  icon_style_ = appearance.tool_bar_button_style;
  icon_size_ =
      QSize(appearance.tool_bar_icon_width, appearance.tool_bar_icon_height);

  crypt_tool_bar_->clear();

  if ((appearance.tool_bar_crypto_operas_type & GpgOperation::kENCRYPT) != 0) {
    crypt_tool_bar_->addAction(encrypt_act_);
  }
  if ((appearance.tool_bar_crypto_operas_type & GpgOperation::kDECRYPT) != 0) {
    crypt_tool_bar_->addAction(decrypt_act_);
  }
  if ((appearance.tool_bar_crypto_operas_type & GpgOperation::kSIGN) != 0) {
    crypt_tool_bar_->addAction(sign_act_);
  }
  if ((appearance.tool_bar_crypto_operas_type & GpgOperation::kVERIFY) != 0) {
    crypt_tool_bar_->addAction(verify_act_);
  }
  if ((appearance.tool_bar_crypto_operas_type & GpgOperation::kENCRYPT_SIGN) !=
      0) {
    crypt_tool_bar_->addAction(encrypt_sign_act_);
  }
  if ((appearance.tool_bar_crypto_operas_type &
       GpgOperation::kDECRYPT_VERIFY) != 0) {
    crypt_tool_bar_->addAction(decrypt_verify_act_);
  }
  if ((appearance.tool_bar_crypto_operas_type &
       GpgOperation::kSYMMETRIC_ENCRYPT) != 0) {
    crypt_tool_bar_->addAction(sym_encrypt_act_);
  }

  apply_tool_bar_appearance();
}

void MainWindow::close_attachment_dock() {
  if (!attachment_dock_created_) {
    return;
  }
  attachment_dock_->close();
  attachment_dock_->deleteLater();
  attachment_dock_created_ = false;
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (!edit_->MaybeSaveAnyTab()) {
    event->ignore();
    return;
  }
  GeneralMainWindow::closeEvent(event);
}

auto MainWindow::create_action(const QString& id, const QString& name,
                               const QString& icon, const QString& too_tip,
                               const QContainer<QKeySequence>& shortcuts)
    -> QAction* {
  auto* action = new QAction(name, this);

  if (!icon.isEmpty()) {
    action->setIcon(QIcon(icon));
  }

  action->setToolTip(too_tip);
  action->setStatusTip(too_tip);

  if (!shortcuts.isEmpty()) {
    action->setShortcuts(
        QList<QKeySequence>{shortcuts.cbegin(), shortcuts.cend()});
  }

  buffered_actions_.insert(id, {action});
  return action;
}

void MainWindow::slot_popup_menu_by_key_list(QContextMenuEvent* event,
                                             KeyTable* key_table) {
  if (event == nullptr || key_table == nullptr) return;

  const auto key_table_name = key_table->objectName();

  LOG_D() << "current key table object name: " << key_table_name;

  if (key_table_name == "favourite") {
    remove_key_from_favourtie_act_->setVisible(true);
    add_key_2_favourite_act_->setVisible(false);
  } else {
    remove_key_from_favourtie_act_->setVisible(false);
    add_key_2_favourite_act_->setVisible(true);
  }

  auto if_owner_trust_level_supported = IsOpSupported<SetOwnerTrustLevelOpTag>(
      m_key_list_->GetCurrentGpgContextChannel());
  set_owner_trust_of_key_act_->setVisible(if_owner_trust_level_supported);

  popup_menu_->popup(event->globalPos());
}

auto MainWindow::GetCurrentGpgContextChannel() const -> int {
  return m_key_list_->GetCurrentGpgContextChannel();
}

auto MainWindow::check_and_notify_invalid_key_dbs() -> void {
  auto key_dbs = GetAllKeyDatabaseInfoBySettings();
  auto active_dbs = GetGpgKeyDatabaseInfos();

  // 1. Filter and identify invalid databases
  QString details;
  int invalid_count = 0;

  for (const auto& key_db : key_dbs) {
    bool is_active = std::find_if(active_dbs.begin(), active_dbs.end(),
                                  [key_db](const KeyDatabaseInfo& i) -> bool {
                                    return i.name == key_db.name;
                                  }) != active_dbs.end();

    if (!key_db.valid || !is_active) {
      invalid_count++;
      details += tr("Name: %1").arg(key_db.name) + "\n" +
                 tr("Path: %1").arg(key_db.origin_path) + "\n\n";
    }
  }

  // If everything is valid, just log and exit
  if (invalid_count == 0) {
    LOG_D() << "No invalid key databases detected.";
    return;
  }

  // 2. Configure the Message Box
  auto* msg_box = new QMessageBox(this);
  msg_box->setIcon(QMessageBox::Warning);
  msg_box->setWindowTitle(tr("Invalid Key Databases"));
  msg_box->setAttribute(Qt::WA_DeleteOnClose);

  // Main summary text (Large font)
  msg_box->setText(
      tr("Detected %1 invalid key database(s).").arg(invalid_count));
  // Secondary description text
  msg_box->setInformativeText(
      tr("The application cannot load these databases. "
         "Please review the details below and fix the issues in the GnuPG "
         "Controller."));

  // Technical details hidden behind the "Show Details..." button
  msg_box->setDetailedText(details);
  msg_box->addButton(QMessageBox::Ignore);

  // 4. Execute and Handle Interaction
  LOG_W() << "Invalid key databases detected count: " << invalid_count;
  msg_box->show();
}

}  // namespace GpgFrontend::UI
