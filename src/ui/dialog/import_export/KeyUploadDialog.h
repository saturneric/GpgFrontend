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

#ifndef GPGFRONTEND_KEYUPLOADWIDGET_H
#define GPGFRONTEND_KEYUPLOADWIDGET_H

#include "core/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyUploadDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Upload Dialog object
   *
   * @param keys_ids
   * @param parent
   */
  explicit KeyUploadDialog(const KeyIdArgsListPtr& keys_ids, QWidget* parent);

 public slots:

  /**
   * @brief
   *
   */
  void SlotUpload();

 private slots:

  /**
   * @brief
   *
   * @param keys_data
   */
  void slot_upload_key_to_server(const GpgFrontend::ByteArray& keys_data);

  /**
   * @brief
   *
   */
  void slot_upload_finished();

 private:
  KeyListPtr m_keys_;      ///<
  QByteArray m_key_data_;  ///<
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYUPLOADWIDGET_H
