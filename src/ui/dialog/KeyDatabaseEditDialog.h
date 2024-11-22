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

#pragma once

#include <utility>

#include "core/model/KeyDatabaseInfo.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_KeyDatabaseEditDialog;

namespace GpgFrontend::UI {
class KeyDatabaseEditDialog : public GeneralDialog {
  Q_OBJECT
 public:
  explicit KeyDatabaseEditDialog(QWidget* parent);

  void SetDefaultName(QString name);

  void SetDefaultPath(const QString& path);

 signals:
  void SignalKeyDatabaseInfoAccepted(QString name, QString path);

 private:
  std::shared_ptr<Ui_KeyDatabaseEditDialog> ui_;  ///<
  bool default_;
  QString name_;
  QString path_;
  QList<KeyDatabaseInfo> key_database_infos_;

  void slot_button_box_accepted();

  void slot_show_err_msg(const QString& error_msg);

  void slot_clear_err_msg();

  auto check_custom_gnupg_key_database_path(const QString& path) -> bool;
};

}  // namespace GpgFrontend::UI