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

#ifndef WIZARD_H
#define WIZARD_H

#include "core/GpgConstants.h"
#include "main_window/KeyMgmt.h"
#include "ui/GpgFrontendUI.h"
#include "ui/key_generate/KeygenDialog.h"
#include "ui/settings/SettingsDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class Wizard : public QWizard {
  Q_OBJECT
  Q_ENUMS(WizardPages)

 public:
  enum WizardPages { Page_Intro, Page_Choose, Page_GenKey, Page_Conclusion };

  /**
   * @brief Construct a new Wizard object
   *
   * @param parent
   */
  explicit Wizard(QWidget* parent = nullptr);

 private slots:
  /**
   * @brief
   *
   */
  void slot_wizard_accepted();

 signals:
  /**
   * @brief
   *
   * @param page
   */
  void SignalOpenHelp(QString page);
};

/**
 * @brief
 *
 */
class IntroPage : public QWizardPage {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Intro Page object
   *
   * @param parent
   */
  explicit IntroPage(QWidget* parent = nullptr);

 protected:
  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] int nextId() const override;
};

class ChoosePage : public QWizardPage {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Choose Page object
   *
   * @param parent
   */
  explicit ChoosePage(QWidget* parent = nullptr);

 private slots:

  /**
   * @brief
   *
   * @param page
   */
  void slot_jump_page(const QString& page);

 protected:
  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] int nextId() const override;

  int next_page_;  ///<
};

/**
 * @brief
 *
 */
class KeyGenPage : public QWizardPage {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Gen Page object
   *
   * @param parent
   */
  explicit KeyGenPage(QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] int nextId() const override;

 private slots:

  /**
   * @brief
   *
   */
  void slot_generate_key_dialog();
};

/**
 * @brief
 *
 */
class ConclusionPage : public QWizardPage {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Conclusion Page object
   *
   * @param parent
   */
  explicit ConclusionPage(QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] int nextId() const override;

 private:
  QCheckBox* dont_show_wizard_checkbox_;  ///<
  QCheckBox* open_help_check_box_;        ///<
};

}  // namespace GpgFrontend::UI

#endif
