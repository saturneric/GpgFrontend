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
 * @brief
 *
 */
class WaitingDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Waiting Dialog object
   *
   * @param title
   * @param parent
   */
  explicit WaitingDialog(const QString& title, bool range,
                         QWidget* parent = nullptr);

 public slots:

  /**
   * @brief max 100, min 0
   *
   */
  void SlotUpdateValue(int value);

 signals:

  /**
   * @brief
   *
   * @param value
   */
  void SignalUpdateValue(int value);

 protected:
  /**
   * @brief Center, raise and activate the dialog whenever it is presented.
   *
   */
  void showEvent(QShowEvent* event) override;

  /**
   * @brief Ignore Esc.
   *
   * The operations behind this dialog cannot be cancelled on this release
   * series. Letting the user dismiss it would end the wait while they are
   * still writing their output, and the caller would act on a partial result.
   * The dialog is closed by the caller once every operation reported back.
   *
   */
  void keyPressEvent(QKeyEvent* event) override;

  /**
   * @brief Ignore close requests coming from the user or the window system,
   * for the same reason as keyPressEvent(); programmatic closes still work.
   *
   */
  void closeEvent(QCloseEvent* event) override;

 private:
  QProgressBar* pb_;
};

}  // namespace GpgFrontend::UI
