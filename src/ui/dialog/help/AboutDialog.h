/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief Class containing the main tab of about dialog
 *
 */
class InfoTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Info Tab object
   *
   * @param parent
   */
  explicit InfoTab(QWidget* parent = nullptr);
};

/**
 * @brief Class containing the translator tab of about dialog
 *
 */
class TranslatorsTab : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Translators Tab object
   *
   * @param parent
   */
  explicit TranslatorsTab(QWidget* parent = nullptr);
};

/**
 * @brief Class containing the main tab of about dialog
 *
 */
class UpdateTab : public QWidget {
  Q_OBJECT

  QLabel* current_version_label_;  ///<
  QLabel* latest_version_label_;   ///<
  QLabel* upgrade_label_;          ///<
  QProgressBar* pb_;               ///<
  QString current_version_;        ///<

 public:
  /**
   * @brief Construct a new Update Tab object
   *
   * @param parent
   */
  explicit UpdateTab(QWidget* parent = nullptr);

 protected:
  void showEvent(QShowEvent* event) override;

 private slots:
  /**
   * @brief
   *
   * @param version
   */
  void slot_show_version_status();

 signals:
  /**
   * @brief
   *
   * @param data
   */
  void SignalReplyFromUpdateServer(QByteArray data);
};

/**
 * @brief Class for handling the about dialog
 *
 */
class AboutDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new About Dialog object
   *
   * @param defaultIndex
   * @param parent
   */
  explicit AboutDialog(const QString& default_tab_name, QWidget* parent);

 protected:
  /**
   * @brief
   *
   * @param ev
   */
  void showEvent(QShowEvent* ev) override;
};

}  // namespace GpgFrontend::UI
