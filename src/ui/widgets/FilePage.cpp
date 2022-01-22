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

#include <boost/filesystem.hpp>
#include <string>

#include "ui/MainWindow.h"
#include "ui/SignalStation.h"
#include "ui_FilePage.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent)
    : QWidget(parent), ui(std::make_shared<Ui_FilePage>()) {
  ui->setupUi(this);

  firstParent = parent;

  dirModel = new QFileSystemModel();
  dirModel->setRootPath(QDir::currentPath());
  dirModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

  ui->fileTreeView->setModel(dirModel);
  ui->fileTreeView->setColumnWidth(0, 320);
  ui->fileTreeView->sortByColumn(0, Qt::AscendingOrder);
  mPath = boost::filesystem::path(dirModel->rootPath().toStdString());

  createPopupMenu();

  connect(ui->upPathButton, &QPushButton::clicked, this,
          &FilePage::slotUpLevel);
  connect(ui->refreshButton, &QPushButton::clicked, this,
          &FilePage::slotGoPath);
  ui->optionsButton->setMenu(optionPopUpMenu);

  ui->pathEdit->setText(dirModel->rootPath());

  pathEditCompleter = new QCompleter(this);
  pathCompleteModel = new QStringListModel();
  pathEditCompleter->setModel(pathCompleteModel);
  pathEditCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  pathEditCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  ui->pathEdit->setCompleter(pathEditCompleter);

  connect(ui->fileTreeView, &QTreeView::clicked, this,
          &FilePage::fileTreeViewItemClicked);
  connect(ui->fileTreeView, &QTreeView::doubleClicked, this,
          &FilePage::fileTreeViewItemDoubleClicked);
  connect(ui->fileTreeView, &QTreeView::customContextMenuRequested, this,
          &FilePage::onCustomContextMenu);

  connect(ui->pathEdit, &QLineEdit::textChanged, [=]() {
    auto path = ui->pathEdit->text();
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
      pathCompleteModel->setStringList(paths);
    }
  });

  connect(this, &FilePage::signalRefreshInfoBoard, SignalStation::GetInstance(),
          &SignalStation::signalRefreshInfoBoard);
}

void FilePage::fileTreeViewItemClicked(const QModelIndex& index) {
  selectedPath = boost::filesystem::path(
      dirModel->fileInfo(index).absoluteFilePath().toStdString());
  mPath = selectedPath;
  LOG(INFO) << "selected path" << selectedPath;
}

void FilePage::slotUpLevel() {
  QModelIndex currentRoot = ui->fileTreeView->rootIndex();

  auto utf8_path =
      dirModel->fileInfo(currentRoot).absoluteFilePath().toStdString();
  boost::filesystem::path path_obj(utf8_path);

  mPath = path_obj;
  LOG(INFO) << "get path" << mPath;
  if (mPath.has_parent_path() && !mPath.parent_path().empty()) {
    mPath = mPath.parent_path();
    LOG(INFO) << "parent path" << mPath;
    ui->pathEdit->setText(mPath.string().c_str());
    this->slotGoPath();
  }
}

void FilePage::fileTreeViewItemDoubleClicked(const QModelIndex& index) {
  QFileInfo file_info(dirModel->fileInfo(index).absoluteFilePath());
  if (file_info.isFile()) {
    slotOpenItem();
  } else {
    ui->pathEdit->setText(file_info.filePath());
    slotGoPath();
  }
}

QString FilePage::getSelected() const {
  return QString::fromStdString(selectedPath.string());
}

void FilePage::slotGoPath() {
  const auto path_edit = ui->pathEdit->text().toStdString();
  boost::filesystem::path path_obj(path_edit);

  if (mPath.string() != path_edit) mPath = path_obj;
  auto fileInfo = QFileInfo(mPath.string().c_str());
  if (fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
    mPath = boost::filesystem::path(fileInfo.filePath().toStdString());
    LOG(INFO) << "set path" << mPath;
    ui->fileTreeView->setRootIndex(dirModel->index(fileInfo.filePath()));
    dirModel->setRootPath(fileInfo.filePath());
    for (int i = 1; i < dirModel->columnCount(); ++i) {
      ui->fileTreeView->resizeColumnToContents(i);
    }
    ui->pathEdit->setText(mPath.generic_path().string().c_str());
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The path is not exists, unprivileged or unreachable."));
  }
  emit pathChanged(mPath.string().c_str());
}

void FilePage::createPopupMenu() {
  popUpMenu = new QMenu();

  ui->actionOpenFile->setText(_("Open"));
  connect(ui->actionOpenFile, &QAction::triggered, this,
          &FilePage::slotOpenItem);
  ui->actionRenameFile->setText(_("Rename"));
  connect(ui->actionRenameFile, &QAction::triggered, this,
          &FilePage::slotRenameItem);
  ui->actionDeleteFile->setText(_("Delete"));
  connect(ui->actionDeleteFile, &QAction::triggered, this,
          &FilePage::slotDeleteItem);

  ui->actionEncrypt->setText(_("Encrypt"));
  connect(ui->actionEncrypt, &QAction::triggered, this,
          &FilePage::slotEncryptItem);
  ui->actionEncryptSign->setText(_("Encrypt Sign"));
  connect(ui->actionEncryptSign, &QAction::triggered, this,
          &FilePage::slotEncryptSignItem);
  ui->actionDecrypt->setText(QString(_("Decrypt Verify")) + " " +
                             _("(.gpg .asc)"));
  connect(ui->actionDecrypt, &QAction::triggered, this,
          &FilePage::slotDecryptItem);
  ui->actionSign->setText(_("Sign"));
  connect(ui->actionSign, &QAction::triggered, this, &FilePage::slotSignItem);
  ui->actionVerify->setText(QString(_("Verify")) + " " + _("(.sig .gpg .asc)"));
  connect(ui->actionVerify, &QAction::triggered, this,
          &FilePage::slotVerifyItem);

  ui->actionCalculateHash->setText(_("Calculate Hash"));
  connect(ui->actionCalculateHash, &QAction::triggered, this,
          &FilePage::slotCalculateHash);

  ui->actionMakeDirectory->setText(_("Make New Directory"));
  connect(ui->actionMakeDirectory, &QAction::triggered, this,
          &FilePage::slotMkdir);

  ui->actionCreateEmptyFile->setText(_("Create Empty File"));
  connect(ui->actionCreateEmptyFile, &QAction::triggered, this,
          &FilePage::slotCreateEmptyFile);

  popUpMenu->addAction(ui->actionOpenFile);
  popUpMenu->addAction(ui->actionRenameFile);
  popUpMenu->addAction(ui->actionDeleteFile);
  popUpMenu->addSeparator();
  popUpMenu->addAction(ui->actionEncrypt);
  popUpMenu->addAction(ui->actionEncryptSign);
  popUpMenu->addAction(ui->actionDecrypt);
  popUpMenu->addAction(ui->actionSign);
  popUpMenu->addAction(ui->actionVerify);
  popUpMenu->addSeparator();
  popUpMenu->addAction(ui->actionMakeDirectory);
  popUpMenu->addAction(ui->actionCreateEmptyFile);
  popUpMenu->addAction(ui->actionCalculateHash);

  optionPopUpMenu = new QMenu();

  auto showHiddenAct = new QAction(_("Show Hidden File"), this);
  showHiddenAct->setCheckable(true);
  connect(showHiddenAct, &QAction::triggered, this, [&](bool checked) {
    LOG(INFO) << "Set Hidden" << checked;
    if (checked)
      dirModel->setFilter(dirModel->filter() | QDir::Hidden);
    else
      dirModel->setFilter(dirModel->filter() & ~QDir::Hidden);
    dirModel->setRootPath(mPath.string().c_str());
  });
  optionPopUpMenu->addAction(showHiddenAct);

  auto showSystemAct = new QAction(_("Show System File"), this);
  showSystemAct->setCheckable(true);
  connect(showSystemAct, &QAction::triggered, this, [&](bool checked) {
    LOG(INFO) << "Set Hidden" << checked;
    if (checked)
      dirModel->setFilter(dirModel->filter() | QDir::System);
    else
      dirModel->setFilter(dirModel->filter() & ~QDir::System);
    dirModel->setRootPath(mPath.string().c_str());
  });
  optionPopUpMenu->addAction(showSystemAct);
}

void FilePage::onCustomContextMenu(const QPoint& point) {
  QModelIndex index = ui->fileTreeView->indexAt(point);
  selectedPath = boost::filesystem::path(
      dirModel->fileInfo(index).absoluteFilePath().toStdString());
  LOG(INFO) << "right click" << selectedPath;
  if (index.isValid()) {
    ui->actionOpenFile->setEnabled(true);
    ui->actionRenameFile->setEnabled(true);
    ui->actionDeleteFile->setEnabled(true);

    QFileInfo info(QString::fromStdString(selectedPath.string()));
    ui->actionEncrypt->setEnabled(info.isFile() && (info.suffix() != "gpg" &&
                                                    info.suffix() != "sig" &&
                                                    info.suffix() != "asc"));
    ui->actionEncryptSign->setEnabled(
        info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig" &&
                          info.suffix() != "asc"));
    ui->actionDecrypt->setEnabled(
        info.isFile() && (info.suffix() == "gpg" || info.suffix() == "asc"));
    ui->actionSign->setEnabled(info.isFile() && (info.suffix() != "gpg" &&
                                                 info.suffix() != "sig" &&
                                                 info.suffix() != "asc"));
    ui->actionVerify->setEnabled(info.isFile() && (info.suffix() == "sig" ||
                                                   info.suffix() == "gpg" ||
                                                   info.suffix() == "asc"));
    ui->actionCalculateHash->setEnabled(info.isFile() && info.isReadable());
  } else {
    ui->actionOpenFile->setEnabled(false);
    ui->actionRenameFile->setEnabled(false);
    ui->actionDeleteFile->setEnabled(false);

    ui->actionEncrypt->setEnabled(false);
    ui->actionEncryptSign->setEnabled(false);
    ui->actionDecrypt->setEnabled(false);
    ui->actionSign->setEnabled(false);
    ui->actionVerify->setEnabled(false);
    ui->actionCalculateHash->setEnabled(false);
  }
  popUpMenu->exec(ui->fileTreeView->viewport()->mapToGlobal(point));
}

void FilePage::slotOpenItem() {
  QFileInfo info(QString::fromStdString(selectedPath.string()));
  if (info.isDir()) {
    if (info.isReadable() && info.isExecutable()) {
      const auto file_path = info.filePath().toStdString();
      LOG(INFO) << "set path" << file_path;
      ui->pathEdit->setText(info.filePath());
      slotGoPath();
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The directory is unprivileged or unreachable."));
    }
  } else {
    if (info.isReadable()) {
      auto mainWindow = qobject_cast<MainWindow*>(firstParent);
      LOG(INFO) << "open item" << selectedPath;
      auto qt_path = QString::fromStdString(selectedPath.string());
      if (mainWindow != nullptr) mainWindow->SlotOpenFile(qt_path);
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The file is unprivileged or unreachable."));
    }
  }
}

void FilePage::slotRenameItem() {
  auto new_name_path = selectedPath, old_name_path = selectedPath;
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
      boost::filesystem::rename(old_name_path, new_name_path);
      // refresh
      this->slotGoPath();
    } catch (...) {
      LOG(ERROR) << "rename error" << new_name_path;
      QMessageBox::critical(this, _("Error"),
                            _("Unable to rename the file or folder."));
    }
  }
}

void FilePage::slotDeleteItem() {
  QModelIndex index = ui->fileTreeView->currentIndex();
  QVariant data = ui->fileTreeView->model()->data(index);

  auto ret = QMessageBox::warning(this, _("Warning"),
                                  _("Are you sure you want to delete it?"),
                                  QMessageBox::Ok | QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  LOG(INFO) << "Delete Item" << data.toString().toStdString();

  if (!dirModel->remove(index)) {
    QMessageBox::critical(this, _("Error"),
                          _("Unable to delete the file or folder."));
  }
}

void FilePage::slotEncryptItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->SlotFileEncrypt();
}

void FilePage::slotEncryptSignItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->SlotFileEncryptSign();
}

void FilePage::slotDecryptItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->SlotFileDecryptVerify();
}

void FilePage::slotSignItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->SlotFileSign();
}

void FilePage::slotVerifyItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->SlotFileVerify();
}

void FilePage::slotCalculateHash() {
  // Returns empty QByteArray() on failure.
  QFileInfo info(QString::fromStdString(selectedPath.string()));

  if (info.isFile() && info.isReadable()) {
    std::stringstream ss;

    ss << "[#] " << _("File Hash Information") << std::endl;
    ss << "    " << _("filename") << _(": ")
       << selectedPath.filename().string().c_str() << std::endl;

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

      emit signalRefreshInfoBoard(ss.str().c_str(),
                                  InfoBoardStatus::INFO_ERROR_OK);
    }
  }
}

void FilePage::slotMkdir() {
  auto index = ui->fileTreeView->rootIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name =
      QInputDialog::getText(this, _("Make New Directory"), _("Directory Name"),
                            QLineEdit::Normal, new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dirModel->mkdir(index, new_dir_name);
  }
}

void FilePage::slotCreateEmptyFile() {
  auto root_path_str = dirModel->rootPath().toStdString();
  boost::filesystem::path root_path(root_path_str);

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
  if (ui->pathEdit->hasFocus() &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    slotGoPath();
  } else if (ui->fileTreeView->currentIndex().isValid()) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
      slotOpenItem();
    else if (event->key() == Qt::Key_Delete ||
             event->key() == Qt::Key_Backspace)
      slotDeleteItem();
  }
}

}  // namespace GpgFrontend::UI
