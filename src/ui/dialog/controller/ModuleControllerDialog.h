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

#include "core/module/Module.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_ModuleControllerDialog;

namespace GpgFrontend::UI {

class ModuleListView;

class ModuleControllerDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Module Controller Dialog object
   *
   * @param parent
   */
  explicit ModuleControllerDialog(QWidget* parent);

 private slots:
  /**
   * @brief Fill the detail panel with the given module's information.
   *
   * An empty identifier switches the panel back to its placeholder page.
   */
  void slot_load_module_details(Module::ModuleIdentifier);

 private:
  QSharedPointer<Ui_ModuleControllerDialog> ui_;  ///<
  Module::ModuleManager* module_manager_;

  /**
   * @brief Translate all static texts of the dialog.
   */
  void init_texts();

  /**
   * @brief Connect all widget signals.
   */
  void init_connections();

  /**
   * @brief Show a notice when the module loading policy limits what is loaded.
   */
  void update_policy_notice();

  /**
   * @brief Reload the list and the currently displayed module's details.
   */
  void refresh_all();

  /**
   * @brief Render a colored chip into the given label.
   */
  void set_chip(QLabel* label, const QString& text, const QColor& color);
};

}  // namespace GpgFrontend::UI
