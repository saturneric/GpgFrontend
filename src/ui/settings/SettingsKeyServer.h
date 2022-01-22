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

#ifndef GPGFRONTEND_SETTINGSKEYSERVER_H
#define GPGFRONTEND_SETTINGSKEYSERVER_H

#include "ui/GpgFrontendUI.h"

class Ui_KeyServerSettings;

namespace GpgFrontend::UI {
class KeyserverTab : public QWidget {
  Q_OBJECT

 public:
  explicit KeyserverTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private:
  std::shared_ptr<Ui_KeyServerSettings> ui;
  QString defaultKeyServer;
  QStringList keyServerStrList;
  QMenu* popupMenu{};

  QRegularExpression url_reg{
      R"(^https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)$)"};

 private slots:

  void addKeyServer();

  void refreshTable();

  void slotTestListedKeyServer();

 signals:

  void signalRestartNeeded(bool needed);

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSKEYSERVER_H
