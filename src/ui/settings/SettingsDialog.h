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

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include "ui/GpgFrontendUI.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

class GeneralTab;

#ifdef SMTP_SUPPORT
class SendMailTab;
#endif

class AppearanceTab;
class KeyserverTab;
class NetworkTab;

#ifdef ADVANCED_SUPPORT
class AdvancedTab;
#endif

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  GeneralTab* generalTab;
#ifdef SMTP_SUPPORT
  SendMailTab* sendMailTab;
#endif
  AppearanceTab* appearanceTab;
  KeyserverTab* keyserverTab;
  NetworkTab* networkTab;
#ifdef ADVANCED_SUPPORT
  AdvancedTab* advancedTab;
#endif

  static QHash<QString, QString> listLanguages();

 public slots:

  void slotAccept();

 signals:

  void signalRestartNeeded(bool needed);

 private:
  QTabWidget* tabWidget;
  QDialogButtonBox* buttonBox;
  bool restartNeeded{};

  bool getRestartNeeded() const;

 private slots:

  void slotSetRestartNeeded(bool needed);
};

}  // namespace GpgFrontend::UI

#endif  // __SETTINGSDIALOG_H__
