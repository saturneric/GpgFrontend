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
#include "ui/dialog/CreateFileSystemItemDialog.h"
#include "ui/function/GpgOperaHelper.h"

namespace GpgFrontend::UI {

namespace {

auto SelectedItemsText(const QStringList& paths) -> QString {
  if (paths.size() == 1) {
    return QFileInfo(paths.front()).fileName();
  }

  return QObject::tr("%1 item(s)").arg(paths.size());
}

void NotifyStatus(const QString& text, int timeout = 3000) {
  emit UISignalStation::GetInstance() -> SignalRefreshStatusBar(text, timeout);
}

auto MovePathToTrash(const QString& path) -> bool {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  return QFile::moveToTrash(path);
#else
  QFileInfo info(path);
  if (info.isDir()) {
    QDir dir(path);
    return dir.removeRecursively();
  }

  return QFile::remove(path);
#endif
}
}  // namespace

FileTreeView::FileTreeView(QWidget* parent)
    : QTreeView(parent), dir_model_(new QFileSystemModel(this)) {
  dir_model_->setReadOnly(false);
  dir_model_->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
  dir_model_->setRootPath(QDir::homePath());
  current_path_ = dir_model_->rootPath();

  this->setModel(dir_model_);

  InitViewStyle();

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

void FileTreeView::InitViewStyle() {
  setProperty("gfFileTreeView", true);

  setRootIsDecorated(true);
  setItemsExpandable(true);
  setExpandsOnDoubleClick(false);

  setAlternatingRowColors(true);
  setUniformRowHeights(true);
  setAnimated(false);
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setContextMenuPolicy(Qt::CustomContextMenu);

  setDragEnabled(true);
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
  setDropIndicatorShown(true);

  setDragDropMode(QAbstractItemView::DragDrop);
  setDefaultDropAction(Qt::MoveAction);
  setDragDropOverwriteMode(false);

  setAllColumnsShowFocus(true);
  setMouseTracking(true);
  setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  header()->setStretchLastSection(false);
  header()->setHighlightSections(false);
  header()->setSectionsClickable(true);
  header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  setColumnWidth(0, 360);
  setColumnWidth(1, 90);
  setColumnWidth(2, 120);
  setColumnWidth(3, 160);

  header()->setSectionResizeMode(0, QHeaderView::Interactive);
  header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header()->setSectionResizeMode(3, QHeaderView::Stretch);

  setStyleSheet(R"(
QTreeView[gfFileTreeView="true"] {
  outline: 0;
}

QTreeView[gfFileTreeView="true"]::item {
  min-height: 24px;
  padding: 2px 4px;
  border: none;
}

QTreeView[gfFileTreeView="true"]::item:selected {
  background: palette(highlight);
  color: palette(highlighted-text);
}
)");

  style()->unpolish(this);
  style()->polish(this);
}

void FileTreeView::selectionChanged(const QItemSelection& selected,
                                    const QItemSelection& deselected) {
  QTreeView::selectionChanged(selected, deselected);
  sync_selected_paths_from_selection();
}

void FileTreeView::SlotGoPath(const QString& target_path) {
  if (target_path.isEmpty()) {
    current_path_.clear();
    dir_model_->setRootPath("");
    setRootIndex(dir_model_->index(""));
    slot_adjust_column_widths();
    emit SignalPathChanged(current_path_);
    return;
  }

  const QFileInfo file_info(target_path);

  if (!file_info.exists() || !file_info.isDir() || !file_info.isReadable() ||
      !file_info.isExecutable()) {
    LOG_W() << "invalid or inaccessible path:" << target_path;
    return;
  }

  current_path_ = file_info.absoluteFilePath();
  dir_model_->setRootPath(current_path_);
  setRootIndex(dir_model_->index(current_path_));
  clearSelection();

  slot_adjust_column_widths();
  emit SignalPathChanged(current_path_);
}
void FileTreeView::slot_file_tree_view_item_double_clicked(
    const QModelIndex& index) {
  if (!index.isValid()) return;

  const QFileInfo file_info(dir_model_->fileInfo(index).absoluteFilePath());

  if (file_info.isFile()) {
    if (file_info.isReadable()) {
      emit SignalOpenFile(file_info.absoluteFilePath());
    } else {
      QMessageBox::warning(this, tr("Unable to Open File"),
                           tr("The file cannot be read."));
    }
    return;
  }

  if (file_info.isDir()) {
    if (file_info.isReadable() && file_info.isExecutable()) {
      SlotGoPath(file_info.absoluteFilePath());
    } else {
      QMessageBox::warning(
          this, tr("Unable to Open Folder"),
          tr("The folder cannot be opened. Please check permissions."));
    }
  }
}

void FileTreeView::SlotUpLevel() {
  if (current_path_.isEmpty()) {
    LOG_D() << "current path is empty, ignoring ...";
    return;
  }

  QFileInfo info(current_path_);
  QString target_path = info.absoluteFilePath();
  QDir parent_dir(target_path);

  target_path.clear();
  if (parent_dir.cdUp()) {
    target_path = parent_dir.absolutePath();
  }

  this->SlotGoPath(target_path);
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
  const auto paths = GetSelectedPaths();
  if (paths.isEmpty()) return;

  const auto display_text = SelectedItemsText(paths);

  const auto ret = QMessageBox::warning(
      this, tr("Move to Trash"),
      paths.size() == 1
          ? tr("Move \"%1\" to Trash?").arg(display_text)
          : tr("Move %1 selected items to Trash?").arg(paths.size()),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  QStringList failed_paths;
  QStringList trashed_paths;

  for (const auto& path : paths) {
    if (path.isEmpty() || !QFileInfo::exists(path)) continue;

    if (MovePathToTrash(path)) {
      trashed_paths.append(path);
    } else {
      failed_paths.append(path);
    }
  }

  SlotGoPath(current_path_);

  if (!failed_paths.isEmpty()) {
    QMessageBox::warning(
        this, tr("Unable to Move to Trash"),
        failed_paths.size() == 1
            ? tr("The item \"%1\" could not be moved to Trash.")
                  .arg(QFileInfo(failed_paths.front()).fileName())
            : tr("%1 item(s) could not be moved to Trash.")
                  .arg(failed_paths.size()));
  }

  if (!trashed_paths.isEmpty()) {
    NotifyStatus(tr("Moved %1 item(s) to Trash.").arg(trashed_paths.size()));
  }
}

void FileTreeView::SlotMkdir() {
  clearSelection();
  setCurrentIndex(rootIndex());
  SlotMkdirBelowAtSelectedItem();
}

void FileTreeView::SlotMkdirBelowAtSelectedItem() {
  const auto target_dir = current_target_directory_path();
  if (target_dir.isEmpty()) return;

  CreateFileSystemItemDialog dialog(
      CreateFileSystemItemDialog::ItemType::kFOLDER, target_dir, this);

  if (dialog.exec() != QDialog::Accepted) return;

  const auto new_path = dialog.GetPath();
  const auto new_name = dialog.GetName();

  const auto parent_index = dir_model_->index(target_dir);
  if (!parent_index.isValid()) {
    QMessageBox::warning(this, tr("Unable to Create Folder"),
                         tr("The target folder is not available."));
    return;
  }

  const auto new_index = dir_model_->mkdir(parent_index, new_name);
  if (!new_index.isValid()) {
    QMessageBox::warning(
        this, tr("Unable to Create Folder"),
        tr("The folder could not be created. Please check permissions."));
    return;
  }

  setCurrentIndex(new_index);
  selectionModel()->select(new_index, QItemSelectionModel::ClearAndSelect |
                                          QItemSelectionModel::Rows);
  scrollTo(new_index, QAbstractItemView::PositionAtCenter);
  NotifyStatus(tr("Created folder: %1").arg(new_name));
}

void FileTreeView::SlotTouch() {
  clearSelection();
  setCurrentIndex(rootIndex());
  SlotTouchBelowAtSelectedItem();
}

void FileTreeView::SlotTouchBelowAtSelectedItem() {
  const auto target_dir = current_target_directory_path();
  if (target_dir.isEmpty()) return;

  CreateFileSystemItemDialog dialog(CreateFileSystemItemDialog::ItemType::kFILE,
                                    target_dir, this);

  if (dialog.exec() != QDialog::Accepted) return;

  QFile new_file(dialog.GetPath());
  if (!new_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
    QMessageBox::warning(
        this, tr("Unable to Create File"),
        tr("The file could not be created. Please check permissions."));
    return;
  }

  new_file.close();

  const auto index = dir_model_->index(dialog.GetPath());
  if (index.isValid()) {
    setCurrentIndex(index);
    selectionModel()->select(
        index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(index, QAbstractItemView::PositionAtCenter);
  }
  NotifyStatus(
      tr("Created file: %1").arg(QFileInfo(dialog.GetPath()).fileName()));
}

void FileTreeView::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Backspace) {
    SlotUpLevel();
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_F2) {
    SlotRenameSelectedItem();
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Delete) {
    SlotDeleteSelectedItem();
    event->accept();
    return;
  }

  if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
      currentIndex().isValid()) {
    slot_file_tree_view_item_double_clicked(currentIndex());
    event->accept();
    return;
  }

  QTreeView::keyPressEvent(event);
}

void FileTreeView::SlotOpenSelectedItemBySystemApplication() {
  if (selected_paths_.size() != 1) return;

  const auto selected_path = selected_paths_.front();
  if (!QFileInfo::exists(selected_path)) return;

  QDesktopServices::openUrl(QUrl::fromLocalFile(selected_path));
}

void FileTreeView::SlotRenameSelectedItem() {
  if (selected_paths_.size() != 1) return;

  const auto selected_path = selected_paths_.front();
  const QFileInfo file_info(selected_path);
  const auto old_name = file_info.fileName();

  bool ok = false;
  const auto text =
      QInputDialog::getText(this, tr("Rename"), tr("New name:"),
                            QLineEdit::Normal, file_info.fileName(), &ok);

  if (!ok) return;

  const auto new_name = text.trimmed();
  if (new_name.isEmpty() || new_name == file_info.fileName()) return;

  if (new_name.contains("/") || new_name.contains("\\")) {
    QMessageBox::warning(this, tr("Invalid Name"),
                         tr("The name must not contain path separators."));
    return;
  }

  const auto new_name_path =
      QDir(file_info.absolutePath()).absoluteFilePath(new_name);

  if (QFileInfo::exists(new_name_path)) {
    QMessageBox::warning(this, tr("Name Already Exists"),
                         tr("A file or folder with this name already exists."));
    return;
  }

  if (!QDir().rename(file_info.absoluteFilePath(), new_name_path)) {
    QMessageBox::warning(this, tr("Unable to Rename"),
                         tr("The file or folder could not be renamed."));
    return;
  }

  refresh_current_path_and_select_paths({new_name_path});

  NotifyStatus(tr(R"(Renamed "%1" to "%2".)").arg(old_name, new_name));
}

auto FileTreeView::GetMousePointGlobal(const QPoint& point) -> QPoint {
  return this->viewport()->mapToGlobal(point);
}

void FileTreeView::slot_create_popup_menu() {
  popup_menu_ = new QMenu(this);

  action_open_file_ = new QAction(
      QIcon::fromTheme(QStringLiteral("document-open")), tr("Open"), this);
  connect(action_open_file_, &QAction::triggered, this, [this](bool) {
    for (const auto& path : GetSelectedPaths()) emit SignalOpenFile(path);
  });

  action_open_with_system_default_application_ =
      new QAction(QIcon::fromTheme(QStringLiteral("system-run")),
                  tr("Open with Default Application"), this);
  connect(action_open_with_system_default_application_, &QAction::triggered,
          this, &FileTreeView::SlotOpenSelectedItemBySystemApplication);

  action_rename_file_ = new QAction(
      QIcon::fromTheme(QStringLiteral("edit-rename")), tr("Rename"), this);
  connect(action_rename_file_, &QAction::triggered, this,
          &FileTreeView::SlotRenameSelectedItem);

  action_delete_file_ =
      new QAction(QIcon::fromTheme(QStringLiteral("user-trash")),
                  tr("Move to Trash"), this);
  connect(action_delete_file_, &QAction::triggered, this,
          &FileTreeView::SlotDeleteSelectedItem);

  action_calculate_hash_ =
      new QAction(QIcon::fromTheme(QStringLiteral("document-properties")),
                  tr("Calculate Hash"), this);
  connect(action_calculate_hash_, &QAction::triggered, this,
          &FileTreeView::slot_calculate_hash);

  action_make_directory_ = new QAction(
      QIcon::fromTheme(QStringLiteral("folder-new")), tr("Folder"), this);
  connect(action_make_directory_, &QAction::triggered, this,
          &FileTreeView::SlotMkdirBelowAtSelectedItem);

  action_create_empty_file_ = new QAction(
      QIcon::fromTheme(QStringLiteral("document-new")), tr("Empty File"), this);
  connect(action_create_empty_file_, &QAction::triggered, this,
          &FileTreeView::SlotTouchBelowAtSelectedItem);

  action_compress_files_ =
      new QAction(QIcon::fromTheme(QStringLiteral("archive-insert")),
                  tr("Compress..."), this);
  action_compress_files_->setVisible(false);

  action_copy_path_ = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")),
                                  tr("Copy Path"), this);
  connect(action_copy_path_, &QAction::triggered, this,
          &FileTreeView::SlotCopyPath);

  action_refresh_ = new QAction(
      QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Refresh"), this);
  connect(action_refresh_, &QAction::triggered, this,
          &FileTreeView::SlotRefresh);

  new_item_action_menu_ = new QMenu(tr("New"), this);
  new_item_action_menu_->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
  new_item_action_menu_->addAction(action_create_empty_file_);
  new_item_action_menu_->addAction(action_make_directory_);

  popup_menu_->addAction(action_open_file_);
  popup_menu_->addAction(action_open_with_system_default_application_);
  popup_menu_->addSeparator();

  popup_menu_->addMenu(new_item_action_menu_);
  popup_menu_->addSeparator();

  popup_menu_->addAction(action_rename_file_);
  popup_menu_->addAction(action_delete_file_);
  popup_menu_->addAction(action_copy_path_);
  popup_menu_->addSeparator();

  popup_menu_->addAction(action_refresh_);
  popup_menu_->addAction(action_compress_files_);
  popup_menu_->addAction(action_calculate_hash_);
}

void FileTreeView::slot_show_custom_context_menu(const QPoint& point) {
  const auto index = indexAt(point);

  if (index.isValid()) {
    if (!selectionModel()->isSelected(index)) {
      selectionModel()->select(index, QItemSelectionModel::ClearAndSelect |
                                          QItemSelectionModel::Rows);
      setCurrentIndex(index);
    }
  } else {
    clearSelection();
  }

  const auto select_paths = GetSelectedPaths();

  QFileInfo file_info;
  bool has_target = false;

  if (index.isValid()) {
    file_info = dir_model_->fileInfo(index);
    has_target = file_info.exists();
  } else if (select_paths.size() == 1) {
    file_info = QFileInfo(select_paths.front());
    has_target = file_info.exists();
  }

  action_open_file_->setEnabled(false);
  action_open_with_system_default_application_->setEnabled(false);
  action_rename_file_->setEnabled(false);
  action_delete_file_->setEnabled(false);
  action_calculate_hash_->setEnabled(false);
  action_make_directory_->setEnabled(false);
  action_create_empty_file_->setEnabled(false);
  action_copy_path_->setEnabled(false);
  action_refresh_->setEnabled(true);

  const QFileInfo current_dir_info(current_path_);
  const bool current_dir_writable = current_dir_info.exists() &&
                                    current_dir_info.isDir() &&
                                    current_dir_info.isWritable();

  if (!has_target) {
    action_make_directory_->setEnabled(current_dir_writable);
    action_create_empty_file_->setEnabled(current_dir_writable);
    action_refresh_->setEnabled(true);
    popup_menu_->exec(GetMousePointGlobal(point));
    return;
  }

  const bool single_selection = select_paths.size() <= 1;

  action_copy_path_->setEnabled(!select_paths.isEmpty());
  action_open_file_->setEnabled(file_info.isFile() && file_info.isReadable());
  action_open_with_system_default_application_->setEnabled(file_info.exists() &&
                                                           single_selection);
  action_rename_file_->setEnabled(
      single_selection && file_info.exists() &&
      QFileInfo(file_info.absolutePath()).isWritable());
  action_delete_file_->setEnabled(
      !select_paths.isEmpty() &&
      std::all_of(
          select_paths.cbegin(), select_paths.cend(), [](const QString& path) {
            const QFileInfo info(path);
            return info.exists() && QFileInfo(info.absolutePath()).isWritable();
          }));

  const bool target_dir_writable =
      file_info.isDir() && file_info.exists() && file_info.isWritable();

  action_make_directory_->setEnabled(target_dir_writable);
  action_create_empty_file_->setEnabled(target_dir_writable);

  action_calculate_hash_->setEnabled(file_info.isFile() &&
                                     file_info.isReadable());

  popup_menu_->exec(GetMousePointGlobal(point));
}

void FileTreeView::slot_calculate_hash() {
  if (GetSelectedPaths().empty()) return;
  auto selected_path = GetSelectedPaths().front();

  GpgOperaHelper::WaitForOpera(
      this->parentWidget(), tr("Calculating"), [=](const OperaWaitingHd& hd) {
        RunOperaAsync(
            [=](const DataObjectPtr& data_object) {
              // Hash the file once and carry the structured fields across the
              // async boundary; the card and the report text are both derived
              // from them, so the Info Board never parses formatted text.
              data_object->Swap({CalculateFileHashInfo(selected_path)});
              return 0;
            },
            [hd](int rtn, const DataObjectPtr& data_object) {
              hd();
              using HashFields = QContainer<QPair<QString, QString>>;
              if (rtn < 0 || !data_object->Check<HashFields>()) {
                return;
              }
              auto fields = ExtractParams<HashFields>(data_object, 0);
              if (fields.isEmpty()) return;

              InfoBoardCard card;
              card.title = tr("File Hash Information");
              card.status = InfoBoardStatus::kINFO_ERROR_OK;
              card.fields = fields;

              QContainer<InfoBoardCard> cards;
              cards.append(card);

              emit UISignalStation::GetInstance()
                  -> SignalRefreshInfoBoardCards(
                      FormatFileHashInfo(fields),
                      InfoBoardStatus::kINFO_ERROR_OK, cards, card.title, {},
                      {}, {});
            },
            "calculate_file_hash");
      });
}

void FileTreeView::mousePressEvent(QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto pos = event->position().toPoint();
#else
  auto pos = event->pos();
#endif

  if (!indexAt(pos).isValid() && event->button() == Qt::LeftButton) {
    clearSelection();
    setCurrentIndex(QModelIndex());
  }

  QTreeView::mousePressEvent(event);
}

void FileTreeView::slot_adjust_column_widths() {
  if (dir_model_ == nullptr) return;

  for (int i = 1; i < dir_model_->columnCount(); ++i) {
    resizeColumnToContents(i);
  }

  if (columnWidth(0) < 280) {
    setColumnWidth(0, 320);
  }
}

void FileTreeView::SlotSwitchBatchMode(bool batch) {
  setSelectionMode(batch ? QAbstractItemView::ExtendedSelection
                         : QAbstractItemView::SingleSelection);
  selectionModel()->clearSelection();

  setToolTip(batch ? tr("Batch mode is enabled. Use Ctrl or Shift to select "
                        "multiple items.")
                   : QString());
}

void FileTreeView::SetPath(const QString& target_path) {
  LOG_D() << "try to open target path:" << target_path;

  const QFileInfo input_info(target_path);

  QString effective_path;
  QString file_to_select;

  if (input_info.exists()) {
    if (input_info.isFile()) {
      effective_path = input_info.absolutePath();
      file_to_select = input_info.absoluteFilePath();
    } else {
      effective_path = input_info.absoluteFilePath();
    }
  } else {
    effective_path = QDir::currentPath();
  }

  LOG_D() << "effective path:" << effective_path;

  const QFileInfo effective_info(effective_path);
  if (!effective_info.exists() || !effective_info.isDir()) {
    LOG_W() << "invalid path, fallback to current dir.";
    effective_path = QDir::currentPath();
  }

  dir_model_->setRootPath(effective_path);
  current_path_ = dir_model_->rootPath();

  const auto root_index = dir_model_->index(current_path_);
  if (!root_index.isValid()) {
    LOG_W() << "invalid root index:" << current_path_;
    return;
  }

  setRootIndex(root_index);
  emit SignalPathChanged(current_path_);

  if (!file_to_select.isEmpty()) {
    QPointer<FileTreeView> self(this);
    QTimer::singleShot(200, this, [self, file_to_select]() {
      if (self == nullptr) return;

      const auto index = self->dir_model_->index(file_to_select);
      if (!index.isValid()) return;

      self->setCurrentIndex(index);
      self->scrollTo(index, QAbstractItemView::PositionAtCenter);
      self->selectionModel()->select(
          index,
          QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    });
  }
}

auto FileTreeView::current_target_directory_path() const -> QString {
  auto index = currentIndex();

  if (!index.isValid()) {
    return current_path_;
  }

  const auto info = dir_model_->fileInfo(index);
  if (info.isDir()) {
    return info.absoluteFilePath();
  }

  return info.absolutePath();
}

void FileTreeView::dragEnterEvent(QDragEnterEvent* event) {
  if (!event->mimeData()->hasUrls()) {
    event->ignore();
    return;
  }

  if (event->source() == this) {
    event->setDropAction(Qt::MoveAction);
  } else {
    event->setDropAction(Qt::CopyAction);
  }

  event->accept();
}

void FileTreeView::dragMoveEvent(QDragMoveEvent* event) {
  if (!event->mimeData()->hasUrls()) {
    event->ignore();
    return;
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto pos = event->position().toPoint();
#else
  auto pos = event->pos();
#endif

  const auto target_dir = get_drop_target_directory(pos);

  if (target_dir.isEmpty() || !QFileInfo(target_dir).isWritable()) {
    event->ignore();
    return;
  }

  event->setDropAction(event->source() == this ? Qt::MoveAction
                                               : Qt::CopyAction);
  event->accept();
}

auto FileTreeView::get_drop_target_directory(const QPoint& pos) const
    -> QString {
  const auto index = indexAt(pos);

  if (!index.isValid()) {
    return current_path_;
  }

  const auto info = dir_model_->fileInfo(index);

  if (info.isDir()) {
    return info.absoluteFilePath();
  }

  if (info.isFile()) {
    return info.absolutePath();
  }

  return current_path_;
}

auto FileTreeView::is_move_into_itself_or_child(const QString& source_path,
                                                const QString& target_dir) const
    -> bool {
  auto source = QDir::cleanPath(QFileInfo(source_path).absoluteFilePath());
  auto target = QDir::cleanPath(QFileInfo(target_dir).absoluteFilePath());

#ifdef Q_OS_WIN
  source.replace("\\", "/");
  target.replace("\\", "/");
#endif

  if (source == target) {
    return true;
  }

  const QFileInfo source_info(source);
  if (!source_info.isDir()) {
    return false;
  }

  return target.startsWith(source + "/");
}

auto FileTreeView::move_path_to_directory(const QString& source_path,
                                          const QString& target_dir) -> bool {
  const QFileInfo source_info(source_path);
  const QFileInfo target_info(target_dir);

  if (!source_info.exists() || !target_info.exists() || !target_info.isDir()) {
    return false;
  }

  if (!target_info.isWritable()) {
    return false;
  }

  if (is_move_into_itself_or_child(source_path, target_dir)) {
    return false;
  }

  const auto source_abs_path = source_info.absoluteFilePath();
  const auto target_path =
      QDir(target_dir).absoluteFilePath(source_info.fileName());

  if (QDir::cleanPath(source_abs_path) == QDir::cleanPath(target_path)) {
    return true;
  }

  if (QFileInfo::exists(target_path)) {
    QMessageBox::warning(
        this, tr("Move Failed"),
        tr("A file or folder named \"%1\" already exists in the target folder.")
            .arg(source_info.fileName()));
    return false;
  }

  if (!QDir().rename(source_abs_path, target_path)) {
    QMessageBox::warning(
        this, tr("Move Failed"),
        tr("Unable to move \"%1\".\n\nThe target may be on another volume, or "
           "you may not have sufficient permissions.")
            .arg(source_info.fileName()));
    return false;
  }

  return true;
}

auto FileTreeView::is_same_directory_operation(const QStringList& source_paths,
                                               const QString& target_dir) const
    -> bool {
  auto clean_target_dir =
      QDir::cleanPath(QFileInfo(target_dir).absoluteFilePath());

#ifdef Q_OS_WIN
  clean_target_dir.replace("\\", "/");
#endif

  for (const auto& source_path : source_paths) {
    const QFileInfo source_info(source_path);
    auto source_dir = QDir::cleanPath(source_info.absoluteDir().absolutePath());

#ifdef Q_OS_WIN
    source_dir.replace("\\", "/");
#endif

    if (source_dir != clean_target_dir) {
      return false;
    }
  }

  return true;
}

void FileTreeView::dropEvent(QDropEvent* event) {
  if (!event->mimeData()->hasUrls()) {
    event->ignore();
    return;
  }

  const bool internal_drag = event->source() == this;
  const auto operation = internal_drag ? Qt::MoveAction : Qt::CopyAction;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto pos = event->position().toPoint();
#else
  auto pos = event->pos();
#endif

  const auto target_dir = get_drop_target_directory(pos);
  if (target_dir.isEmpty()) {
    event->ignore();
    return;
  }

  const QFileInfo target_info(target_dir);
  if (!target_info.exists() || !target_info.isDir() ||
      !target_info.isWritable()) {
    QMessageBox::warning(this,
                         internal_drag ? tr("Move Failed") : tr("Copy Failed"),
                         tr("The target folder is not writable."));
    event->ignore();
    return;
  }

  QStringList source_paths;
  for (const auto& url : event->mimeData()->urls()) {
    if (!url.isLocalFile()) continue;

    const auto path = url.toLocalFile();
    if (path.isEmpty()) continue;

    source_paths.append(path);
  }

  source_paths.removeDuplicates();

  if (source_paths.isEmpty()) {
    event->ignore();
    return;
  }

  if (internal_drag && is_same_directory_operation(source_paths, target_dir)) {
    NotifyStatus(tr("The source and target folder are the same."), 2000);
    event->ignore();
    return;
  }

  if (!internal_drag && is_same_directory_operation(source_paths, target_dir)) {
    NotifyStatus(tr("The source and target folder are the same."), 2000);
    event->ignore();
    return;
  }

  for (const auto& source_path : source_paths) {
    const QFileInfo source_info(source_path);

    if (!source_info.exists()) {
      continue;
    }

    if (internal_drag &&
        is_move_into_itself_or_child(source_path, target_dir)) {
      QMessageBox::warning(
          this, tr("Move Failed"),
          tr("Cannot move \"%1\" into itself or one of its subfolders.")
              .arg(source_info.fileName()));
      event->ignore();
      return;
    }

    if (!internal_drag && source_info.isDir() &&
        is_move_into_itself_or_child(source_path, target_dir)) {
      QMessageBox::warning(
          this, tr("Copy Failed"),
          tr("Cannot copy \"%1\" into itself or one of its subfolders.")
              .arg(source_info.fileName()));
      event->ignore();
      return;
    }
  }

  QMessageBox box(this);
  box.setIcon(QMessageBox::Question);

  if (internal_drag) {
    box.setWindowTitle(tr("Move Items"));
    box.setText(
        source_paths.size() == 1
            ? tr(R"(Move "%1" to "%2"?)")
                  .arg(QFileInfo(source_paths.front()).fileName(), target_dir)
            : tr("Move %1 items to \"%2\"?")
                  .arg(source_paths.size())
                  .arg(target_dir));
  } else {
    box.setWindowTitle(tr("Copy Items"));
    box.setText(
        source_paths.size() == 1
            ? tr(R"(Copy "%1" to "%2"?)")
                  .arg(QFileInfo(source_paths.front()).fileName(), target_dir)
            : tr("Copy %1 items to \"%2\"?")
                  .arg(source_paths.size())
                  .arg(target_dir));
  }

  auto* confirm_button = box.addButton(internal_drag ? tr("Move") : tr("Copy"),
                                       QMessageBox::AcceptRole);
  box.addButton(QMessageBox::Cancel);
  box.exec();

  if (box.clickedButton() != confirm_button) {
    event->ignore();
    return;
  }

  QStringList result_paths;
  bool all_ok = true;
  bool detailed_error_shown = false;

  for (const auto& source_path : source_paths) {
    const QFileInfo source_info(source_path);
    const auto new_path =
        QDir(target_dir).absoluteFilePath(source_info.fileName());

    bool ok = false;
    if (internal_drag) {
      ok = move_path_to_directory(source_path, target_dir);
    } else {
      ok = copy_path_to_directory(source_path, target_dir);
    }

    if (!ok) {
      all_ok = false;
      detailed_error_shown = true;
      continue;
    }

    result_paths.append(new_path);
  }

  refresh_current_path_and_select_paths(result_paths);

  event->setDropAction(operation);
  event->accept();

  if (!result_paths.isEmpty()) {
    NotifyStatus(internal_drag
                     ? tr("Moved %1 item(s).").arg(result_paths.size())
                     : tr("Copied %1 item(s).").arg(result_paths.size()));

    if (internal_drag &&
        is_same_directory_operation(source_paths, target_dir)) {
      event->ignore();
      return;
    }
  }

  if (!all_ok && !detailed_error_shown) {
    QMessageBox::warning(
        this,
        internal_drag ? tr("Move Partially Completed")
                      : tr("Copy Partially Completed"),
        internal_drag
            ? tr("Some items could not be moved. Please check permissions or "
                 "whether the target is on another volume.")
            : tr("Some items could not be copied. Please check permissions."));
  }
}

auto FileTreeView::copy_path_to_directory(const QString& source_path,
                                          const QString& target_dir) -> bool {
  const QFileInfo source_info(source_path);
  const QFileInfo target_info(target_dir);

  if (!source_info.exists() || !target_info.exists() || !target_info.isDir()) {
    return false;
  }

  if (!target_info.isWritable()) {
    return false;
  }

  const auto target_path =
      QDir(target_dir).absoluteFilePath(source_info.fileName());

  if (QFileInfo::exists(target_path)) {
    QMessageBox::warning(
        this, tr("Copy Failed"),
        tr("A file or folder named \"%1\" already exists in the target folder.")
            .arg(source_info.fileName()));
    return false;
  }

  if (source_info.isDir()) {
    return copy_directory_recursive(source_info.absoluteFilePath(),
                                    target_path);
  }

  if (source_info.isFile()) {
    if (!QFile::copy(source_info.absoluteFilePath(), target_path)) {
      QMessageBox::warning(
          this, tr("Copy Failed"),
          tr("Unable to copy \"%1\". Please check permissions.")
              .arg(source_info.fileName()));
      return false;
    }

    return true;
  }

  QMessageBox::warning(this, tr("Copy Failed"),
                       tr("\"%1\" is not a regular file or folder.")
                           .arg(source_info.fileName()));
  return false;
}

auto FileTreeView::copy_directory_recursive(const QString& source_dir,
                                            const QString& target_dir) -> bool {
  const QDir source(source_dir);

  if (!source.exists()) {
    return false;
  }

  if (QFileInfo::exists(target_dir)) {
    QMessageBox::warning(this, tr("Copy Failed"),
                         tr("The target folder \"%1\" already exists.")
                             .arg(QFileInfo(target_dir).fileName()));
    return false;
  }

  QDir target_parent(QFileInfo(target_dir).absolutePath());
  if (!target_parent.mkpath(QFileInfo(target_dir).fileName())) {
    QMessageBox::warning(this, tr("Copy Failed"),
                         tr("Unable to create target folder \"%1\".")
                             .arg(QFileInfo(target_dir).fileName()));
    return false;
  }

  const auto entries = source.entryInfoList(
      QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System,
      QDir::DirsFirst | QDir::Name);

  for (const auto& entry : entries) {
    const auto source_path = entry.absoluteFilePath();
    const auto target_path =
        QDir(target_dir).absoluteFilePath(entry.fileName());

    if (entry.isDir()) {
      if (!copy_directory_recursive(source_path, target_path)) {
        return false;
      }
      continue;
    }

    if (entry.isFile()) {
      if (!QFile::copy(source_path, target_path)) {
        QMessageBox::warning(
            this, tr("Copy Failed"),
            tr("Unable to copy \"%1\".").arg(entry.fileName()));
        return false;
      }
      continue;
    }

    // symbolic links, special files, sockets, etc.
    LOG_W() << "skip unsupported file system entry:" << source_path;
  }

  return true;
}

void FileTreeView::SlotCopyPath() {
  const auto paths = GetSelectedPaths();
  if (paths.isEmpty()) return;

  QGuiApplication::clipboard()->setText(paths.join(QChar::LineFeed));

  emit UISignalStation::GetInstance() -> SignalRefreshStatusBar(
      paths.size() == 1 ? tr("Path copied to clipboard.")
                        : tr("%1 paths copied to clipboard.").arg(paths.size()),
      2000);
}

void FileTreeView::SlotRefresh() {
  if (current_path_.isEmpty()) {
    dir_model_->setRootPath(QString());
    setRootIndex(dir_model_->index(QString()));
    slot_adjust_column_widths();

    emit UISignalStation::GetInstance()
        -> SignalRefreshStatusBar(tr("File list refreshed."), 1500);
    return;
  }

  const QFileInfo info(current_path_);
  if (!info.exists() || !info.isDir() || !info.isReadable() ||
      !info.isExecutable()) {
    QMessageBox::warning(
        this, tr("Unable to Refresh"),
        tr("The current folder no longer exists or cannot be opened."));

    SlotGoPath(QDir::homePath());
    return;
  }

  SlotGoPath(current_path_);

  emit UISignalStation::GetInstance()
      -> SignalRefreshStatusBar(tr("File list refreshed."), 1500);
}

void FileTreeView::sync_selected_paths_from_selection() {
  selected_paths_.clear();

  if (selectionModel() == nullptr || dir_model_ == nullptr) {
    emit SignalSelectedChanged(selected_paths_);
    return;
  }

  const auto rows = selectionModel()->selectedRows(0);
  selected_paths_.reserve(rows.size());

  for (const auto& index : rows) {
    if (!index.isValid()) continue;

    const auto path = dir_model_->filePath(index);
    if (path.isEmpty() || path == current_path_) continue;

    selected_paths_.append(path);
  }

  emit SignalSelectedChanged(selected_paths_);
}

void FileTreeView::refresh_current_path_and_select_paths(
    const QStringList& paths) {
  SlotGoPath(current_path_);

  QPointer<FileTreeView> self(this);

  QTimer::singleShot(200, this, [self, paths]() {
    if (self == nullptr) return;
    if (self->dir_model_ == nullptr || self->selectionModel() == nullptr) {
      return;
    }

    self->selectionModel()->clearSelection();

    QModelIndex first_valid_index;

    for (const auto& path : paths) {
      if (path.isEmpty()) continue;

      const auto index = self->dir_model_->index(path);
      if (!index.isValid()) continue;

      if (!first_valid_index.isValid()) {
        first_valid_index = index;
      }

      self->selectionModel()->select(
          index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    if (first_valid_index.isValid()) {
      self->setCurrentIndex(first_valid_index);
      self->scrollTo(first_valid_index, QAbstractItemView::PositionAtCenter);
    } else {
      self->setCurrentIndex(QModelIndex());
    }

    self->sync_selected_paths_from_selection();
  });
}

}  // namespace GpgFrontend::UI
