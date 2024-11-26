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

#include "core/GpgModel.h"
#include "ui/UISignalStation.h"
#include "ui/main_window/MainWindow.h"
#include "ui_FilePage.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent, const QString& target_path)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_FilePage>()),
      file_tree_view_(new FileTreeView(this, target_path)) {
  ui_->setupUi(this);
  ui_->trewViewLayout->addWidget(file_tree_view_);

  connect(ui_->upPathButton, &QPushButton::clicked, file_tree_view_,
          &FileTreeView::SlotUpLevel);
  connect(ui_->refreshButton, &QPushButton::clicked, this,
          &FilePage::SlotGoPath);
  connect(this->ui_->newDirButton, &QPushButton::clicked, file_tree_view_,
          &FileTreeView::SlotMkdir);

  ui_->pathEdit->setText(file_tree_view_->GetCurrentPath());

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
  connect(show_hidden_act, &QAction::triggered, file_tree_view_,
          &FileTreeView::SlotShowHiddenFile);
  option_popup_menu_->addAction(show_hidden_act);

  auto* show_system_act = new QAction(tr("Show System File"), this);
  show_system_act->setCheckable(true);
  connect(show_system_act, &QAction::triggered, file_tree_view_,
          &FileTreeView::SlotShowSystemFile);
  option_popup_menu_->addAction(show_system_act);
  ui_->optionsButton->setMenu(option_popup_menu_);

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
  connect(file_tree_view_, &FileTreeView::SignalPathChanged, this,
          [this](const QString& path) { this->ui_->pathEdit->setText(path); });
  connect(file_tree_view_, &FileTreeView::SignalPathChanged, this,
          &FilePage::SignalPathChanged);
  connect(file_tree_view_, &FileTreeView::SignalOpenFile,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowOpenFile);
  connect(file_tree_view_, &FileTreeView::SignalSelectedChanged, this,
          &FilePage::update_main_basical_opera_menu);
  connect(this, &FilePage::SignalCurrentTabChanged, this,
          [this]() { update_main_basical_opera_menu(GetSelected()); });
  connect(this, &FilePage::SignalMainWindowlUpdateBasicalOperaMenu,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalMainWindowlUpdateBasicalOperaMenu);
}

auto FilePage::GetSelected() const -> QString {
  return file_tree_view_->GetSelectedPath();
}

void FilePage::SlotGoPath() {
  file_tree_view_->SlotGoPath(ui_->pathEdit->text());
}

void FilePage::keyPressEvent(QKeyEvent* event) {
  if (ui_->pathEdit->hasFocus() &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    SlotGoPath();
  }
}

void FilePage::update_main_basical_opera_menu(const QString& selected_path) {
  MainWindow::OperationMenu::OperationType operation_type =
      MainWindow::OperationMenu::kNone;

  // abort...
  if (selected_path.isEmpty()) return;

  QFileInfo const info(selected_path);

  if ((info.isDir() || info.isFile()) &&
      (info.suffix() != "gpg" && info.suffix() != "pgp" &&
       info.suffix() != "sig" && info.suffix() != "asc")) {
    operation_type |= MainWindow::OperationMenu::kEncrypt;
  }

  if ((info.isDir() || info.isFile()) &&
      (info.suffix() != "gpg" && info.suffix() != "pgp" &&
       info.suffix() != "sig" && info.suffix() != "asc")) {
    operation_type |= MainWindow::OperationMenu::kEncryptAndSign;
  }

  if (info.isFile() && (info.suffix() == "gpg" || info.suffix() == "pgp" ||
                        info.suffix() == "asc")) {
    operation_type |= MainWindow::OperationMenu::kDecrypt;
    operation_type |= MainWindow::OperationMenu::kDecryptAndVerify;
  }

  if (info.isFile() && (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                        info.suffix() != "sig" && info.suffix() != "asc")) {
    operation_type |= MainWindow::OperationMenu::kSign;
  }

  if (info.isFile() && (info.suffix() == "sig" || info.suffix() == "gpg" ||
                        info.suffix() == "pgp" || info.suffix() == "asc")) {
    operation_type |= MainWindow::OperationMenu::kVerify;
  }

  if (info.isFile() && (info.suffix() == "eml")) {
    operation_type |= MainWindow::OperationMenu::kVerifyEMail;
  }

  emit SignalMainWindowlUpdateBasicalOperaMenu(operation_type);
}
}  // namespace GpgFrontend::UI
