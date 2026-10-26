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

#include "core/typedef/GpgTypedef.h"
#include "ui/dialog/GeneralWizard.h"
#include "ui/dialog/import_export/KeyPackageExportSummary.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;

namespace GpgFrontend::UI {

class MetaListPanel;
class SecretEntryPanel;

/**
 * @brief Export keys as a key package in three steps: what goes in, where it is
 * written, and what protects it.
 *
 * A key package is two files and a PIN. The package holds the keys, encrypted
 * with a random passphrase made at export; the key file holds that passphrase,
 * sealed with the PIN. Importing needs all three, and nothing can be recovered
 * without any one of them. The package's name and folder come first; the key
 * file follows them, under the same name beside the package, until the user
 * puts it somewhere else. The PIN comes last, right before the button that
 * uses it, so it never arrives as a surprise after the last button.
 *
 * A GeneralWizard, matching Export Profile. The pages live for the wizard's
 * lifetime, so Back and Continue never lose what was typed or ticked, and a
 * failed export leaves everything in place for another try.
 */
class ExportKeyPackageDialog : public GeneralWizard {
  Q_OBJECT

 public:
  /**
   * @brief How the content options start.
   */
  struct Preset {
    bool include_private = false;
    bool only_with_private = false;
  };

  /**
   * @brief Construct the wizard.
   *
   * @param channel the key database channel the keys come from
   * @param keys the keys to offer
   * @param preset how the content options start
   * @param parent parent widget
   */
  ExportKeyPackageDialog(int channel, GpgAbstractKeyPtrList keys, Preset preset,
                         QWidget* parent);

  /**
   * @brief Refuse to close while the files are being written.
   *
   * @param result
   */
  void done(int result) override;

 protected:
  /// @brief Recolour the left-out keys when the palette changes.
  void changeEvent(QEvent* event) override;

 private slots:
  void slot_choose_key_file();
  void slot_reset_key_file();
  void slot_choose_location();
  void slot_generate_name();
  void slot_state_changed();
  void slot_page_changed(int id);

 private:
  int current_gpg_context_channel_;
  GpgAbstractKeyPtrList keys_;
  QList<KeyPackageKeyEntry> entries_;
  int with_private_ = 0;

  std::array<GeneralWizardPage*, kKeyPackageExportStepCount> pages_{};

  // Contents
  QLabel* contents_summary_{};
  QCheckBox* only_private_box_{};
  QCheckBox* include_private_box_{};
  QLabel* private_warning_label_{};
  QLabel* limits_label_{};
  QStandardItemModel* keys_model_{};
  QTableView* keys_table_{};

  // Location
  QLineEdit* name_edit_{};
  QLineEdit* location_edit_{};
  QPushButton* location_button_{};
  QLineEdit* key_file_edit_{};
  QPushButton* key_file_button_{};
  QLabel* key_file_note_{};
  QPushButton* key_file_reset_{};
  QLabel* problem_label_{};

  // Protection
  SecretEntryPanel* entry_{};
  MetaListPanel* review_{};

  /// Where the user put the key file, when it is not beside the package.
  /// Empty while it follows the package's name and folder.
  QString key_file_path_;
  QString location_;
  bool location_writable_ = false;

  /// Paths whose replacement the native picker already confirmed. Anything
  /// else that exists is asked about once, right before it is written.
  QSet<QString> confirmed_paths_;

  bool exporting_ = false;
  bool exported_ = false;

  /// @brief Step 1: the keys, a one-line count, and the two content options.
  void build_contents_page(Preset preset);

  /// @brief Step 2: the package's name and folder, and the key file.
  void build_location_page();

  /// @brief Step 3: the PIN for the key file, and the summary.
  void build_protection_page();

  /// @brief Refill the key table for the current choice.
  void refresh_key_table(const KeyPackageExportChoice& choice);

  /// @brief A page for a step, titled and registered under the step's id.
  auto add_page(KeyPackageExportStep step) -> GeneralWizardPage*;

  /// @brief What has been chosen so far, for the pure rules.
  [[nodiscard]] auto choice() const -> KeyPackageExportChoice;

  /// @brief Set the folder the package goes into.
  void set_location(const QString& location);

  /// @brief Ask before replacing a file the picker never asked about.
  /// @return false when the user declined
  auto confirm_replacements(const QStringList& paths) -> bool;

  /// @brief Write both files; the wizard closes only once they exist.
  void start_export();

  /// @brief Show how the export went, and close on success.
  void finish_export(bool success, const QString& error,
                     const QString& package_path, const QString& key_path);

  /// @brief Keep the buttons from starting a second export.
  void set_busy(bool busy);
};

}  // namespace GpgFrontend::UI
