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

#include "FileTreeView.h"

#include "core/utils/AsyncUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/function/GpgOperaHelper.h"

namespace GpgFrontend::UI {

FileTreeView::FileTreeView(QWidget* parent)
    : QTreeView(parent), dir_model_(new QFileSystemModel(this)) {
  dir_model_->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
  dir_model_->setRootPath(QDir::homePath());
  current_path_ = dir_model_->rootPath();

  this->setModel(dir_model_);
  this->setColumnWidth(0, 320);
  this->sortByColumn(0, Qt::AscendingOrder);
  this->setSortingEnabled(true);

  slot_create_popup_menu();
  this->setContextMenuPolicy(Qt::CustomContextMenu);
  this->setSelectionMode(QAbstractItemView::SingleSelection);

  connect(this, &QWidget::customContextMenuRequested, this,
          &FileTreeView::slot_show_custom_context_menu);
  connect(this, &QTreeView::doubleClicked, this,
          &FileTreeView::slot_file_tree_view_item_double_clicked);
  connect(dir_model_, &QFileSystemModel::layoutChanged, this,
          &FileTreeView::slot_adjust_column_widths);
  connect(dir_model_, &QFileSystemModel::dataChanged, this,
          &FileTreeView::slot_adjust_column_widths);
}

void FileTreeView::selectionChanged(const QItemSelection& selected,
                                    const QItemSelection& deselected) {
  QTreeView::selectionChanged(selected, deselected);

  selected_paths_.clear();
  if (!this->selectedIndexes().isEmpty()) {
    QSet<QString> paths;
    for (const auto& index : this->selectedIndexes()) {
      const auto path = dir_model_->filePath(index);
      if (path == current_path_) continue;

      paths.insert(path);
    }
    selected_paths_.append(paths.values());
  }

  emit SignalSelectedChanged(selected_paths_);
}

void FileTreeView::SlotGoPath(const QString& target_path) {
  auto file_info = QFileInfo(target_path);
  if (file_info.isDir() && file_info.isReadable() && file_info.isExecutable()) {
    current_path_ = file_info.absoluteFilePath();
    this->setRootIndex(dir_model_->index(file_info.filePath()));
    dir_model_->setRootPath(file_info.filePath());
    slot_adjust_column_widths();
  } else {
    QMessageBox::critical(
        this, tr("Error"),
        tr("The path is not exists, unprivileged or unreachable."));
  }
  emit SignalPathChanged(current_path_);
}

void FileTreeView::slot_file_tree_view_item_double_clicked(
    const QModelIndex& index) {
  QFileInfo const file_info(dir_model_->fileInfo(index).absoluteFilePath());
  if (file_info.isFile()) {
    if (file_info.isReadable()) {
      emit SignalOpenFile(file_info.absoluteFilePath());
    } else {
      QMessageBox::critical(this, tr("Error"),
                            tr("The file is unprivileged or unreachable."));
    }
  } else {
    SlotGoPath(file_info.absoluteFilePath());
  }
}

void FileTreeView::SlotUpLevel() {
  QModelIndex const current_root = this->rootIndex();

  auto target_path = dir_model_->fileInfo(current_root).absoluteFilePath();
  if (auto parent_path = QDir(target_path); parent_path.cdUp()) {
    target_path = parent_path.absolutePath();
    this->SlotGoPath(target_path);
  }
  current_path_ = target_path;
}

auto FileTreeView::GetCurrentPath() -> QString { return current_path_; }

void FileTreeView::SlotShowSystemFile(bool on) {
  auto filters = on ? dir_model_->filter() | QDir::System
                    : dir_model_->filter() & ~QDir::System;
  dir_model_->setFilter(filters);
  dir_model_->setRootPath(current_path_);
}

void FileTreeView::SlotShowHiddenFile(bool on) {
  auto filters = on ? dir_model_->filter() | QDir::Hidden
                    : dir_model_->filter() & ~QDir::Hidden;
  dir_model_->setFilter(filters);
  dir_model_->setRootPath(current_path_);
}

auto FileTreeView::GetPathByClickPoint(const QPoint& point) -> QString {
  auto const index = this->indexAt(point);

  if (!index.isValid()) {
    return {};
  }

  return dir_model_->fileInfo(index).absoluteFilePath();
}

auto FileTreeView::GetSelectedPaths() -> QStringList { return selected_paths_; }

auto FileTreeView::SlotDeleteSelectedItem() -> void {
  QModelIndex const index = this->currentIndex();
  QVariant const data = this->model()->data(index);

  auto ret = QMessageBox::warning(this, tr("Warning"),
                                  tr("Are you sure you want to delete it?"),
                                  QMessageBox::Ok | QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  if (!dir_model_->remove(index)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Unable to delete the file or folder."));
  }
}

void FileTreeView::SlotMkdir() {
  auto index = this->rootIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name = QInputDialog::getText(this, tr("Make New Directory"),
                                       tr("Directory Name"), QLineEdit::Normal,
                                       new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dir_model_->mkdir(index, new_dir_name);
  }
}

void FileTreeView::SlotMkdirBelowAtSelectedItem() {
  auto index = this->currentIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name = QInputDialog::getText(this, tr("Make New Directory"),
                                       tr("Directory Name"), QLineEdit::Normal,
                                       new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dir_model_->mkdir(index, new_dir_name);
  }
}

void FileTreeView::SlotTouch() {
  auto root_path = dir_model_->rootPath();

  QString new_file_name;
  bool ok;

  new_file_name = QInputDialog::getText(
      this, tr("Create Empty File"), tr("Filename (you can given extension)"),
      QLineEdit::Normal, new_file_name, &ok);
  if (ok && !new_file_name.isEmpty()) {
    auto file_path = root_path + "/" + new_file_name;

    QFile new_file(file_path);
    if (!new_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
      QMessageBox::critical(this, tr("Error"),
                            tr("Unable to create the file."));
    }
    new_file.close();
  }
}

void FileTreeView::SlotTouchBelowAtSelectedItem() {
  if (selected_paths_.size() != 1) return;

  auto root_path(selected_paths_.front());
  if (root_path.isEmpty()) root_path = dir_model_->rootPath();

  QString new_file_name;
  bool ok;
  new_file_name = QInputDialog::getText(
      this, tr("Create Empty File"), tr("Filename (you can given extension)"),
      QLineEdit::Normal, new_file_name, &ok);
  if (ok && !new_file_name.isEmpty()) {
    auto file_path = root_path + "/" + new_file_name;

    QFile new_file(file_path);
    if (!new_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
      QMessageBox::critical(this, tr("Error"),
                            tr("Unable to create the file."));
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
  if (selected_paths_.size() != 1) return;

  auto selected_path = selected_paths_.front();
  QFileInfo const info(selected_path);
  if (info.isDir()) {
    const auto file_path = info.filePath().toUtf8();
    QDesktopServices::openUrl(QUrl::fromLocalFile(selected_path));

  } else {
    QDesktopServices::openUrl(QUrl::fromLocalFile(selected_path));
  }
}

void FileTreeView::SlotRenameSelectedItem() {
  if (selected_paths_.size() != 1) return;

  bool ok;
  auto selected_path = selected_paths_.front();
  auto text = QInputDialog::getText(this, tr("Rename"), tr("New Filename"),
                                    QLineEdit::Normal,
                                    QFileInfo(selected_path).fileName(), &ok);
  if (ok && !text.isEmpty()) {
    auto file_info = QFileInfo(selected_path);
    auto new_name_path = file_info.absolutePath() + "/" + text;

    if (!QDir().rename(file_info.absoluteFilePath(), new_name_path)) {
      QMessageBox::critical(this, tr("Error"),
                            tr("Unable to rename the file or folder."));
      return;
    }

    // refresh
    SlotGoPath(current_path_);
  }
}

auto FileTreeView::GetMousePointGlobal(const QPoint& point) -> QPoint {
  return this->viewport()->mapToGlobal(point);
}

void FileTreeView::slot_create_popup_menu() {
  popup_menu_ = new QMenu();

  action_open_file_ = new QAction(this);
  action_open_file_->setText(tr("Open"));
  connect(action_open_file_, &QAction::triggered, this, [this](bool) {
    for (const auto& path : GetSelectedPaths()) emit SignalOpenFile(path);
  });

  action_rename_file_ = new QAction(this);
  action_rename_file_->setText(tr("Rename"));
  connect(action_rename_file_, &QAction::triggered, this,
          &FileTreeView::SlotRenameSelectedItem);

  action_delete_file_ = new QAction(this);
  action_delete_file_->setText(tr("Delete"));
  connect(action_delete_file_, &QAction::triggered, this,
          &FileTreeView::SlotDeleteSelectedItem);

  action_calculate_hash_ = new QAction(this);
  action_calculate_hash_->setText(tr("Calculate Hash"));
  connect(action_calculate_hash_, &QAction::triggered, this,
          &FileTreeView::slot_calculate_hash);

  action_make_directory_ = new QAction(this);
  action_make_directory_->setText(tr("Directory"));
  connect(action_make_directory_, &QAction::triggered, this,
          &FileTreeView::SlotMkdirBelowAtSelectedItem);

  action_create_empty_file_ = new QAction(this);
  action_create_empty_file_->setText(tr("File"));
  connect(action_create_empty_file_, &QAction::triggered, this,
          &FileTreeView::SlotTouchBelowAtSelectedItem);

  action_compress_files_ = new QAction(this);
  action_compress_files_->setText(tr("Compress..."));
  action_compress_files_->setVisible(false);
  connect(action_compress_files_, &QAction::triggered, this,
          &FileTreeView::slot_compress_files);

  auto* action_open_with_system_default_application = new QAction(this);
  action_open_with_system_default_application->setText(
      tr("Open with Default System Application"));
  connect(action_open_with_system_default_application, &QAction::triggered,
          this, &FileTreeView::SlotOpenSelectedItemBySystemApplication);

  new_item_action_menu_ = new QMenu(this);
  new_item_action_menu_->setTitle(tr("New"));
  new_item_action_menu_->addAction(action_create_empty_file_);
  new_item_action_menu_->addAction(action_make_directory_);

  popup_menu_->addAction(action_open_file_);
  popup_menu_->addAction(action_open_with_system_default_application);

  popup_menu_->addSeparator();
  popup_menu_->addMenu(new_item_action_menu_);
  popup_menu_->addSeparator();

  popup_menu_->addAction(action_rename_file_);
  popup_menu_->addAction(action_delete_file_);
  popup_menu_->addAction(action_compress_files_);
  popup_menu_->addAction(action_calculate_hash_);
}

void FileTreeView::slot_show_custom_context_menu(const QPoint& point) {
  auto target_path = this->GetPathByClickPoint(point);
  auto select_paths = GetSelectedPaths();

  if (select_paths.size() != 1) return;

  auto select_path = select_paths.front();
  if (target_path.isEmpty() && !select_path.isEmpty()) {
    target_path = select_path;
  }

  QFileInfo file_info(target_path);

  action_open_file_->setEnabled(false);
  action_rename_file_->setEnabled(false);
  action_delete_file_->setEnabled(false);
  action_calculate_hash_->setEnabled(false);
  action_make_directory_->setEnabled(false);
  action_create_empty_file_->setEnabled(false);
  action_calculate_hash_->setEnabled(false);

  if (file_info.exists()) {
    action_open_file_->setEnabled(file_info.isFile() && file_info.isReadable());
    action_rename_file_->setEnabled(true);
    action_delete_file_->setEnabled(true);

    action_make_directory_->setEnabled(file_info.isDir() &&
                                       file_info.isWritable());
    action_create_empty_file_->setEnabled(file_info.isDir() &&
                                          file_info.isWritable());
    action_calculate_hash_->setEnabled(file_info.isFile() &&
                                       file_info.isReadable());
  } else {
    action_create_empty_file_->setEnabled(true);
    action_make_directory_->setEnabled(true);
  }

  popup_menu_->exec(this->GetMousePointGlobal(point));
}

void FileTreeView::slot_calculate_hash() {
  if (GetSelectedPaths().empty()) return;
  auto selected_path = GetSelectedPaths().front();

  GpgOperaHelper::WaitForOpera(
      this->parentWidget(), tr("Calculating"), [=](const OperaWaitingHd& hd) {
        RunOperaAsync(
            [=](const DataObjectPtr& data_object) {
              data_object->Swap({CalculateHash(selected_path)});
              return 0;
            },
            [hd](int rtn, const DataObjectPtr& data_object) {
              hd();
              if (rtn < 0 || !data_object->Check<QString>()) {
                return;
              }
              auto result = ExtractParams<QString>(data_object, 0);
              emit UISignalStation::GetInstance() -> SignalRefreshInfoBoard(
                  result, InfoBoardStatus::INFO_ERROR_OK);
            },
            "calculate_file_hash");
      });
}

void FileTreeView::slot_compress_files() {}

void FileTreeView::paintEvent(QPaintEvent* event) {
  QTreeView::paintEvent(event);

  // if (!initial_resize_done_) {
  //   slot_adjust_column_widths();
  //   initial_resize_done_ = true;
  // }
}

void FileTreeView::mousePressEvent(QMouseEvent* event) {
  QTreeView::mousePressEvent(event);
}

void FileTreeView::slot_adjust_column_widths() {
  for (int i = 1; i < dir_model_->columnCount(); ++i) {
    this->resizeColumnToContents(i);
  }
}

void FileTreeView::SlotSwitchBatchMode(bool batch) {
  this->setSelectionMode(batch ? QAbstractItemView::MultiSelection
                               : QAbstractItemView::SingleSelection);
  selectionModel()->clearSelection();
}

void FileTreeView::SetPath(const QString& target_path) {
  LOG_D() << "try to open target path:" << target_path;

  QFileInfo info(target_path);
  QString effective_path;

  if (info.exists()) {
    effective_path =
        info.isFile() ? info.absolutePath() : info.absoluteFilePath();
  } else {
    effective_path = QDir::currentPath();
  }

  LOG_D() << "effective path:" << effective_path;

  dir_model_->setRootPath(effective_path);
  current_path_ = dir_model_->rootPath();
  QModelIndex root_index = dir_model_->index(current_path_);

  if (root_index.isValid()) {
    QPointer<FileTreeView> self(this);
    this->setRootIndex(root_index);
    QTimer::singleShot(200, [=]() {
      if (self != nullptr && info.isFile()) {
        self->setCurrentIndex(dir_model_->index(info.absoluteFilePath()));
        self->expand(currentIndex().parent());
        self->scrollTo(currentIndex(), QAbstractItemView::PositionAtCenter);
        self->selectionModel()->select(
            currentIndex(),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      }
    });
  } else {
    LOG_W() << "invalid path, fallback to current dir.";
    current_path_ = QDir::currentPath();
    this->setRootIndex(dir_model_->index(current_path_));
  }
}
}  // namespace GpgFrontend::UI
