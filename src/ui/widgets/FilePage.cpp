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

#include "ui/widgets/FilePage.h"

#include "core/function/GlobalSettingStation.h"
#include "ui/UISignalStation.h"
#include "ui/main_window/MainWindow.h"
#include "ui_FilePage.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent, const QString& target_path)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_FilePage>()) {
  ui_->setupUi(this);

  ui_->batchModeButton->setToolTip(tr("Switch Batch Mode"));

  connect(ui_->upPathButton, &QPushButton::clicked, ui_->treeView,
          &FileTreeView::SlotUpLevel);
  connect(ui_->refreshButton, &QPushButton::clicked, this,
          &FilePage::SlotGoPath);
  connect(this->ui_->newDirButton, &QPushButton::clicked, ui_->treeView,
          &FileTreeView::SlotMkdir);

  ui_->treeView->SetPath(target_path);
  ui_->pathEdit->setText(ui_->treeView->GetCurrentPath());

  path_edit_completer_ = new QCompleter(this);
  path_complete_model_ = new QStringListModel();
  path_edit_completer_->setModel(path_complete_model_);
  path_edit_completer_->setCaseSensitivity(Qt::CaseInsensitive);
  path_edit_completer_->setCompletionMode(
      QCompleter::UnfilteredPopupCompletion);
  ui_->pathEdit->setCompleter(path_edit_completer_);

  option_popup_menu_ = new QMenu(this);
  auto* show_hidden_act = new QAction(tr("Show Hidden File"), this);
  show_hidden_act->setCheckable(true);
  connect(show_hidden_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotShowHiddenFile);
  option_popup_menu_->addAction(show_hidden_act);

  auto* show_system_act = new QAction(tr("Show System File"), this);
  show_system_act->setCheckable(true);
  connect(show_system_act, &QAction::triggered, ui_->treeView,
          &FileTreeView::SlotShowSystemFile);
  option_popup_menu_->addAction(show_system_act);

  auto* switch_asc_mode_act = new QAction(tr("ASCII Mode"), this);
  switch_asc_mode_act->setCheckable(true);
  connect(switch_asc_mode_act, &QAction::triggered, this,
          [=](bool on) { ascii_mode_ = on; });
  option_popup_menu_->addAction(switch_asc_mode_act);

  ui_->optionsButton->setMenu(option_popup_menu_);

  ascii_mode_ = !(
      GetSettings().value("gnupg/non_ascii_at_file_operation", true).toBool());
  switch_asc_mode_act->setChecked(ascii_mode_);

  connect(ui_->pathEdit, &QLineEdit::textChanged, [=]() {
    auto path = ui_->pathEdit->text();
    auto dir = QDir(path);
    if (path.endsWith("/") && dir.isReadable()) {
      auto dir_list = dir.entryInfoList(QDir::AllEntries);
      QStringList paths;
      for (int i = 1; i < dir_list.size(); i++) {
        const auto file_path = dir_list.at(i).filePath();
        const auto file_name = dir_list.at(i).fileName();
        if (file_name == "." || file_name == "..") continue;
        paths.append(file_path);
      }
      path_complete_model_->setStringList(paths);
    }
  });

  connect(this, &FilePage::SignalRefreshInfoBoard,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshInfoBoard);
  connect(ui_->treeView, &FileTreeView::SignalPathChanged, this,
          [this](const QString& path) { this->ui_->pathEdit->setText(path); });
  connect(ui_->treeView, &FileTreeView::SignalPathChanged, this,
          &FilePage::SignalPathChanged);
  connect(ui_->treeView, &FileTreeView::SignalOpenFile,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowOpenFile);
  connect(ui_->treeView, &FileTreeView::SignalSelectedChanged, this,
          &FilePage::update_main_basic_opera_menu);
  connect(this, &FilePage::SignalCurrentTabChanged, this,
          [this]() { update_main_basic_opera_menu(GetSelected()); });
  connect(this, &FilePage::SignalMainWindowUpdateBasicOperaMenu,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowUpdateBasicOperaMenu);
  connect(ui_->batchModeButton, &QToolButton::toggled, ui_->treeView,
          &FileTreeView::SlotSwitchBatchMode);
}

auto FilePage::GetSelected() const -> QStringList {
  return ui_->treeView->GetSelectedPaths();
}

void FilePage::SlotGoPath() {
  ui_->treeView->SlotGoPath(ui_->pathEdit->text());
}

void FilePage::keyPressEvent(QKeyEvent* event) {
  if (ui_->pathEdit->hasFocus() &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    SlotGoPath();
  }
}

void FilePage::update_main_basic_opera_menu(const QStringList& selected_paths) {
  if (selected_paths.isEmpty()) {
    emit SignalMainWindowUpdateBasicOperaMenu(MainWindow::OperationMenu::kNone);
    return;
  }

  MainWindow::OperationMenu::OperationType operation_type =
      MainWindow::OperationMenu::kNone;

  LOG_D() << "selected path size: " << selected_paths.size();
  LOG_D() << "selected paths: " << selected_paths;

  QContainer<QFileInfo> infos;

  for (const auto& path : selected_paths) {
    infos.append(QFileInfo(path));
  }

  bool c_encr =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return (info.isDir() || info.isFile()) &&
               (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                info.suffix() != "sig" && info.suffix() != "asc");
      });

  if (c_encr) {
    operation_type |= MainWindow::OperationMenu::kEncrypt |
                      MainWindow::OperationMenu::kEncryptAndSign;
  }

  bool c_decr =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return info.isFile() &&
               (info.suffix() == "gpg" || info.suffix() == "pgp" ||
                info.suffix() == "asc");
      });

  if (c_decr) {
    operation_type |= MainWindow::OperationMenu::kDecrypt |
                      MainWindow::OperationMenu::kDecryptAndVerify;
  }

  bool c_sign =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return info.isFile() &&
               (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                info.suffix() != "sig" && info.suffix() != "asc");
      });

  if (c_sign) operation_type |= MainWindow::OperationMenu::kSign;

  bool c_verify =
      std::all_of(infos.cbegin(), infos.cend(), [](const QFileInfo& info) {
        return info.isFile() &&
               (info.suffix() == "sig" || info.suffix() == "gpg" ||
                info.suffix() == "pgp" || info.suffix() == "asc");
      });

  if (c_verify) operation_type |= MainWindow::OperationMenu::kVerify;

  emit SignalMainWindowUpdateBasicOperaMenu(operation_type);
}

auto FilePage::IsBatchMode() const -> bool {
  return ui_->batchModeButton->isChecked();
}

auto FilePage::IsASCIIMode() const -> bool { return ascii_mode_; }
}  // namespace GpgFrontend::UI
