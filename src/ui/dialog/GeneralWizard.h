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

namespace GpgFrontend::UI {

/**
 * @brief A wizard page whose gates are the wizard's to decide.
 *
 * The rules live in a pure summary beside each wizard, so a page only forwards
 * QWizard's two questions to it. Not a Q_OBJECT: it declares no signals or
 * slots of its own, and completeChanged() is QWizardPage's.
 */
class GF_UI_EXPORT GeneralWizardPage : public QWizardPage {
 public:
  using QWizardPage::QWizardPage;

  std::function<bool()> complete;  ///< enables Continue, or the final action
  std::function<bool()> validate;  ///< runs when it is pressed

  [[nodiscard]] auto isComplete() const -> bool override;

  auto validatePage() -> bool override;
};

/**
 * @brief The frame the step-by-step dialogs share.
 *
 * Export Profile and Export As Key Package are one family: the same style, the
 * same "Step N of M" header, the same button wording, the same remembered
 * size. Only the presentation is shared here; what each wizard exports and
 * when it may continue stays with the wizard.
 *
 * A QWizard rather than a GeneralDialog, so navigation and button order are the
 * platform's own. The geometry GeneralDialog keeps is kept here too, under the
 * same settings key shape, so a resized wizard opens at that size next time.
 */
class GF_UI_EXPORT GeneralWizard : public QWizard {
  Q_OBJECT

 public:
  /**
   * @brief Construct the frame.
   *
   * @param name the key the geometry is remembered under
   * @param finish_text what the last step's button does, never just "Finish"
   * @param parent parent widget
   */
  GeneralWizard(QString name, const QString& finish_text, QWidget* parent);

  /**
   * @brief Settle the size and position before the window is first mapped.
   *
   * A remembered geometry wins. Otherwise the wizard opens large enough for its
   * largest page, so going from one step to the next never resizes it.
   *
   * @param visible
   */
  void setVisible(bool visible) override;

  /**
   * @brief Remember the geometry however the wizard ends.
   *
   * @param result
   */
  void done(int result) override;

 protected:
  /**
   * @brief A page for a step, titled and registered under its index.
   *
   * @param index the step, from 0
   * @param count how many steps there are
   * @param title what the step is called
   * @return the page, owned by the wizard
   */
  auto AddStep(int index, int count, const QString& title)
      -> GeneralWizardPage*;

  /// @brief A page's body: no margins of its own, since the wizard frames it.
  static auto PageLayout(QWidget* page) -> QVBoxLayout*;

  /// @brief The one sentence a page opens with: what this step is for, before
  /// any control asks for anything.
  static void AddLead(QVBoxLayout* layout, const QString& text, QWidget* page);

 private:
  QString name_;
  bool first_show_handled_ = false;

  /// @brief Restore the remembered geometry.
  /// @return false when there was none to restore
  auto restore_geometry() -> bool;

  /// @brief Size for the largest page, and centre on the parent window.
  void place_first_time();

  /// @brief Once the pages have their real width, grow until the tallest one
  /// fits, and never allow less. Only ever grows: a size the user chose that
  /// already fits is kept.
  void fit_tallest_page();

  void save_geometry();
};

}  // namespace GpgFrontend::UI
