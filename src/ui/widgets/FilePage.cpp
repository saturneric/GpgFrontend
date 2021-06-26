/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

FilePage::FilePage(QWidget *parent) : QWidget(parent) {
    qDebug() << "New File Page";

    dirModel = new QFileSystemModel();
    dirModel->setRootPath(QDir::currentPath());

    dirTreeView = new QTreeView();
    dirTreeView->setModel(dirModel);
    dirTreeView->setAnimated(true);
    dirTreeView->setIndentation(20);
    dirTreeView->setRootIndex(dirModel->index(QDir::currentPath()));

    upLevelButton = new QPushButton("UP Level");
    connect(upLevelButton, SIGNAL(clicked(bool)), this, SLOT(slotUpLevel()));

    goPathButton = new QPushButton("Go Path");
    connect(goPathButton, SIGNAL(clicked(bool)), this, SLOT(slotGoPath()));

    pathEdit = new QLineEdit();
    pathEdit->setFixedWidth(520);
    pathEdit->setText(dirModel->rootPath());

    auto *menuLayout = new QHBoxLayout();
    menuLayout->addWidget(upLevelButton);
    menuLayout->addWidget(pathEdit);
    menuLayout->addWidget(goPathButton);
    menuLayout->addStretch(0);

    auto *layout = new QVBoxLayout();
    layout->addLayout(menuLayout);
    layout->addWidget(dirTreeView);

    this->setLayout(layout);

    connect(dirTreeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(fileTreeViewItemClicked(const QModelIndex &)));
    connect(dirTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(fileTreeViewItemDoubleClicked(const QModelIndex &)));

}

void FilePage::fileTreeViewItemClicked(const QModelIndex &index) {
    mPath = dirModel->fileInfo(index).absoluteFilePath();
    qDebug() << "mPath" << mPath;
}

void FilePage::slotUpLevel() {
    QModelIndex currentRoot = dirTreeView->rootIndex();
    mPath = dirModel->fileInfo(currentRoot.parent()).absoluteFilePath();
    auto fileInfo = QFileInfo(mPath);
    if(fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
        dirTreeView->setRootIndex(currentRoot.parent());
        pathEdit->setText(mPath);
    }
    qDebug() << "Current Root mPath" << mPath;
}

void FilePage::fileTreeViewItemDoubleClicked(const QModelIndex &index) {
    mPath = dirModel->fileInfo(index).absoluteFilePath();
    auto fileInfo = QFileInfo(mPath);
    if(fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
        dirTreeView->setRootIndex(index);
        pathEdit->setText(mPath);
    }
    qDebug() << "Index mPath" << mPath;
}

QString FilePage::getSelected() const {
    QModelIndex index = dirTreeView->currentIndex();
    QVariant data = dirTreeView->model()->data(index);
    qDebug() << "Target Path" << mPath;
    return data.toString();
}

void FilePage::slotGoPath() {
    qDebug() << "getSelected" << pathEdit->text();
    auto fileInfo = QFileInfo(pathEdit->text());
    if(fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
        qDebug() << "Set Path" << fileInfo.filePath();
        dirTreeView->setRootIndex(dirModel->index(fileInfo.filePath()));
    } else {
        QMessageBox::critical(this, "Error", "The path is unprivileged or unreachable.");
    }
}
