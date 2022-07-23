/*
 * Copyright (c) 2022. Saturneric
 *
 *  This file is part of GpgFrontend.
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
 */

//
// Created by eric on 2022/7/23.
//

#include "GnupgTab.h"

GpgFrontend::UI::GnupgTab::GnupgTab(QWidget* parent) : QWidget(parent) {
  GpgContext& ctx = GpgContext::GetInstance();
  auto info = ctx.GetInfo();

  auto* pixmap = new QPixmap(":gnupg.png");
  auto* text = new QString(
      "<center><h2>" + QString(_("GnuPG")) + "</h2></center>" + "<center><b>" +
      QString(_("GnuPG Version")) + ": " +
      QString::fromStdString(info.GnupgVersion) + "</b></center>" +
      "<center><b>" + +"</b></center>" + "<center>" +
      QString(_("GpgME Version")) + ": " +
      QString::fromStdString(info.GpgMEVersion) + "</center><br /><hr />" +
      "<h3>" + QString(_("PATHs")) + "</h3>" + QString(_("GpgConf")) + ": " +
      QString::fromStdString(info.GpgConfPath) + "<br />" +
      QString(_("GnuPG")) + ": " + QString::fromStdString(info.AppPath) +
      "<br />" + QString(_("CMS")) + ": " +
      QString::fromStdString(info.CMSPath) + "<br />");

  auto* layout = new QGridLayout();
  auto* pixmapLabel = new QLabel();
  pixmapLabel->setPixmap(*pixmap);
  layout->addWidget(pixmapLabel, 0, 0, 1, -1, Qt::AlignCenter);
  auto* aboutLabel = new QLabel();
  aboutLabel->setText(*text);
  aboutLabel->setWordWrap(true);
  aboutLabel->setOpenExternalLinks(true);
  layout->addWidget(aboutLabel, 1, 0, 1, -1);
  layout->addItem(
      new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 1,
      1, 1);

  setLayout(layout);
}
