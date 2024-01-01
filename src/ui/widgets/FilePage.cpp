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

#include "ui/widgets/FilePage.h"

#include <string>

#include "core/GpgModel.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/function/gpg/GpgFileOpera.h"
#include "ui/UISignalStation.h"
#include "ui/main_window/MainWindow.h"
#include "ui_FilePage.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_FilePage>()),
      file_tree_view_(new FileTreeView(this)) {
  ui_->setupUi(this);
  ui_->trewViewLayout->addWidget(file_tree_view_);

  connect(ui_->upPathButton, &QPushButton::clicked, file_tree_view_,
          &FileTreeView::SlotUpLevel);
  connect(ui_->refreshButton, &QPushButton::clicked, this,
          &FilePage::SlotGoPath);
  connect(this->ui_->newDirButton, &QPushButton::clicked, file_tree_view_,
          &FileTreeView::SlotMkdir);

  ui_->pathEdit->setText(
      QString::fromStdString(file_tree_view_->GetCurrentPath().u8string()));

  path_edit_completer_ = new QCompleter(this);
  path_complete_model_ = new QStringListModel();
  path_edit_completer_->setModel(path_complete_model_);
  path_edit_completer_->setCaseSensitivity(Qt::CaseInsensitive);
  path_edit_completer_->setCompletionMode(
      QCompleter::UnfilteredPopupCompletion);
  ui_->pathEdit->setCompleter(path_edit_completer_);

  option_popup_menu_ = new QMenu(this);
  auto* show_hidden_act = new QAction(_("Show Hidden File"), this);
  show_hidden_act->setCheckable(true);
  connect(show_hidden_act, &QAction::triggered, file_tree_view_,
          &FileTreeView::SlotShowHiddenFile);
  option_popup_menu_->addAction(show_hidden_act);

  auto* show_system_act = new QAction(_("Show System File"), this);
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

  auto* main_window = qobject_cast<MainWindow*>(this->parent());
  if (main_window != nullptr) {
    connect(file_tree_view_, &FileTreeView::SignalOpenFile, main_window,
            &MainWindow::SlotOpenFile);

    connect(file_tree_view_, &FileTreeView::SignalSelectedChanged, this,
            [main_window](const QString& selected_path) {
              MainWindow::CryptoMenu::OperationType operation_type =
                  MainWindow::CryptoMenu::None;

              // abort...
              if (selected_path.isEmpty()) return;

              QFileInfo const info(selected_path);

              if ((info.isDir() || info.isFile()) &&
                  (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                   info.suffix() != "sig" && info.suffix() != "asc")) {
                operation_type |= MainWindow::CryptoMenu::Encrypt;
              }

              if ((info.isDir() || info.isFile()) &&
                  (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                   info.suffix() != "sig" && info.suffix() != "asc")) {
                operation_type |= MainWindow::CryptoMenu::EncryptAndSign;
              }

              if (info.isFile() &&
                  (info.suffix() == "gpg" || info.suffix() == "pgp" ||
                   info.suffix() == "asc")) {
                operation_type |= MainWindow::CryptoMenu::Decrypt;
                operation_type |= MainWindow::CryptoMenu::DecryptAndVerify;
              }

              if (info.isFile() &&
                  (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                   info.suffix() != "sig" && info.suffix() != "asc")) {
                operation_type |= MainWindow::CryptoMenu::Sign;
              }

              if (info.isFile() &&
                  (info.suffix() == "sig" || info.suffix() == "gpg" ||
                   info.suffix() == "pgp" || info.suffix() == "asc")) {
                operation_type |= MainWindow::CryptoMenu::Verify;
              }

              main_window->SetCryptoMenuStatus(operation_type);
            });
  }
}

auto FilePage::GetSelected() const -> QString {
  return QString::fromStdString(file_tree_view_->GetSelectedPath().string());
}

void FilePage::SlotGoPath() {
#ifdef WINDOWS
  std::filesystem::path target_path(ui_->pathEdit->text().toStdU16String());
#else
  std::filesystem::path target_path(ui_->pathEdit->text().toStdString());
#endif
  file_tree_view_->SlotGoPath(target_path);
}

void FilePage::keyPressEvent(QKeyEvent* event) {
  SPDLOG_DEBUG("file page notices key press by user: {}", event->key());
  if (ui_->pathEdit->hasFocus() &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    SlotGoPath();
  }
}

}  // namespace GpgFrontend::UI
