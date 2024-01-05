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

#include "RaisePinentry.h"

#include <qwidget.h>

#include "core/function/CoreSignalStation.h"
#include "pinentry/pinentrydialog.h"

namespace GpgFrontend::UI {

auto FindTopMostWindow(QWidget* fallback) -> QWidget* {
  QList<QWidget*> top_widgets = QApplication::topLevelWidgets();
  foreach (QWidget* widget, top_widgets) {
    if (widget->isActiveWindow()) {
      GF_UI_LOG_TRACE("find a topmost widget, address: {}",
                      static_cast<void*>(widget));
      return widget;
    }
  }
  return fallback;
}

RaisePinentry::RaisePinentry(QWidget* parent) : QWidget(parent) {}

auto RaisePinentry::Exec() -> int {
  auto* pinentry =
      new PinEntryDialog(FindTopMostWindow(this), 0, 0, true, false, QString(),
                         QString::fromStdString(_("Show passphrase")),
                         QString::fromStdString(_("Hide passphrase")));

  GF_UI_LOG_DEBUG("setting pinetry's arguments");

  pinentry->setPrompt(QString::fromStdString(_("PIN:")));
  pinentry->setDescription(QString());
  pinentry->setRepeatErrorText(
      QString::fromStdString(_("Passphrases do not match")));
  pinentry->setGenpinLabel(QString());
  pinentry->setGenpinTT(QString());
  pinentry->setCapsLockHint(QString::fromStdString(_("Caps Lock is on")));
  pinentry->setFormattedPassphrase({false, QString()});
  pinentry->setConstraintsOptions({false, QString(), QString(), QString()});

  pinentry->setWindowTitle(_("Pinentry"));

  /* If we reuse the same dialog window.  */
  pinentry->setPin(QString());
  pinentry->setOkText(_("Confirm"));
  pinentry->setCancelText(_("Cancel"));

  GF_UI_LOG_DEBUG("pinentry is ready to start");

  connect(pinentry, &PinEntryDialog::finished, this, [pinentry](int result) {
    bool ret = result != 0;
    GF_UI_LOG_DEBUG("pinentry finished, ret: {}", ret);

    if (!ret) {
      emit CoreSignalStation::GetInstance()->SignalUserInputPassphraseCallback(
          {});
      return -1;
    }

    auto pin = pinentry->pin().toUtf8();
    emit CoreSignalStation::GetInstance()->SignalUserInputPassphraseCallback(
        pin);
    return 0;
  });
  connect(pinentry, &PinEntryDialog::finished, this, &QWidget::deleteLater);

  pinentry->open();
  return 0;
}
}  // namespace GpgFrontend::UI
