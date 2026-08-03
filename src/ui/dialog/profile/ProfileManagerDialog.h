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

#include "core/function/ProfilePackage.h"
#include "core/function/ProfileRegistry.h"
#include "ui/dialog/GeneralDialog.h"

class QTableWidget;
class QPushButton;
class QLabel;

namespace GpgFrontend::UI {

/**
 * @brief Pick a profile — a local one or a package — and open it.
 *
 * Opening never disturbs the window it was opened from: each profile gets its
 * own process, so the profile in front of the user stays exactly where it was.
 *
 * Creating and deleting live here too, because this is where the user is
 * already looking at the list. Classic and portable appear like anything else
 * but cannot be deleted: they exist whether or not the registry knows about
 * them, and there is no meaningful "remove" for a directory the application
 * resolves on its own.
 */
class ProfileManagerDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct the dialog.
   *
   * @param parent parent widget
   */
  explicit ProfileManagerDialog(QWidget* parent);

 private slots:
  void slot_open();
  void slot_create();
  void slot_export();
  void slot_import();
  void slot_delete();
  void slot_reveal();
  void slot_selection_changed();

 private:
  QTableWidget* table_{};
  QPushButton* open_button_{};
  QPushButton* create_button_{};
  QPushButton* export_button_{};
  QPushButton* import_button_{};
  QPushButton* delete_button_{};
  QPushButton* reveal_button_{};
  QLabel* hint_{};

  ProfileRegistryData data_;

  void init_ui();
  void reload();

  /**
   * @brief Ask for a name for an imported profile, until it is usable.
   *
   * @param suggestion the name the package carried
   * @param taken ids already in use here
   * @param id receives the sanitised folder name
   * @return the display name, empty if the user gave up
   */
  auto ask_import_name(const QString& suggestion, const QStringList& taken,
                       QString& id) -> QString;

  /**
   * @brief Turn a package that has been read into a profile, or explain why
   * not.
   *
   * Separate from the reading because naming the profile is a question for the
   * user, and the reading happens on a worker thread where there is nobody to
   * ask.
   *
   * @param package_path the file it came from, for the messages
   * @param staging_dir the extracted tree; removed before this returns
   * @param result what the reader made of it
   */
  void finish_import(const QString& package_path, const QString& staging_dir,
                     const ProfilePackageReadResult& result);

  /**
   * @brief The entry the user has selected, if any.
   *
   * @return the entry, or nothing
   */
  [[nodiscard]] auto selected() const -> std::optional<ProfileRegistryEntry>;
};

}  // namespace GpgFrontend::UI
