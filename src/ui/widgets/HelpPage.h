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

#ifndef HELPPAGE_H
#define HELPPAGE_H

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class HelpPage : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Help Page object
   *
   * @param path
   * @param parent
   */
  explicit HelpPage(const QString& path, QWidget* parent = nullptr);

  /**
   * @brief Get the Browser object
   *
   * @return QTextBrowser*
   */
  QTextBrowser* GetBrowser();

 public slots:

  /**
   * @brief
   *
   * @param url
   */
  void slot_open_url(const QUrl& url);

 private:
  QTextBrowser* browser_;  ///< The textbrowser of the tab
  QUrl localized_help(const QUrl& path);
};

}  // namespace GpgFrontend::UI

#endif  // HELPPAGE_H
