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
    dirTreeView->setSortingEnabled(true);
    dirTreeView->setRootIndex(dirModel->index(QDir::currentPath()));

    upLevelButton = new QPushButton("UP Level");
    connect(upLevelButton, SIGNAL(clicked(bool)), this, SLOT(slotUpLevel()));

    auto *menuLayout = new QHBoxLayout();
    menuLayout->addWidget(upLevelButton);
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
    mPath = dirModel->fileInfo(currentRoot).absoluteFilePath();
    auto fileInfo = QFileInfo(mPath);
    if(fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
        dirTreeView->setRootIndex(currentRoot.parent());
    }
    qDebug() << "Current Root mPath" << mPath;
}

void FilePage::fileTreeViewItemDoubleClicked(const QModelIndex &index) {
    mPath = dirModel->fileInfo(index).absoluteFilePath();
    auto fileInfo = QFileInfo(mPath);
    if(fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
        dirTreeView->setRootIndex(index);
    }
    qDebug() << "Index mPath" << mPath;
}

void FilePage::getSelected(QString &path) {
    QModelIndex index = dirTreeView->currentIndex();
    QVariant data = dirTreeView->model()->data(index);
    path = data.toString();
    qDebug() << "Target Path" << mPath;
}
