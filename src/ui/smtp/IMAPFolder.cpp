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

#include "IMAPFolder.h"

#include <utility>
#include <vmime/vmime.hpp>

GpgFrontend::UI::IMAPFolder::IMAPFolder(
    std::shared_ptr<vmime::net::folder> folder)
    : folder_(std::move(folder)),
      tree_node_(new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr),
                                     {"server"})) {
  const std::string folder_name = folder_->getName().getBuffer();
  LOG(INFO) << "folder" << folder_name;

  const vmime::net::folderAttributes attr = folder_->getAttributes();
  std::ostringstream attrStr;

  tree_node_->setIcon(0, QIcon(":folder.png"));
  if (attr.getSpecialUse() == vmime::net::folderAttributes::SPECIALUSE_ALL) {
    LOG(INFO) << "use:All";
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_ARCHIVE) {
    tree_node_->setIcon(0, QIcon(":archive.png"));
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_DRAFTS) {
    tree_node_->setIcon(0, QIcon(":drafts.png"));
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_FLAGGED) {
    tree_node_->setIcon(0, QIcon(":flag.png"));
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_JUNK) {
    tree_node_->setIcon(0, QIcon(":junk.png"));
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_SENT) {
    tree_node_->setIcon(0, QIcon(":sent.png"));
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_TRASH) {
    tree_node_->setIcon(0, QIcon(":trash.png"));
  } else if (attr.getSpecialUse() ==
             vmime::net::folderAttributes::SPECIALUSE_IMPORTANT) {
    tree_node_->setIcon(0, QIcon(":importance.png"));
  }

  if (attr.getFlags() & vmime::net::folderAttributes::FLAG_HAS_CHILDREN) {
    LOG(INFO) << " flag:HasChildren";
  }
  if (attr.getFlags() & vmime::net::folderAttributes::FLAG_NO_OPEN) {
    LOG(INFO) << " flag:NoOpen";
    // tree_node_->setDisabled(true);
  }

  if (!folder_name.empty())
    tree_node_->setText(0, folder_name.c_str());
  else
    tree_node_->setIcon(0, QIcon(":server.png"));

}

void GpgFrontend::UI::IMAPFolder::SetParentFolder(IMAPFolder *parent_folder) {
  parent_folder->GetTreeWidgetItem()->addChild(tree_node_);
}

QTreeWidgetItem *GpgFrontend::UI::IMAPFolder::GetTreeWidgetItem() {
  return tree_node_;
}

vmime::net::folder *GpgFrontend::UI::IMAPFolder::GetVmimeFolder() {
  return folder_.get();
}
