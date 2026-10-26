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

#include <array>

#include "core/model/GpgImportInformation.h"
#include "ui/dialog/GeneralWizard.h"
#include "ui/dialog/import_export/KeyPackageImportSummary.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;

namespace GpgFrontend::UI {

class MetaListPanel;
class SecretEntryPanel;

/**
 * @brief Import a key package in three steps: which files, the PIN, and which
 * of the keys inside.
 *
 * A key package is two files and a PIN: the package holds the keys, the key
 * file holds the passphrase that opens it, and the PIN opens the key file. The
 * key file is picked up from beside the package, where the export wizard puts
 * it by default, until the user chooses another one.
 *
 * Continue on the PIN step opens the package, so a mistyped PIN is answered
 * on the page it was typed on, with the files still chosen. The keys inside
 * are then listed, each saying what importing it would change here, and only
 * the ticked ones are imported. The opened key data is held in a GFBuffer for
 * as long as the wizard is on the last step, and dropped the moment a file or
 * the PIN changes.
 */
class ImportKeyPackageDialog : public GeneralWizard {
  Q_OBJECT

 public:
  /**
   * @brief Construct the wizard.
   *
   * @param channel the key database channel to import into
   * @param parent parent widget
   */
  ImportKeyPackageDialog(int channel, QWidget* parent);

  /**
   * @brief Refuse to close while the package is being opened or imported.
   *
   * @param result
   */
  void done(int result) override;

 signals:
  /**
   * @brief The keys were imported.
   *
   * @param channel the channel they went into, after any database reload
   * @param info what the import reported
   */
  void SignalKeysImported(int channel,
                          QSharedPointer<GpgImportInformation> info);

 private slots:
  void slot_choose_package();
  void slot_choose_key_file();
  void slot_reset_key_file();
  void slot_state_changed();
  void slot_page_changed(int id);
  void slot_key_item_changed();

 private:
  int current_gpg_context_channel_;

  std::array<GeneralWizardPage*, kKeyPackageImportStepCount> pages_{};

  // Files
  QLineEdit* package_edit_{};
  QPushButton* package_button_{};
  QLineEdit* key_file_edit_{};
  QPushButton* key_file_button_{};
  QLabel* key_file_note_{};
  QPushButton* key_file_reset_{};
  QLabel* problem_label_{};

  // Unlock
  SecretEntryPanel* entry_{};
  MetaListPanel* review_{};

  // Keys
  QLabel* selection_label_{};
  QStandardItemModel* keys_model_{};
  QTableView* keys_table_{};

  QString package_path_;
  /// The key file the user chose. Empty while it is the one found beside the
  /// package.
  QString key_file_path_;

  /// The package's decrypted contents, once the PIN has opened it.
  GFBuffer key_data_;
  QList<KeyPackageImportKey> found_;
  bool opened_ = false;
  /// Set while the key table is being filled, when its rows are half built.
  bool filling_ = false;

  bool busy_ = false;
  bool imported_ = false;

  /// @brief Step 1: the package and its key file.
  void build_files_page();

  /// @brief Step 2: the PIN, and which files it opens.
  void build_unlock_page();

  /// @brief Step 3: the keys inside, to tick or untick.
  void build_keys_page();

  /// @brief Fill the key table from what the package holds.
  void fill_key_table();

  /// @brief The fingerprints of the ticked keys.
  [[nodiscard]] auto selected_fprs() const -> QStringList;

  /// @brief Forget the opened package: a file or the PIN changed.
  void drop_opened();

  /// @brief Decrypt the package on the GPG runner, behind a waiting dialog.
  void start_open();

  /// @brief Go on to the keys, or say why the package did not open.
  void finish_open(const QString& error, const GFBuffer& key_data);

  /// @brief Show a failure that retyping the PIN cannot fix.
  void show_failure(const QString& error);

  /// @brief A page for a step, titled and registered under the step's id.
  auto add_page(KeyPackageImportStep step) -> GeneralWizardPage*;

  /// @brief The key file found beside the package, if there is one.
  [[nodiscard]] auto sibling_key_file() const -> QString;

  /// @brief What has been chosen so far, read from disk, for the pure rules.
  [[nodiscard]] auto choice() const -> KeyPackageImportChoice;

  /// @brief Import the ticked keys on the GPG runner, behind a waiting
  /// dialog.
  void start_import();

  /// @brief Close on success; otherwise say why and stay.
  void finish_import(const QString& error,
                     const QSharedPointer<GpgImportInformation>& info);

  /// @brief Keep the buttons from starting a second open or import.
  void set_busy(bool busy);
};

}  // namespace GpgFrontend::UI
