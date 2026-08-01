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

#include <functional>

#include "core/GFConstants.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief a wizard page whose text can be re-applied after a language change
 *
 * The wizard lets the user pick the interface language on its very first page
 * and switches the translators right away, so every page has to be able to
 * re-apply its own text while it is alive.
 */
class WizardPage : public QWizardPage {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Wizard Page object
   *
   * @param parent
   */
  explicit WizardPage(QWidget* parent = nullptr);

 protected:
  /**
   * @brief register a closure applying one piece of translated text
   *
   * The closure runs immediately, so the page is built and translated in one
   * step, and again on every QEvent::LanguageChange.
   *
   * @param retranslator
   */
  void add_retranslator(std::function<void()> retranslator);

  /**
   * @brief re-run every registered retranslator
   *
   */
  void retranslate_ui();

  /**
   * @brief
   *
   * @param event
   */
  void changeEvent(QEvent* event) override;

 private:
  QContainer<std::function<void()>> retranslators_;  ///<
};

class IntroPage;

/**
 * @brief
 *
 */
class Wizard : public QWizard {
  Q_OBJECT
  Q_ENUMS(WizardPages)

 public:
  enum WizardPages {
    kPAGE_INTRO,
    kPAGE_CHOOSE,
    kPAGE_GEN_KEY,
    kPAGE_CONCLUSION,
  };

  /**
   * @brief Construct a new Wizard object
   *
   * @param parent
   */
  explicit Wizard(QWidget* parent = nullptr);

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void changeEvent(QEvent* event) override;

 private slots:
  /**
   * @brief
   *
   */
  void slot_wizard_accepted();

  /**
   * @brief restore the language the wizard started with
   *
   */
  void slot_wizard_rejected();

 signals:
  /**
   * @brief
   *
   * @param page
   */
  void SignalOpenHelp(QString page);

  /**
   * @brief emitted when the choices made here only take effect after a restart
   *
   * @param mode kRestartCode or deeper
   */
  void SignalRestartNeeded(int mode);

 private:
  /**
   * @brief
   *
   */
  void retranslate_ui();

  IntroPage* intro_page_;  ///<
  QString initial_lang_;   ///< value of basic/lang when the wizard opened
};

/**
 * @brief
 *
 */
class IntroPage : public WizardPage {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Intro Page object
   *
   * @param parent
   */
  explicit IntroPage(QWidget* parent = nullptr);

  /**
   * @brief the locale key currently picked, empty for the system default
   *
   * @return QString
   */
  [[nodiscard]] auto SelectedLanguage() const -> QString;

 protected:
  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] int nextId() const override;

 private slots:
  /**
   * @brief apply the picked language to the running interface at once
   *
   * @param index
   */
  void slot_language_changed(int index);

 private:
  /**
   * @brief fill the language box, system default first, the rest sorted
   *
   */
  void populate_languages();

  QLabel* lang_label_;          ///<
  QComboBox* lang_select_box_;  ///<
  QString applied_lang_;        ///< locale key the translators are loaded from
};

class ChoosePage : public WizardPage {
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
class ConclusionPage : public WizardPage {
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
  [[nodiscard]] auto nextId() const -> int override;

 private:
  QCheckBox* dont_show_wizard_checkbox_;  ///<
  QCheckBox* check_updates_checkbox_;     ///<
  QCheckBox* open_help_check_box_;        ///<
};

}  // namespace GpgFrontend::UI
