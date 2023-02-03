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

#include "WaitingDialog.h"

namespace GpgFrontend::UI {

WaitingDialog::WaitingDialog(const QString& title, QWidget* parent)
    : QDialog(parent) {
  auto* pb = new QProgressBar();
  pb->setRange(0, 0);
  pb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  pb->setTextVisible(false);

  auto* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(pb);
  this->setLayout(layout);

  this->setModal(true);
  this->raise();
  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);
  this->setWindowTitle(title);
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setFixedSize(240, 42);

  if (parentWidget() == nullptr) {
    auto* screen = QGuiApplication::primaryScreen();
    QRect geo = screen->availableGeometry();
    int screen_width = geo.width();
    int screen_height = geo.height();

    SPDLOG_INFO("primary screen available geometry: {} {}", screen_width,
                screen_height);

    auto pos = QPoint((screen_width - QWidget::width()) / 2,
                      (screen_height - QWidget::height()) / 2);
    this->move(pos);

  } else {
    auto pos = QPoint(parent->x() + (parent->width() - QWidget::width()) / 2,
                      parent->y() + (parent->height() - QWidget::height()) / 2);
    SPDLOG_INFO("pos: {} {}", pos.x(), pos.y());
    this->move(pos);
  }

  this->show();
}

}  // namespace GpgFrontend::UI
