/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "TOFUInfoPage.h"

GpgFrontend::UI::TOFUInfoPage::TOFUInfoPage(
    const GpgFrontend::GpgTOFUInfo &tofu_info, QWidget *parent)
    : QWidget(parent) {
  auto grid_layout = new QGridLayout();

  grid_layout->addWidget(new QLabel(QString(_("Key ID")) + ": "), 0, 0);
  grid_layout->addWidget(new QLabel(QString(_("Algorithm")) + ": "), 1, 0);
  grid_layout->addWidget(new QLabel(QString(_("Key Size")) + ": "), 2, 0);
  grid_layout->addWidget(new QLabel(QString(_("Nominal Usage")) + ": "), 3, 0);
  grid_layout->addWidget(new QLabel(QString(_("Actual Usage")) + ": "), 4, 0);
  grid_layout->addWidget(new QLabel(QString(_("Expires on")) + ": "), 5, 0);
  grid_layout->addWidget(new QLabel(QString(_("Last Update")) + ": "), 6, 0);
  grid_layout->addWidget(new QLabel(QString(_("Secret Key Existence")) + ": "),
                         7, 0);

  setLayout(grid_layout);
}
