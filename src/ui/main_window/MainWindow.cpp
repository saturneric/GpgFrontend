/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "core/function/CacheManager.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/module/ModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/main_window/GeneralMainWindow.h"
#include "ui/struct/SettingsObject.h"
#include "ui/struct/settings/KeyServerSO.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

MainWindow::MainWindow() : GeneralMainWindow("main_window") {
  this->setMinimumSize(1200, 700);
  this->setWindowTitle(qApp->applicationName());

  connect(CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalNeedUserInputPassphrase, this,
          &MainWindow::SlotRaisePinentry);
}

void MainWindow::Init() noexcept {
  try {
    /* get path where app was started */
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    edit_ = new TextEdit(this);
    setCentralWidget(edit_);

    /* the list of Keys available*/
    m_key_list_ =
        new KeyList(KeyMenuAbility::REFRESH | KeyMenuAbility::UNCHECK_ALL |
                        KeyMenuAbility::SEARCH_BAR,
                    this);

    info_board_ = new InfoBoardWidget(this);

    /* List of binary Attachments */
    attachment_dock_created_ = false;

    /* Variable containing if restart is needed */
    this->SlotSetRestartNeeded(false);

    // init menu bar
    this->setMenuBar(new QMenuBar());

    create_actions();
    create_menus();
    create_tool_bars();
    create_status_bar();
    create_dock_windows();

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

    connect(edit_->tab_widget_, &QTabWidget::currentChanged, this,
            &MainWindow::slot_disable_tab_actions);
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalRefreshStatusBar, this,
            [=](const QString &message, int timeout) {
              statusBar()->showMessage(message, timeout);
            });
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalMainWindowlUpdateBasicalOperaMenu, this,
            &MainWindow::SlotUpdateCryptoMenuStatus);
    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalMainWindowOpenFile, this,
            &MainWindow::SlotOpenFile);

    m_key_list_->AddMenuAction(append_selected_keys_act_);
    m_key_list_->AddMenuAction(append_key_create_date_to_editor_act_);
    m_key_list_->AddMenuAction(append_key_expire_date_to_editor_act_);
    m_key_list_->AddMenuAction(append_key_fingerprint_to_editor_act_);
    m_key_list_->AddSeparator();
    m_key_list_->AddMenuAction(copy_mail_address_to_clipboard_act_);
    m_key_list_->AddMenuAction(copy_key_default_uid_to_clipboard_act_);
    m_key_list_->AddMenuAction(copy_key_id_to_clipboard_act_);
    m_key_list_->AddMenuAction(set_owner_trust_of_key_act_);
    m_key_list_->AddMenuAction(add_key_2_favourtie_act_);
    m_key_list_->AddMenuAction(remove_key_from_favourtie_act_);

    m_key_list_->AddSeparator();
    m_key_list_->AddMenuAction(show_key_details_act_);

    restore_settings();

    edit_->CurTextPage()->setFocus();

    Module::ListenRTPublishEvent(
        this, "com.bktus.gpgfrontend.module.integrated.version-checking",
        "version.loading_done",
        [=](Module::Namespace, Module::Key, int, std::any) {
          GF_UI_LOG_DEBUG(
              "versionchecking version.loading_done changed, calling slot "
              "version upgrade");
          this->slot_version_upgrade_nofity();
        });

    // loading process is done
    emit SignalLoaded();
    Module::TriggerEvent("APPLICATION_LOADED");

    // recover unsaved page from cache if it exists
    recover_editor_unsaved_pages_from_cache();

    // check if need to open wizard window
    auto settings = GlobalSettingStation::GetInstance().GetSettings();
    bool show_wizard = settings.value("wizard/show_wizard", true).toBool();
    if (show_wizard) slot_start_wizard();

  } catch (...) {
    GF_UI_LOG_ERROR(tr("Critical error occur while loading GpgFrontend."));
    QMessageBox::critical(
        nullptr, tr("Loading Failed"),
        tr("Critical error occur while loading GpgFrontend."));
    QCoreApplication::quit();
    exit(0);
  }
}

void MainWindow::restore_settings() {
  GF_UI_LOG_DEBUG("restore settings for main windows");

  KeyServerSO key_server(SettingsObject("key_server"));
  if (key_server.server_list.empty()) key_server.ResetDefaultServerList();
  if (key_server.default_server < 0) key_server.default_server = 0;

  auto settings = GlobalSettingStation::GetInstance().GetSettings();
  if (!settings.contains("basic/non_ascii_when_export")) {
    settings.setValue("basic/non_ascii_when_export", true);
  }

  // set appearance
  import_button_->setToolButtonStyle(icon_style_);

  prohibit_update_checking_ =
      settings.value("network/prohibit_update_check").toBool();

  GF_UI_LOG_DEBUG("settings restored");
}

void MainWindow::recover_editor_unsaved_pages_from_cache() {
  auto json_data =
      CacheManager::GetInstance().LoadDurableCache("editor_unsaved_pages");

  if (json_data.isEmpty() || !json_data.isArray()) {
    return;
  }

  GF_UI_LOG_DEBUG("plan ot recover unsaved page from cache, page array: {}",
                  json_data.toJson());

  bool first = true;

  QJsonArray unsaved_page_array = json_data.array();
  for (const auto &value_ref : unsaved_page_array) {
    if (!value_ref.isObject()) continue;
    auto unsaved_page_json = value_ref.toObject();

    if (!unsaved_page_json.contains("title") ||
        !unsaved_page_json.contains("content")) {
      continue;
    }

    QString title = unsaved_page_json["title"].toString();
    QString content = unsaved_page_json["content"].toString();

    GF_UI_LOG_DEBUG(
        "recovering unsaved page from cache, page title: {}, content size",
        title, content.size());

    if (first) {
      edit_->SlotCloseTab();
      first = false;
    }

    edit_->SlotNewTabWithContent(title, content);
  }
}

void MainWindow::close_attachment_dock() {
  if (!attachment_dock_created_) {
    return;
  }
  attachment_dock_->close();
  attachment_dock_->deleteLater();
  attachment_dock_created_ = false;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  /*
   * ask to save changes, if there are
   * modified documents in any tab
   */
  if (edit_->MaybeSaveAnyTab()) {
    event->accept();
  } else {
    event->ignore();
  }

  if (event->isAccepted()) {
    // clear cache of unsaved page
    CacheManager::GetInstance().SaveDurableCache(
        "editor_unsaved_pages", QJsonDocument(QJsonArray()), true);

    // clear password from memory
    //  GpgContext::GetInstance().clearPasswordCache();

    // call parent
    GeneralMainWindow::closeEvent(event);
  }
}

}  // namespace GpgFrontend::UI
