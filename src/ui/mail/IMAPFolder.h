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
#ifndef GPGFRONTEND_IMAPFOLDER_H
#define GPGFRONTEND_IMAPFOLDER_H

#include "GpgFrontendUI.h"

namespace vmime::net {
class folder;
};

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class IMAPFolder {
 public:
  /**
   * @brief Construct a new IMAPFolder object
   *
   * @param folder
   */
  explicit IMAPFolder(std::shared_ptr<vmime::net::folder> folder);

  /**
   * @brief Set the Parent Folder object
   *
   * @param parent_node
   */
  void SetParentFolder(IMAPFolder* parent_node);

  /**
   * @brief Get the Tree Widget Item object
   *
   * @return QTreeWidgetItem*
   */
  QTreeWidgetItem* GetTreeWidgetItem();

  /**
   * @brief Get the Vmime Folder object
   *
   * @return vmime::net::folder*
   */
  vmime::net::folder* GetVmimeFolder();

 private:
  std::shared_ptr<vmime::net::folder> folder_;  ///<
  QTreeWidgetItem* tree_node_;                  ///<
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_IMAPFOLDER_H
