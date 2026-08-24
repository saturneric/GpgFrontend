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

#include "ui/function/GpgErrorMessageBox.h"

#include "core/utils/GpgUtils.h"

namespace GpgFrontend::UI {

void RaiseMessageBox(QWidget* parent, GpgError err) {
  GpgErrorCode err_code = CheckGpgError2ErrCode(err);

  if (err_code == GPG_ERR_NO_ERROR) {
    QMessageBox::information(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Success"),
        QCoreApplication::translate("GpgFrontend::UI",
                                    "Operation completed successfully."));
  } else {
    RaiseFailureMessageBox(parent, err);
  }
}

void RaiseFailureMessageBox(QWidget* parent, GpgError err, const QString& msg) {
  GpgErrorDesc desc = DescribeGpgErrCode(err);
  GpgErrorCode err_code = CheckGpgError2ErrCode(err);

  QMessageBox::critical(
      parent, QCoreApplication::translate("GpgFrontend::UI", "Failure"),
      QCoreApplication::translate("GpgFrontend::UI", "Gpg Operation failed.") +
          "\n\n" +
          QCoreApplication::translate("GpgFrontend::UI", "Error code: %1")
              .arg(err_code) +
          "\n\n\n" +
          QCoreApplication::translate("GpgFrontend::UI", "Source:  %1")
              .arg(desc.first) +
          "\n" +
          QCoreApplication::translate("GpgFrontend::UI", "Description: %1")
              .arg(desc.second) +
          "\n" +
          QCoreApplication::translate("GpgFrontend::UI", "Error Message: %1")
              .arg(msg));
}

}  // namespace GpgFrontend::UI
