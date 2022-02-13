/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/widgets/FilePage.h"

#include <string>

#include "main_window/MainWindow.h"
#include "ui/SignalStation.h"
#include "ui_FilePage.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent)
    : QWidget(parent), ui_(std::make_shared<Ui_FilePage>()) {
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

  connect(this, &FilePage::SignalRefreshInfoBoard, SignalStation::GetInstance(),
          &SignalStation::SignalRefreshInfoBoard);
}

void FilePage::slot_file_tree_view_item_clicked(const QModelIndex& index) {
  selected_path_ = std::filesystem::path(
      dir_model_->fileInfo(index).absoluteFilePath().toStdString());
  m_path_ = selected_path_;
  LOG(INFO) << "selected path" << selected_path_;

  selected_path_ = std::filesystem::path(selected_path_);
  MainWindow::CryptoMenu::OperationType operation_type = MainWindow::CryptoMenu::None;
  
  if (index.isValid()) {
    QFileInfo info(QString::fromStdString(selected_path_.string()));


    if(info.isFile() && (info.suffix() != "gpg" &&
                          info.suffix() != "sig" &&
                          info.suffix() != "asc")) {
      operation_type |= MainWindow::CryptoMenu::Encrypt;
    }

    if(info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig" &&
                          info.suffix() != "asc")){
      operation_type |= MainWindow::CryptoMenu::EncryptAndSign;
    }

    if(info.isFile() && (info.suffix() == "gpg" || info.suffix() == "asc")) {
      operation_type |= MainWindow::CryptoMenu::Decrypt;
      operation_type |= MainWindow::CryptoMenu::DecryptAndVerify;
    }

    if(info.isFile() && (info.suffix() != "gpg" &&
                          info.suffix() != "sig" &&
                          info.suffix() != "asc")){
      operation_type |= MainWindow::CryptoMenu::Sign;
    }

    if(info.isFile() && (info.suffix() == "sig" ||
                          info.suffix() == "gpg" ||
                          info.suffix() == "asc")) {
      operation_type |= MainWindow::CryptoMenu::Verify;
    }
  }

  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SetCryptoMenuStatus(operation_type);
}

void FilePage::slot_up_level() {
  QModelIndex currentRoot = ui_->fileTreeView->rootIndex();

  auto utf8_path =
      dir_model_->fileInfo(currentRoot).absoluteFilePath().toStdString();
  std::filesystem::path path_obj(utf8_path);

  m_path_ = path_obj;
  LOG(INFO) << "get path" << m_path_;
  if (m_path_.has_parent_path() && !m_path_.parent_path().empty()) {
    m_path_ = m_path_.parent_path();
    LOG(INFO) << "parent path" << m_path_;
    ui_->pathEdit->setText(m_path_.string().c_str());
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
  return QString::fromStdString(selected_path_.string());
}

void FilePage::SlotGoPath() {
  const auto path_edit = ui_->pathEdit->text().toStdString();
  std::filesystem::path path_obj(path_edit);

  if (m_path_.string() != path_edit) m_path_ = path_obj;
  auto fileInfo = QFileInfo(m_path_.string().c_str());
  if (fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
    m_path_ = std::filesystem::path(fileInfo.filePath().toStdString());
    LOG(INFO) << "set path" << m_path_;
    ui_->fileTreeView->setRootIndex(dir_model_->index(fileInfo.filePath()));
    dir_model_->setRootPath(fileInfo.filePath());
    for (int i = 1; i < dir_model_->columnCount(); ++i) {
      ui_->fileTreeView->resizeColumnToContents(i);
    }
    ui_->pathEdit->setText(m_path_.generic_string().c_str());
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The path is not exists, unprivileged or unreachable."));
  }
  emit SignalPathChanged(m_path_.string().c_str());
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

  ui_->actionEncrypt->setText(_("Encrypt"));
  connect(ui_->actionEncrypt, &QAction::triggered, this,
          &FilePage::slot_encrypt_item);
  ui_->actionEncryptSign->setText(_("Encrypt Sign"));
  connect(ui_->actionEncryptSign, &QAction::triggered, this,
          &FilePage::slot_encrypt_sign_item);
  ui_->actionDecrypt->setText(QString(_("Decrypt Verify")) + " " +
                              _("(.gpg .asc)"));
  connect(ui_->actionDecrypt, &QAction::triggered, this,
          &FilePage::slot_decrypt_item);
  ui_->actionSign->setText(_("Sign"));
  connect(ui_->actionSign, &QAction::triggered, this,
          &FilePage::slot_sign_item);
  ui_->actionVerify->setText(QString(_("Verify")) + " " +
                             _("(.sig .gpg .asc)"));
  connect(ui_->actionVerify, &QAction::triggered, this,
          &FilePage::slot_verify_item);

  ui_->actionCalculateHash->setText(_("Calculate Hash"));
  connect(ui_->actionCalculateHash, &QAction::triggered, this,
          &FilePage::slot_calculate_hash);

  ui_->actionMakeDirectory->setText(_("Make New Directory"));
  connect(ui_->actionMakeDirectory, &QAction::triggered, this,
          &FilePage::slot_mkdir);

  ui_->actionCreateEmptyFile->setText(_("Create Empty File"));
  connect(ui_->actionCreateEmptyFile, &QAction::triggered, this,
          &FilePage::slot_create_empty_file);

  popup_menu_->addAction(ui_->actionOpenFile);
  popup_menu_->addAction(ui_->actionRenameFile);
  popup_menu_->addAction(ui_->actionDeleteFile);
  popup_menu_->addSeparator();
  popup_menu_->addAction(ui_->actionEncrypt);
  popup_menu_->addAction(ui_->actionEncryptSign);
  popup_menu_->addAction(ui_->actionDecrypt);
  popup_menu_->addAction(ui_->actionSign);
  popup_menu_->addAction(ui_->actionVerify);
  popup_menu_->addSeparator();
  popup_menu_->addAction(ui_->actionMakeDirectory);
  popup_menu_->addAction(ui_->actionCreateEmptyFile);
  popup_menu_->addAction(ui_->actionCalculateHash);

  option_popup_menu_ = new QMenu();

  auto showHiddenAct = new QAction(_("Show Hidden File"), this);
  showHiddenAct->setCheckable(true);
  connect(showHiddenAct, &QAction::triggered, this, [&](bool checked) {
    LOG(INFO) << "Set Hidden" << checked;
    if (checked)
      dir_model_->setFilter(dir_model_->filter() | QDir::Hidden);
    else
      dir_model_->setFilter(dir_model_->filter() & ~QDir::Hidden);
    dir_model_->setRootPath(m_path_.string().c_str());
  });
  option_popup_menu_->addAction(showHiddenAct);

  auto showSystemAct = new QAction(_("Show System File"), this);
  showSystemAct->setCheckable(true);
  connect(showSystemAct, &QAction::triggered, this, [&](bool checked) {
    LOG(INFO) << "Set Hidden" << checked;
    if (checked)
      dir_model_->setFilter(dir_model_->filter() | QDir::System);
    else
      dir_model_->setFilter(dir_model_->filter() & ~QDir::System);
    dir_model_->setRootPath(m_path_.string().c_str());
  });
  option_popup_menu_->addAction(showSystemAct);
}

void FilePage::onCustomContextMenu(const QPoint& point) {
  QModelIndex index = ui_->fileTreeView->indexAt(point);
  selected_path_ = std::filesystem::path(
      dir_model_->fileInfo(index).absoluteFilePath().toStdString());
  LOG(INFO) << "right click" << selected_path_;
  if (index.isValid()) {
    ui_->actionOpenFile->setEnabled(true);
    ui_->actionRenameFile->setEnabled(true);
    ui_->actionDeleteFile->setEnabled(true);

    QFileInfo info(QString::fromStdString(selected_path_.string()));
    ui_->actionEncrypt->setEnabled(info.isFile() && (info.suffix() != "gpg" &&
                                                     info.suffix() != "sig" &&
                                                     info.suffix() != "asc"));
    ui_->actionEncryptSign->setEnabled(
        info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig" &&
                          info.suffix() != "asc"));
    ui_->actionDecrypt->setEnabled(
        info.isFile() && (info.suffix() == "gpg" || info.suffix() == "asc"));
    ui_->actionSign->setEnabled(info.isFile() && (info.suffix() != "gpg" &&
                                                  info.suffix() != "sig" &&
                                                  info.suffix() != "asc"));
    ui_->actionVerify->setEnabled(info.isFile() && (info.suffix() == "sig" ||
                                                    info.suffix() == "gpg" ||
                                                    info.suffix() == "asc"));
    ui_->actionCalculateHash->setEnabled(info.isFile() && info.isReadable());
  } else {
    ui_->actionOpenFile->setEnabled(false);
    ui_->actionRenameFile->setEnabled(false);
    ui_->actionDeleteFile->setEnabled(false);

    ui_->actionEncrypt->setEnabled(false);
    ui_->actionEncryptSign->setEnabled(false);
    ui_->actionDecrypt->setEnabled(false);
    ui_->actionSign->setEnabled(false);
    ui_->actionVerify->setEnabled(false);
    ui_->actionCalculateHash->setEnabled(false);
  }
  popup_menu_->exec(ui_->fileTreeView->viewport()->mapToGlobal(point));
}

void FilePage::slot_open_item() {
  QFileInfo info(QString::fromStdString(selected_path_.string()));
  if (info.isDir()) {
    if (info.isReadable() && info.isExecutable()) {
      const auto file_path = info.filePath().toStdString();
      LOG(INFO) << "set path" << file_path;
      ui_->pathEdit->setText(info.filePath());
      SlotGoPath();
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The directory is unprivileged or unreachable."));
    }
  } else {
    if (info.isReadable()) {
      auto main_window = qobject_cast<MainWindow*>(first_parent_);
      LOG(INFO) << "open item" << selected_path_;
      auto qt_path = QString::fromStdString(selected_path_.string());
      if (main_window != nullptr) main_window->SlotOpenFile(qt_path);
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The file is unprivileged or unreachable."));
    }
  }
}

void FilePage::slot_rename_item() {
  auto new_name_path = selected_path_, old_name_path = selected_path_;
  auto old_name = old_name_path.filename();
  new_name_path = new_name_path.remove_filename();

  bool ok;
  auto text =
      QInputDialog::getText(this, _("Rename"), _("New Filename"),
                            QLineEdit::Normal, old_name.string().c_str(), &ok);
  if (ok && !text.isEmpty()) {
    try {
      new_name_path /= text.toStdString();
      LOG(INFO) << "new name path" << new_name_path;
      std::filesystem::rename(old_name_path, new_name_path);
      // refresh
      this->SlotGoPath();
    } catch (...) {
      LOG(ERROR) << "rename error" << new_name_path;
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

  LOG(INFO) << "Delete Item" << data.toString().toStdString();

  if (!dir_model_->remove(index)) {
    QMessageBox::critical(this, _("Error"),
                          _("Unable to delete the file or folder."));
  }
}

void FilePage::slot_encrypt_item() {
  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SlotFileEncrypt();
}

void FilePage::slot_encrypt_sign_item() {
  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SlotFileEncryptSign();
}

void FilePage::slot_decrypt_item() {
  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SlotFileDecryptVerify();
}

void FilePage::slot_sign_item() {
  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SlotFileSign();
}

void FilePage::slot_verify_item() {
  auto main_window = qobject_cast<MainWindow*>(first_parent_);
  if (main_window != nullptr) main_window->SlotFileVerify();
}

void FilePage::slot_calculate_hash() {
  // Returns empty QByteArray() on failure.
  QFileInfo info(QString::fromStdString(selected_path_.string()));

  if (info.isFile() && info.isReadable()) {
    std::stringstream ss;

    ss << "[#] " << _("File Hash Information") << std::endl;
    ss << "    " << _("filename") << _(": ")
       << selected_path_.filename().string().c_str() << std::endl;

    QFile f(info.filePath());
    f.open(QFile::ReadOnly);
    auto buffer = f.readAll();
    LOG(INFO) << "buffer size" << buffer.size();
    f.close();
    if (f.open(QFile::ReadOnly)) {
      auto hash_md5 = QCryptographicHash(QCryptographicHash::Md5);
      // md5
      hash_md5.addData(buffer);
      auto md5 = hash_md5.result().toHex().toStdString();
      LOG(INFO) << "md5" << md5;
      ss << "    "
         << "md5" << _(": ") << md5 << std::endl;

      auto hash_sha1 = QCryptographicHash(QCryptographicHash::Sha1);
      // sha1
      hash_sha1.addData(buffer);
      auto sha1 = hash_sha1.result().toHex().toStdString();
      LOG(INFO) << "sha1" << sha1;
      ss << "    "
         << "sha1" << _(": ") << sha1 << std::endl;

      auto hash_sha256 = QCryptographicHash(QCryptographicHash::Sha256);
      // sha1
      hash_sha256.addData(buffer);
      auto sha256 = hash_sha256.result().toHex().toStdString();
      LOG(INFO) << "sha256" << sha256;
      ss << "    "
         << "sha256" << _(": ") << sha256 << std::endl;

      ss << std::endl;

      emit SignalRefreshInfoBoard(ss.str().c_str(),
                                  InfoBoardStatus::INFO_ERROR_OK);
    }
  }
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
  auto root_path_str = dir_model_->rootPath().toStdString();
  std::filesystem::path root_path(root_path_str);

  QString new_file_name;
  bool ok;
  new_file_name = QInputDialog::getText(this, _("Create Empty File"),
                                        _("Filename (you can given extension)"),
                                        QLineEdit::Normal, new_file_name, &ok);
  if (ok && !new_file_name.isEmpty()) {
    auto file_path = root_path / new_file_name.toStdString();
    QFile new_file(file_path.string().c_str());
    if (!new_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
      QMessageBox::critical(this, _("Error"), _("Unable to create the file."));
    }
    new_file.close();
  }
}

void FilePage::keyPressEvent(QKeyEvent* event) {
  LOG(INFO) << "Key Press" << event->key();
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

}  // namespace GpgFrontend::UI
