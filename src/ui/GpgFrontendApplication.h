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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/GpgFrontendUI.h"

#ifndef GPGFRONTEND_GPGFRONTENDAPPLICATION_H
#define GPGFRONTEND_GPGFRONTENDAPPLICATION_H

namespace GpgFrontend::UI {

class GPGFRONTEND_UI_EXPORT GpgFrontendApplication : public QApplication {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new GpgFrontend Application object
   *
   * @param argc
   * @param argv
   */
  explicit GpgFrontendApplication(int &argc, char *argv[]);

  /**
   * @brief Destroy the GpgFrontend Application object
   *
   */
  ~GpgFrontendApplication() override = default;

  /**
   * @brief Get the GpgFrontend Application object
   *
   * @return GpgFrontendApplication*
   */
  static GpgFrontendApplication *GetInstance(int argc = 0,
                                             char *argv[] = nullptr,
                                             bool new_instance = false);

 protected:
  /**
   * @brief
   *
   * @param event
   * @return bool
   */
  bool notify(QObject *receiver, QEvent *event) override;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_GPGFRONTENDAPPLICATION_H