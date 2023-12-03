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
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_FilePage>()) {
  ui_->setupUi(this);

  first_parent_ = parent;

  dir_model_ = new QFileSystemModel();
  dir_model_->setRootPath(QDir::currentPath());
  dir_model_->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

  ui_->fileTreeView->setModel(dir_model_);
  ui_->fileTreeView->setColumnWidth(0, 320);
  ui_->fileTreeView->sortByColumn(0, Qt::AscendingOrder);
  m_path_ = std::filesystem::path(dir_model_->rootPath().toStdString());

  create_popup_menu();

  connect(ui_->upPathButton, &QPushButton::clicked, this,
          &FilePage::slot_up_level);
  connect(ui_->refreshButton, &QPushButton::clicked, this,
          &FilePage::SlotGoPath);
  ui_->optionsButton->setMenu(option_popup_menu_);

  ui_->pathEdit->setText(dir_model_->rootPath());

  path_edit_completer_ = new QCompleter(this);
  path_complete_model_ = new QStringListModel();
  path_edit_completer_->setModel(path_complete_model_);
  path_edit_completer_->setCaseSensitivity(Qt::CaseInsensitive);
  path_edit_completer_->setCompletionMode(
      QCompleter::UnfilteredPopupCompletion);
  ui_->pathEdit->setCompleter(path_edit_completer_);

  connect(ui_->fileTreeView, &QTreeView::clicked, this,
          &FilePage::slot_file_tree_view_item_clicked);
  connect(ui_->fileTreeView, &QTreeView::doubleClicked, this,
          &FilePage::slot_file_tree_view_item_double_clicked);
  connect(ui_->fileTreeView, &QTreeView::customContextMenuRequested, this,
          &FilePage::onCustomContextMenu);

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
}

void FilePage::slot_file_tree_view_item_clicked(const QModelIndex& index) {
#ifdef WINDOWS
  selected_path_ = std::filesystem::path(
      dir_model_->fileInfo(index).absoluteFilePath().toStdU16String());
#else
  selected_path_ = std::filesystem::path(
      dir_model_->fileInfo(index).absoluteFilePath().toStdString());
#endif

  m_path_ = selected_path_;
  SPDLOG_DEBUG("selected path: {}", selected_path_.u8string());

  selected_path_ = std::filesystem::path(selected_path_);
  MainWindow::CryptoMenu::OperationType operation_type =
      MainWindow::CryptoMenu::None;

  if (index.isValid()) {
    QFileInfo info(QString::fromStdString(selected_path_.u8string()));

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

    if (info.isFile() && (info.suffix() == "gpg" || info.suffix() == "pgp" ||
                          info.suffix() == "asc")) {
      operation_type |= MainWindow::CryptoMenu::Decrypt;
      operation_type |= MainWindow::CryptoMenu::DecryptAndVerify;
    }

    if (info.isFile() && (info.suffix() != "gpg" && info.suffix() != "pgp" &&
                          info.suffix() != "sig" && info.suffix() != "asc")) {
      operation_type |= MainWindow::CryptoMenu::Sign;
    }

    if (info.isFile() && (info.suffix() == "sig" || info.suffix() == "gpg" ||
                          info.suffix() == "pgp" || info.suffix() == "asc")) {
      operation_type |= MainWindow::CryptoMenu::Verify;
    }
  }

  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SetCryptoMenuStatus(operation_type);
}

void FilePage::slot_up_level() {
  QModelIndex currentRoot = ui_->fileTreeView->rootIndex();
#ifdef WINDOWS
  auto str_path =
      dir_model_->fileInfo(currentRoot).absoluteFilePath().toStdU16String();
#else
  auto str_path = dir_model_->fileInfo(currentRoot)
                      .absoluteFilePath()
                      .toUtf8()
                      .toStdString();
#endif
  std::filesystem::path path_obj(str_path);

  m_path_ = path_obj;
  SPDLOG_DEBUG("get path: {}", m_path_.u8string());
  if (m_path_.has_parent_path() && !m_path_.parent_path().empty()) {
    m_path_ = m_path_.parent_path();
    SPDLOG_DEBUG("parent path: {}", m_path_.u8string());
    ui_->pathEdit->setText(m_path_.u8string().c_str());
    this->SlotGoPath();
  }
}

void FilePage::slot_file_tree_view_item_double_clicked(
    const QModelIndex& index) {
  QFileInfo file_info(dir_model_->fileInfo(index).absoluteFilePath());
  if (file_info.isFile()) {
    slot_open_item();
  } else {
    ui_->pathEdit->setText(file_info.filePath());
    SlotGoPath();
  }
}

QString FilePage::GetSelected() const {
  return QString::fromStdString(selected_path_.u8string());
}

void FilePage::SlotGoPath() {
#ifdef WINDOWS
  std::filesystem::path path_edit_obj(ui_->pathEdit->text().toStdU16String());
#else
  std::filesystem::path path_edit_obj(ui_->pathEdit->text().toStdString());
#endif

  m_path_ = m_path_ != path_edit_obj ? path_edit_obj : m_path_;
  auto fileInfo = QFileInfo(m_path_.string().c_str());
  if (fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
#ifdef WINDOWS
    m_path_ = std::filesystem::path(fileInfo.filePath().toStdU16String());
#else
    m_path_ = std::filesystem::path(fileInfo.filePath().toStdString());
#endif

    SPDLOG_DEBUG("set path: {}", m_path_.u8string());
    ui_->fileTreeView->setRootIndex(dir_model_->index(fileInfo.filePath()));
    dir_model_->setRootPath(fileInfo.filePath());
    for (int i = 1; i < dir_model_->columnCount(); ++i) {
      ui_->fileTreeView->resizeColumnToContents(i);
    }
    ui_->pathEdit->setText(QString::fromStdString(m_path_.u8string()));
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The path is not exists, unprivileged or unreachable."));
  }
  emit SignalPathChanged(QString::fromStdString(m_path_.u8string()));
}

void FilePage::create_popup_menu() {
  popup_menu_ = new QMenu();

  ui_->actionOpenFile->setText(_("Open"));
  connect(ui_->actionOpenFile, &QAction::triggered, this,
          &FilePage::slot_open_item);
  ui_->actionRenameFile->setText(_("Rename"));
  connect(ui_->actionRenameFile, &QAction::triggered, this,
          &FilePage::slot_rename_item);
  ui_->actionDeleteFile->setText(_("Delete"));
  connect(ui_->actionDeleteFile, &QAction::triggered, this,
          &FilePage::slot_delete_item);

  ui_->actionCalculateHash->setText(_("Calculate Hash"));
  connect(ui_->actionCalculateHash, &QAction::triggered, this,
          &FilePage::slot_calculate_hash);

  ui_->actionMakeDirectory->setText(_("Directory"));
  connect(ui_->actionMakeDirectory, &QAction::triggered, this,
          &FilePage::slot_mkdir);

  ui_->actionCreateEmptyFile->setText(_("File"));
  connect(ui_->actionCreateEmptyFile, &QAction::triggered, this,
          &FilePage::slot_create_empty_file);

  ui_->actionCompressFiles->setText(_("Compress..."));
  ui_->actionCompressFiles->setVisible(false);
  connect(ui_->actionCompressFiles, &QAction::triggered, this,
          &FilePage::slot_compress_files);

  ui_->actionOpenWithSystemDefaultApplication->setText(
      _("Open with Default System Application"));
  connect(ui_->actionOpenWithSystemDefaultApplication, &QAction::triggered,
          this, &FilePage::slot_open_item_by_system_application);

  auto new_item_action_menu = new QMenu(this);
  new_item_action_menu->setTitle(_("New"));
  new_item_action_menu->addAction(ui_->actionCreateEmptyFile);
  new_item_action_menu->addAction(ui_->actionMakeDirectory);

  popup_menu_->addAction(ui_->actionOpenFile);
  popup_menu_->addAction(ui_->actionOpenWithSystemDefaultApplication);

  popup_menu_->addSeparator();
  popup_menu_->addMenu(new_item_action_menu);
  popup_menu_->addSeparator();

  popup_menu_->addAction(ui_->actionRenameFile);
  popup_menu_->addAction(ui_->actionDeleteFile);
  popup_menu_->addAction(ui_->actionCompressFiles);
  popup_menu_->addAction(ui_->actionCalculateHash);

  option_popup_menu_ = new QMenu();

  auto showHiddenAct = new QAction(_("Show Hidden File"), this);
  showHiddenAct->setCheckable(true);
  connect(showHiddenAct, &QAction::triggered, this, [&](bool checked) {
    SPDLOG_DEBUG("set hidden: {}", checked);
    if (checked)
      dir_model_->setFilter(dir_model_->filter() | QDir::Hidden);
    else
      dir_model_->setFilter(dir_model_->filter() & ~QDir::Hidden);
    dir_model_->setRootPath(m_path_.u8string().c_str());
  });
  option_popup_menu_->addAction(showHiddenAct);

  auto showSystemAct = new QAction(_("Show System File"), this);
  showSystemAct->setCheckable(true);
  connect(showSystemAct, &QAction::triggered, this, [&](bool checked) {
    SPDLOG_DEBUG("set hidden: {}", checked);
    if (checked)
      dir_model_->setFilter(dir_model_->filter() | QDir::System);
    else
      dir_model_->setFilter(dir_model_->filter() & ~QDir::System);
    dir_model_->setRootPath(m_path_.u8string().c_str());
  });
  option_popup_menu_->addAction(showSystemAct);
}

void FilePage::onCustomContextMenu(const QPoint& point) {
  QModelIndex index = ui_->fileTreeView->indexAt(point);
  SPDLOG_DEBUG("right click: {}", selected_path_.u8string());

#ifdef WINDOWS
  auto index_dir_str =
      dir_model_->fileInfo(index).absoluteFilePath().toStdU16String();
#else
  auto index_dir_str =
      dir_model_->fileInfo(index).absoluteFilePath().toStdString();
#endif

  selected_path_ = std::filesystem::path(index_dir_str);

  // update crypt menu
  slot_file_tree_view_item_clicked(index);

  if (index.isValid()) {
    ui_->actionOpenFile->setEnabled(true);
    ui_->actionRenameFile->setEnabled(true);
    ui_->actionDeleteFile->setEnabled(true);

    QFileInfo info(QString::fromStdString(selected_path_.u8string()));
    ui_->actionCalculateHash->setEnabled(info.isFile() && info.isReadable());
  } else {
    ui_->actionOpenFile->setEnabled(false);
    ui_->actionRenameFile->setEnabled(false);
    ui_->actionDeleteFile->setEnabled(false);

    ui_->actionCalculateHash->setEnabled(false);
  }
  popup_menu_->exec(ui_->fileTreeView->viewport()->mapToGlobal(point));
}

void FilePage::slot_open_item() {
  QFileInfo info(QString::fromStdString(selected_path_.u8string()));
  if (info.isDir()) {
    if (info.isReadable() && info.isExecutable()) {
      const auto file_path = info.filePath().toUtf8().toStdString();
      SPDLOG_DEBUG("set path: {}", file_path);
      ui_->pathEdit->setText(info.filePath().toUtf8());
      SlotGoPath();
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The directory is unprivileged or unreachable."));
    }
  } else {
    if (info.isReadable()) {
      // handle normal text or binary file
      auto main_window = qobject_cast<MainWindow*>(first_parent_);
      auto qt_open_path = QString::fromStdString(selected_path_.u8string());
      SPDLOG_DEBUG("open item: {}", qt_open_path.toStdString());
      if (main_window != nullptr) main_window->SlotOpenFile(qt_open_path);
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The file is unprivileged or unreachable."));
    }
  }
}

void FilePage::slot_open_item_by_system_application() {
  QFileInfo info(QString::fromStdString(selected_path_.u8string()));
  auto q_selected_path = QString::fromStdString(selected_path_.u8string());
  if (info.isDir()) {
    const auto file_path = info.filePath().toUtf8().toStdString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(q_selected_path));

  } else {
    QDesktopServices::openUrl(QUrl::fromLocalFile(q_selected_path));
  }
}

void FilePage::slot_rename_item() {
  auto new_name_path = selected_path_, old_name_path = selected_path_;
  auto old_name = old_name_path.filename();
  new_name_path = new_name_path.remove_filename();

  bool ok;
  auto text = QInputDialog::getText(
      this, _("Rename"), _("New Filename"), QLineEdit::Normal,
      QString::fromStdString(old_name.u8string()), &ok);
  if (ok && !text.isEmpty()) {
    try {
#ifdef WINDOWS
      new_name_path /= text.toStdU16String();
#else
      new_name_path /= text.toStdString();
#endif
      SPDLOG_DEBUG("new name path: {}", new_name_path.u8string());
      std::filesystem::rename(old_name_path, new_name_path);
      // refresh
      this->SlotGoPath();
    } catch (...) {
      SPDLOG_ERROR("rename error: {}", new_name_path.u8string());
      QMessageBox::critical(this, _("Error"),
                            _("Unable to rename the file or folder."));
    }
  }
}

void FilePage::slot_delete_item() {
  QModelIndex index = ui_->fileTreeView->currentIndex();
  QVariant data = ui_->fileTreeView->model()->data(index);

  auto ret = QMessageBox::warning(this, _("Warning"),
                                  _("Are you sure you want to delete it?"),
                                  QMessageBox::Ok | QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  SPDLOG_DEBUG("delete item: {}", data.toString().toStdString());

  if (!dir_model_->remove(index)) {
    QMessageBox::critical(this, _("Error"),
                          _("Unable to delete the file or folder."));
  }
}

void FilePage::slot_calculate_hash() {
  auto info_str = CalculateHash(selected_path_);
  emit SignalRefreshInfoBoard(info_str.c_str(), InfoBoardStatus::INFO_ERROR_OK);
}

void FilePage::slot_mkdir() {
  auto index = ui_->fileTreeView->rootIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name =
      QInputDialog::getText(this, _("Make New Directory"), _("Directory Name"),
                            QLineEdit::Normal, new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dir_model_->mkdir(index, new_dir_name);
  }
}

void FilePage::slot_create_empty_file() {
#ifdef WINDOWS
  auto root_path_str = dir_model_->rootPath().toStdU16String();
#else
  auto root_path_str = dir_model_->rootPath().toStdString();
#endif
  std::filesystem::path root_path(root_path_str);

  QString new_file_name;
  bool ok;
  new_file_name = QInputDialog::getText(this, _("Create Empty File"),
                                        _("Filename (you can given extension)"),
                                        QLineEdit::Normal, new_file_name, &ok);
  if (ok && !new_file_name.isEmpty()) {
#ifdef WINDOWS
    auto file_path = root_path / new_file_name.toStdU16String();
#else
    auto file_path = root_path / new_file_name.toStdString();
#endif
    QFile new_file(file_path.u8string().c_str());
    if (!new_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
      QMessageBox::critical(this, _("Error"), _("Unable to create the file."));
    }
    new_file.close();
  }
}

void FilePage::keyPressEvent(QKeyEvent* event) {
  SPDLOG_DEBUG("key press: {}", event->key());
  if (ui_->pathEdit->hasFocus() &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    SlotGoPath();
  } else if (ui_->fileTreeView->currentIndex().isValid()) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
      slot_open_item();
    else if (event->key() == Qt::Key_Delete ||
             event->key() == Qt::Key_Backspace)
      slot_delete_item();
  }
}

void FilePage::slot_compress_files() {}

}  // namespace GpgFrontend::UI
