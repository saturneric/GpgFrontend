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

#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/CoreTypedef.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_KeyDatabaseEditDialog;

namespace GpgFrontend::UI {
class KeyDatabaseEditDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /// What this dialog is for. Passed in rather than chosen inside, because the
  /// three kinds of key database are not variations the user picks between at
  /// this point: the settings page already knows which list the button belongs
  /// to, and offering the choice here is what let a folder be picked for a
  /// database whose folder is not the user's to choose.
  ///
  /// The DEFAULT database has no mode at all. It is a checkbox on the settings
  /// page, since nothing about it can be typed.
  enum class Mode {
    kADD_MANAGED,     ///< a new database inside the profile; name only
    kRENAME_MANAGED,  ///< an existing one; the name, and its folder, move
    kADD_EXTERNAL,    ///< a new database somewhere on this computer
    kEDIT_EXTERNAL,   ///< an existing one of those
  };

  /// Add a database. @p mode must be one of the two kADD_* values.
  explicit KeyDatabaseEditDialog(Mode mode,
                                 QContainer<KeyDatabaseInfo> key_db_infos,
                                 QWidget* parent);

  /// Edit the database at @p index. @p mode must be kRENAME_MANAGED or
  /// kEDIT_EXTERNAL.
  explicit KeyDatabaseEditDialog(Mode mode,
                                 QContainer<KeyDatabaseInfo> key_db_infos,
                                 int index, QWidget* parent);

 signals:
  void SignalKeyDatabaseInfoAccepted(QString name, QString backend_type,
                                     QString path);

 private:
  QSharedPointer<Ui_KeyDatabaseEditDialog> ui_;  ///<

  Mode mode_;
  int channel_;
  QString default_name_;
  QString default_path_;
  QString name_;
  QString path_;
  QString backend_type_;
  QContainer<KeyDatabaseInfo> key_database_infos_;
  bool is_sandbox_ = false;

  /// A database inside the profile: its folder is derived from its name, so
  /// there is nothing to pick and nothing to make relative.
  [[nodiscard]] auto is_managed() const -> bool {
    return mode_ == Mode::kADD_MANAGED || mode_ == Mode::kRENAME_MANAGED;
  }

  /// Editing something that already exists, rather than making one.
  [[nodiscard]] auto is_editing() const -> bool {
    return mode_ == Mode::kRENAME_MANAGED || mode_ == Mode::kEDIT_EXTERNAL;
  }

  void init_ui();

  /// Show only what this mode actually lets the user decide.
  void apply_mode();

  void update_generated_path();

  /// A name for a new database that is valid, unused, and says which of the two
  /// lists it is about to join.
  [[nodiscard]] auto suggested_name() const -> QString;

  /// Draw the folder the entry will use, elided to whatever room the label has.
  /// A key database path is long and its two informative ends -- the profile it
  /// is in and the folder it is -- are the parts a middle elision keeps, so it
  /// is shown rather than allowed to widen the dialog past the screen.
  void update_path_display();

  void slot_button_box_accepted();

  void slot_show_err_msg(const QString& error_msg);

  void slot_clear_err_msg();

  auto check_custom_gnupg_key_database_path(const QString& path) -> bool;

 protected:
  /// Re-elides the folder, which depends on the width it is given -- and which
  /// is not known until the layout has run, hence both of these.
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
};

}  // namespace GpgFrontend::UI