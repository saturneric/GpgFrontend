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

#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief Create a custom key category, or edit the name and colour of one.
 *
 * The user types a name and picks a colour from a row of swatches (or any
 * colour through "Custom..."), with a preview of how the category will look
 * in the category list.
 */
class KeyCategoryEditDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct the dialog.
   *
   * @param channel key database channel the category belongs to
   * @param category_id the category to edit, or empty to create a new one
   * @param key_ids keys to add to a newly created category
   * @param colors_in_use colours already shown in the category list, built-in
   * tabs included, which a new category should not repeat
   * @param parent parent widget
   */
  KeyCategoryEditDialog(int channel, QString category_id,
                        QStringList key_ids = {},
                        QList<QColor> colors_in_use = {},
                        QWidget* parent = nullptr);

 signals:
  /**
   * @brief Emitted once the category has been created or saved.
   *
   * @param id the category id
   */
  void SignalSaved(const QString& id);

 protected:
  void showEvent(QShowEvent* event) override;

  void changeEvent(QEvent* event) override;

 private:
  /**
   * @brief Colour the secondary text, the preview frame and the error line
   * from the current palette, so they stay legible in both themes.
   */
  void apply_theme();

  /**
   * @brief Make @p color the selected one, putting it in the custom slot when
   * it is not a preset.
   */
  void select_color(const QColor& color);

  /**
   * @brief Open the colour picker for a colour outside the presets.
   */
  void slot_choose_custom_color();

  /**
   * @brief Re-run validation, gate the save button and redraw the preview.
   */
  void update_state();

  /**
   * @brief Write the category and close.
   */
  void slot_save();

  [[nodiscard]] auto is_editing() const -> bool;

  int current_gpg_context_channel_;
  QString category_id_;
  QStringList key_ids_;

  QString initial_name_;
  QColor initial_color_;
  QColor selected_color_;
  QStringList taken_names_;

  QList<QLabel*> muted_labels_;

  QLineEdit* name_edit_{};
  QFrame* preview_{};
  QLabel* preview_icon_{};
  QLabel* preview_name_{};
  QButtonGroup* swatch_group_{};
  QToolButton* custom_swatch_{};
  QPushButton* save_button_{};
  QLabel* error_label_{};
};

}  // namespace GpgFrontend::UI
