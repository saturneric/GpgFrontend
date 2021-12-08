/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/widgets/FilePage.h"

#include <boost/filesystem.hpp>

#include "ui/MainWindow.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent) : QWidget(parent) {
  firstParent = parent;

  dirModel = new QFileSystemModel();
  dirModel->setRootPath(QDir::currentPath());

  dirTreeView = new QTreeView();
  dirTreeView->setModel(dirModel);
  dirTreeView->setAnimated(true);
  dirTreeView->setIndentation(20);
  dirTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
  dirTreeView->setColumnWidth(0, 320);
  dirTreeView->setRootIndex(dirModel->index(QDir::currentPath()));
  mPath = boost::filesystem::path(dirModel->rootPath().toStdString());

  createPopupMenu();

  upLevelButton = new QPushButton();
  connect(upLevelButton, SIGNAL(clicked(bool)), this, SLOT(slotUpLevel()));

  QString buttonStyle =
      "QPushButton{border:none;background-color:rgba(255, 255, 255, 0);}";

  auto upPixmap = QPixmap(":up.png");
  upPixmap =
      upPixmap.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QIcon upButtonIcon(upPixmap);
  upLevelButton->setIcon(upButtonIcon);
  upLevelButton->setIconSize(upPixmap.rect().size());
  upLevelButton->setStyleSheet(buttonStyle);

  refreshButton = new QPushButton(_("Refresh"));
  connect(refreshButton, SIGNAL(clicked(bool)), this, SLOT(slotGoPath()));

  goPathButton = new QPushButton();
  connect(goPathButton, SIGNAL(clicked(bool)), this, SLOT(slotGoPath()));

  auto updatePixmap = QPixmap(":refresh.png");
  updatePixmap = updatePixmap.scaled(18, 18, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
  QIcon updateButtonIcon(updatePixmap);
  goPathButton->setIcon(updateButtonIcon);
  goPathButton->setIconSize(updatePixmap.rect().size());
  goPathButton->setStyleSheet(buttonStyle);

  pathEdit = new QLineEdit();
  pathEdit->setText(dirModel->rootPath());

  pathEditCompleter = new QCompleter(this);
  pathCompleteModel = new QStringListModel();
  pathEditCompleter->setModel(pathCompleteModel);
  pathEditCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  pathEditCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  pathEdit->setCompleter(pathEditCompleter);

  auto* menuLayout = new QHBoxLayout();
  menuLayout->addWidget(upLevelButton);
  menuLayout->setStretchFactor(upLevelButton, 1);
  menuLayout->addWidget(pathEdit);
  menuLayout->setStretchFactor(pathEdit, 10);
  menuLayout->addWidget(goPathButton);
  menuLayout->setStretchFactor(goPathButton, 1);

  auto* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addLayout(menuLayout);
  layout->setStretchFactor(menuLayout, 1);
  layout->addWidget(dirTreeView);
  layout->setStretchFactor(dirTreeView, 8);

  this->setLayout(layout);

  connect(dirTreeView, &QTreeView::clicked, this,
          &FilePage::fileTreeViewItemClicked);
  connect(dirTreeView, &QTreeView::doubleClicked, this,
          &FilePage::fileTreeViewItemDoubleClicked);
  connect(dirTreeView, &QTreeView::customContextMenuRequested, this,
          &FilePage::onCustomContextMenu);

  connect(pathEdit, &QLineEdit::textChanged, [=]() {
    auto path = pathEdit->text();
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
  QModelIndex currentRoot = dirTreeView->rootIndex();

  mPath = boost::filesystem::path(
      dirModel->fileInfo(currentRoot).absoluteFilePath().toStdString());
  LOG(INFO) << "get path" << mPath;
  if (mPath.has_parent_path()) {
    mPath = mPath.parent_path();
    LOG(INFO) << "parent path" << mPath;
    pathEdit->setText(mPath.string().c_str());
    slotGoPath();
  }
}

void FilePage::fileTreeViewItemDoubleClicked(const QModelIndex& index) {
  pathEdit->setText(dirModel->fileInfo(index).absoluteFilePath());
  slotGoPath();
}

QString FilePage::getSelected() const {
  return QString::fromStdString(selectedPath.string());
}

void FilePage::slotGoPath() {
  const auto path_edit = pathEdit->text().toStdString();
  LOG(INFO) << "get path edit" << path_edit;
  if (mPath.string() != path_edit) mPath = path_edit;
  auto fileInfo = QFileInfo(mPath.string().c_str());
  if (fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
    mPath = boost::filesystem::path(fileInfo.filePath().toStdString());
    LOG(INFO) << "set path" << mPath;
    dirTreeView->setRootIndex(dirModel->index(fileInfo.filePath()));
    for (int i = 1; i < dirModel->columnCount(); ++i) {
      dirTreeView->resizeColumnToContents(i);
    }
    pathEdit->setText(mPath.generic_path().string().c_str());

  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The path is not exists, unprivileged or unreachable."));
  }
  emit pathChanged(mPath.string().c_str());
}

void FilePage::createPopupMenu() {
  popUpMenu = new QMenu();

  auto openItemAct = new QAction(_("Open"), this);
  connect(openItemAct, SIGNAL(triggered()), this, SLOT(slotOpenItem()));
  auto renameItemAct = new QAction(_("Rename"), this);
  connect(renameItemAct, SIGNAL(triggered()), this, SLOT(slotRenameItem()));
  auto deleteItemAct = new QAction(_("Delete"), this);
  connect(deleteItemAct, SIGNAL(triggered()), this, SLOT(slotDeleteItem()));
  encryptItemAct = new QAction(_("Encrypt Sign"), this);
  connect(encryptItemAct, SIGNAL(triggered()), this, SLOT(slotEncryptItem()));
  decryptItemAct =
      new QAction(QString(_("Decrypt Verify")) + " " + _("(.gpg .asc)"), this);
  connect(decryptItemAct, SIGNAL(triggered()), this, SLOT(slotDecryptItem()));
  signItemAct = new QAction(_("Sign"), this);
  connect(signItemAct, SIGNAL(triggered()), this, SLOT(slotSignItem()));
  verifyItemAct =
      new QAction(QString(_("Verify")) + " " + _("(.sig .gpg .asc)"), this);
  connect(verifyItemAct, SIGNAL(triggered()), this, SLOT(slotVerifyItem()));

  hashCalculateAct = new QAction(_("Calculate Hash"), this);
  connect(hashCalculateAct, SIGNAL(triggered()), this,
          SLOT(slotCalculateHash()));

  popUpMenu->addAction(openItemAct);
  popUpMenu->addAction(renameItemAct);
  popUpMenu->addAction(deleteItemAct);
  popUpMenu->addSeparator();
  popUpMenu->addAction(encryptItemAct);
  popUpMenu->addAction(decryptItemAct);
  popUpMenu->addAction(signItemAct);
  popUpMenu->addAction(verifyItemAct);
  popUpMenu->addSeparator();
  popUpMenu->addAction(hashCalculateAct);
}

void FilePage::onCustomContextMenu(const QPoint& point) {
  QModelIndex index = dirTreeView->indexAt(point);
  selectedPath = boost::filesystem::path(
      dirModel->fileInfo(index).absoluteFilePath().toStdString());
  LOG(INFO) << "right click" << selectedPath;
  if (index.isValid()) {
    QFileInfo info(QString::fromStdString(selectedPath.string()));
    encryptItemAct->setEnabled(
        info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig"));
    encryptItemAct->setEnabled(
        info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig"));
    decryptItemAct->setEnabled(info.isFile() && info.suffix() == "gpg");
    signItemAct->setEnabled(info.isFile() &&
                            (info.suffix() != "gpg" && info.suffix() != "sig"));
    verifyItemAct->setEnabled(
        info.isFile() && (info.suffix() == "sig" || info.suffix() == "gpg"));
    hashCalculateAct->setEnabled(info.isFile() && info.isReadable());

    popUpMenu->exec(dirTreeView->viewport()->mapToGlobal(point));
  }
}

void FilePage::slotOpenItem() {
  QFileInfo info(QString::fromStdString(selectedPath.string()));
  if (info.isDir()) {
    if (info.isReadable() && info.isExecutable()) {
      const auto file_path = info.filePath().toStdString();
      LOG(INFO) << "set path" << file_path;
      pathEdit->setText(info.filePath());
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
      if (mainWindow != nullptr) mainWindow->slotOpenFile(qt_path);
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
  QModelIndex index = dirTreeView->currentIndex();
  QVariant data = dirTreeView->model()->data(index);

  auto ret = QMessageBox::warning(this, _("Warning"),
                                  _("Are you sure you want to delete it?"),
                                  QMessageBox::Ok | QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  qDebug() << "Delete Item" << data.toString();

  if (!dirModel->remove(index)) {
    QMessageBox::critical(this, _("Error"),
                          _("Unable to delete the file or folder."));
  }
}

void FilePage::slotEncryptItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileEncryptSign();
}

void FilePage::slotDecryptItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileDecryptVerify();
}

void FilePage::slotSignItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileSign();
}

void FilePage::slotVerifyItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileVerify();
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

void FilePage::keyPressEvent(QKeyEvent* event) {
  qDebug() << "Key Press" << event->key();
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    slotGoPath();
  }
}

}  // namespace GpgFrontend::UI
