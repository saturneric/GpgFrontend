/* pinentryconfirm.cpp - A QMessageBox with a timeout
 *
 * Copyright (C) 2011 Ben Kibbey <bjk@luxsci.net>
 * Copyright (C) 2022 g10 Code GmbH
 *
 * Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "pinentryconfirm.h"

#include <QAbstractButton>
#include <QApplication>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QSpacerItem>

#include "accessibility.h"
#include "pinentrydialog.h"

namespace {
QLabel *messageBoxLabel(QMessageBox *messageBox) {
  return messageBox->findChild<QLabel *>(QStringLiteral("qt_msgbox_label"));
}
}  // namespace

PinentryConfirm::PinentryConfirm(Icon icon, const QString &title,
                                 const QString &text, StandardButtons buttons,
                                 QWidget *parent, Qt::WindowFlags flags)
    : QMessageBox{icon, title, text, buttons, parent, flags} {
  _timer.callOnTimeout(this, &PinentryConfirm::slotTimeout);

#ifndef QT_NO_ACCESSIBILITY
  QAccessible::installActivationObserver(this);
  accessibilityActiveChanged(QAccessible::isActive());
#endif

#if QT_VERSION >= 0x050000
  /* This is in line with PinentryDialog ctor to have a maximizing
   * animation when opening. */
  if (qApp->platformName() != QLatin1String("wayland")) {
    setWindowState(Qt::WindowMinimized);
    QTimer::singleShot(0, this, [this]() { raiseWindow(this); });
  }
#else
  activateWindow();
  raise();
#endif
}

PinentryConfirm::~PinentryConfirm() {
#ifndef QT_NO_ACCESSIBILITY
  QAccessible::removeActivationObserver(this);
#endif
}

void PinentryConfirm::setTimeout(std::chrono::seconds timeout) {
  _timer.setInterval(timeout);
}

std::chrono::seconds PinentryConfirm::timeout() const {
  return std::chrono::duration_cast<std::chrono::seconds>(
      _timer.intervalAsDuration());
}

bool PinentryConfirm::timedOut() const { return _timed_out; }

void PinentryConfirm::showEvent(QShowEvent *event) {
  static bool resized;
  if (!resized) {
    QGridLayout *lay = dynamic_cast<QGridLayout *>(layout());
    if (lay) {
      QSize textSize = fontMetrics().size(Qt::TextExpandTabs, text(),
                                          fontMetrics().maxWidth());
      QSpacerItem *horizontalSpacer =
          new QSpacerItem(textSize.width() + iconPixmap().width(), 0,
                          QSizePolicy::Minimum, QSizePolicy::Expanding);
      lay->addItem(horizontalSpacer, lay->rowCount(), 1, 1,
                   lay->columnCount() - 1);
    }
    resized = true;
  }

  QMessageBox::showEvent(event);

  if (timeout() > std::chrono::milliseconds::zero()) {
    _timer.setSingleShot(true);
    _timer.start();
  }
}

void PinentryConfirm::slotTimeout() {
  QAbstractButton *b = button(QMessageBox::Cancel);
  _timed_out = true;

  if (b) {
    b->animateClick();
  }
}

#ifndef QT_NO_ACCESSIBILITY
void PinentryConfirm::accessibilityActiveChanged(bool active) {
  // Allow text label to get focus if accessibility is active
  const auto focusPolicy = active ? Qt::StrongFocus : Qt::ClickFocus;
  if (auto label = messageBoxLabel(this)) {
    label->setFocusPolicy(focusPolicy);
  }
}
#endif
