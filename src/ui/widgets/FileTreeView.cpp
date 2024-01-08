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

#include "FileTreeView.h"

#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

FileTreeView::FileTreeView(QWidget* parent, const QString& target_path)
    : QTreeView(parent) {
  dir_model_ = new QFileSystemModel();
  dir_model_->setRootPath(target_path.isEmpty() ? QDir::currentPath()
                                                : target_path);
  dir_model_->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

  this->setModel(dir_model_);
  this->setColumnWidth(0, 320);
  this->sortByColumn(0, Qt::AscendingOrder);
  current_path_ = std::filesystem::path(dir_model_->rootPath().toStdString());

  slot_create_popup_menu();
  this->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested, this,
          &FileTreeView::slot_show_custom_context_menu);
  connect(this, &QTreeView::doubleClicked, this,
          &FileTreeView::slot_file_tree_view_item_double_clicked);
}

void FileTreeView::selectionChanged(const QItemSelection& selected,
                                    const QItemSelection& deselected) {
  QTreeView::selectionChanged(selected, deselected);

  if (!selected.indexes().empty()) {
    selected_path_ = dir_model_->fileInfo(selected.indexes().first())
                         .filesystemAbsoluteFilePath();
    GF_UI_LOG_DEBUG("file tree view selected target path: {}",
                    selected_path_.u8string());
    emit SignalSelectedChanged(QString::fromStdString(selected_path_));
  } else {
    selected_path_ = std::filesystem::path{};
  }
}

void FileTreeView::SlotGoPath(const std::filesystem::path& target_path) {
  auto file_info = QFileInfo(target_path);
  if (file_info.isDir() && file_info.isReadable() && file_info.isExecutable()) {
    current_path_ = file_info.filesystemAbsoluteFilePath();
    GF_UI_LOG_DEBUG("file tree view set target path: {}",
                    current_path_.u8string());
    this->setRootIndex(dir_model_->index(file_info.filePath()));
    dir_model_->setRootPath(file_info.filePath());
    for (int i = 1; i < dir_model_->columnCount(); ++i) {
      this->resizeColumnToContents(i);
    }
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The path is not exists, unprivileged or unreachable."));
  }
  emit SignalPathChanged(QString::fromStdString(current_path_.u8string()));
}

void FileTreeView::slot_file_tree_view_item_double_clicked(
    const QModelIndex& index) {
  QFileInfo const file_info(dir_model_->fileInfo(index).absoluteFilePath());
  if (file_info.isFile()) {
    if (file_info.isReadable()) {
      emit SignalOpenFile(file_info.absoluteFilePath());
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The file is unprivileged or unreachable."));
    }
  } else {
    SlotGoPath(file_info.filesystemAbsoluteFilePath());
  }
}

void FileTreeView::SlotUpLevel() {
  QModelIndex const current_root = this->rootIndex();

  auto target_path =
      dir_model_->fileInfo(current_root).filesystemAbsoluteFilePath();
  if (target_path.has_parent_path() && !target_path.parent_path().empty()) {
    target_path = target_path.parent_path();
    GF_UI_LOG_DEBUG("file tree view go parent path: {}",
                    target_path.u8string());
    this->SlotGoPath(target_path);
  }
  current_path_ = target_path;
}

auto FileTreeView::GetCurrentPath() -> std::filesystem::path {
  return current_path_;
}

void FileTreeView::SlotShowSystemFile(bool on) {
  auto filters = on ? dir_model_->filter() | QDir::System
                    : dir_model_->filter() & ~QDir::System;
  dir_model_->setFilter(filters);
  dir_model_->setRootPath(QString::fromStdString(current_path_.u8string()));
}

void FileTreeView::SlotShowHiddenFile(bool on) {
  auto filters = on ? dir_model_->filter() | QDir::Hidden
                    : dir_model_->filter() & ~QDir::Hidden;
  dir_model_->setFilter(filters);
  dir_model_->setRootPath(QString::fromStdString(current_path_.u8string()));
}

auto FileTreeView::GetPathByClickPoint(const QPoint& point)
    -> std::filesystem::path {
  auto const index = this->indexAt(point);

  if (!index.isValid()) {
    return {};
  }

  auto index_path = dir_model_->fileInfo(index).filesystemAbsoluteFilePath();
  GF_UI_LOG_DEBUG("file tree view right click on: {}", index_path.string());
  return index_path;
}

auto FileTreeView::GetSelectedPath() -> std::filesystem::path {
  return selected_path_;
}

auto FileTreeView::SlotDeleteSelectedItem() -> void {
  QModelIndex const index = this->currentIndex();
  QVariant const data = this->model()->data(index);

  auto ret = QMessageBox::warning(this, _("Warning"),
                                  _("Are you sure you want to delete it?"),
                                  QMessageBox::Ok | QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  GF_UI_LOG_DEBUG("delete item: {}", data.toString().toStdString());

  if (!dir_model_->remove(index)) {
    QMessageBox::critical(this, _("Error"),
                          _("Unable to delete the file or folder."));
  }
}

void FileTreeView::SlotMkdir() {
  auto index = this->rootIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name =
      QInputDialog::getText(this, _("Make New Directory"), _("Directory Name"),
                            QLineEdit::Normal, new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dir_model_->mkdir(index, new_dir_name);
  }
}

void FileTreeView::SlotMkdirBelowAtSelectedItem() {
  auto index = this->currentIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name =
      QInputDialog::getText(this, _("Make New Directory"), _("Directory Name"),
                            QLineEdit::Normal, new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dir_model_->mkdir(index, new_dir_name);
  }
}

void FileTreeView::SlotTouch() {
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

void FileTreeView::SlotTouchBelowAtSelectedItem() {
  std::filesystem::path root_path(selected_path_);

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

void FileTreeView::keyPressEvent(QKeyEvent* event) {
  QTreeView::keyPressEvent(event);

  if (this->currentIndex().isValid()) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
      slot_file_tree_view_item_double_clicked(this->currentIndex());
    } else if (event->key() == Qt::Key_Delete ||
               event->key() == Qt::Key_Backspace) {
      SlotDeleteSelectedItem();
    }
  }
}

void FileTreeView::SlotOpenSelectedItemBySystemApplication() {
  QFileInfo const info(QString::fromStdString(selected_path_.u8string()));
  auto q_selected_path = QString::fromStdString(selected_path_.u8string());
  if (info.isDir()) {
    const auto file_path = info.filePath().toUtf8().toStdString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(q_selected_path));

  } else {
    QDesktopServices::openUrl(QUrl::fromLocalFile(q_selected_path));
  }
}

void FileTreeView::SlotRenameSelectedItem() {
  bool ok;
  auto text = QInputDialog::getText(
      this, _("Rename"), _("New Filename"), QLineEdit::Normal,
      QString::fromStdString(selected_path_.filename().u8string()), &ok);
  if (ok && !text.isEmpty()) {
    try {
#ifdef WINDOWS
      auto new_name_path = selected_path_.parent_path() / text.toStdU16String();
#else
      auto new_name_path = selected_path_.parent_path() / text.toStdString();
#endif
      GF_UI_LOG_DEBUG("new name path: {}", new_name_path.u8string());
      std::filesystem::rename(selected_path_, new_name_path);

      // refresh
      SlotGoPath(current_path_);
    } catch (...) {
      GF_UI_LOG_ERROR("file tree view rename error: {}",
                      selected_path_.u8string());
      QMessageBox::critical(this, _("Error"),
                            _("Unable to rename the file or folder."));
    }
  }
}

auto FileTreeView::GetMousePointGlobal(const QPoint& point) -> QPoint {
  return this->viewport()->mapToGlobal(point);
}

void FileTreeView::slot_create_popup_menu() {
  popup_menu_ = new QMenu();

  action_open_file_ = new QAction(this);
  action_open_file_->setText(_("Open"));
  connect(action_open_file_, &QAction::triggered, this, [this](bool) {
    emit SignalOpenFile(QString::fromStdString(GetSelectedPath()));
  });

  action_rename_file_ = new QAction(this);
  action_rename_file_->setText(_("Rename"));
  connect(action_rename_file_, &QAction::triggered, this,
          &FileTreeView::SlotRenameSelectedItem);

  action_delete_file_ = new QAction(this);
  action_delete_file_->setText(_("Delete"));
  connect(action_delete_file_, &QAction::triggered, this,
          &FileTreeView::SlotDeleteSelectedItem);

  action_calculate_hash_ = new QAction(this);
  action_calculate_hash_->setText(_("Calculate Hash"));
  connect(action_calculate_hash_, &QAction::triggered, this,
          &FileTreeView::slot_calculate_hash);

  action_make_directory_ = new QAction(this);
  action_make_directory_->setText(_("Directory"));
  connect(action_make_directory_, &QAction::triggered, this,
          &FileTreeView::SlotMkdirBelowAtSelectedItem);

  action_create_empty_file_ = new QAction(this);
  action_create_empty_file_->setText(_("File"));
  connect(action_create_empty_file_, &QAction::triggered, this,
          &FileTreeView::SlotTouchBelowAtSelectedItem);

  action_compress_files_ = new QAction(this);
  action_compress_files_->setText(_("Compress..."));
  action_compress_files_->setVisible(false);
  connect(action_compress_files_, &QAction::triggered, this,
          &FileTreeView::slot_compress_files);

  auto* action_open_with_system_default_application = new QAction(this);
  action_open_with_system_default_application->setText(
      _("Open with Default System Application"));
  connect(action_open_with_system_default_application, &QAction::triggered,
          this, &FileTreeView::SlotOpenSelectedItemBySystemApplication);

  auto* new_item_action_menu = new QMenu(this);
  new_item_action_menu->setTitle(_("New"));
  new_item_action_menu->addAction(action_create_empty_file_);
  new_item_action_menu->addAction(action_make_directory_);

  popup_menu_->addAction(action_open_file_);
  popup_menu_->addAction(action_open_with_system_default_application);

  popup_menu_->addSeparator();
  popup_menu_->addMenu(new_item_action_menu);
  popup_menu_->addSeparator();

  popup_menu_->addAction(action_rename_file_);
  popup_menu_->addAction(action_delete_file_);
  popup_menu_->addAction(action_compress_files_);
  popup_menu_->addAction(action_calculate_hash_);
}

void FileTreeView::slot_show_custom_context_menu(const QPoint& point) {
  auto target_path = this->GetPathByClickPoint(point);

  if (!target_path.empty()) {
    action_open_file_->setEnabled(true);
    action_rename_file_->setEnabled(true);
    action_delete_file_->setEnabled(true);

    QFileInfo const info(QString::fromStdString(this->GetSelectedPath()));
    action_calculate_hash_->setEnabled(info.isFile() && info.isReadable());

  } else {
    action_open_file_->setEnabled(false);
    action_rename_file_->setEnabled(false);
    action_delete_file_->setEnabled(false);

    action_calculate_hash_->setEnabled(false);
  }
  popup_menu_->exec(this->GetMousePointGlobal(point));
}

void FileTreeView::slot_calculate_hash() {
  emit UISignalStation::GetInstance()->SignalRefreshInfoBoard(
      QString::fromStdString(CalculateHash(this->GetSelectedPath())),
      InfoBoardStatus::INFO_ERROR_OK);
}

void FileTreeView::slot_compress_files() {}

void FileTreeView::paintEvent(QPaintEvent* event) {
  QTreeView::paintEvent(event);
  for (int i = 1; i < dir_model_->columnCount(); ++i) {
    this->resizeColumnToContents(i);
  }
}
}  // namespace GpgFrontend::UI
