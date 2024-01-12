/* pinentrydialog.cpp - A (not yet) secure Qt 4 dialog for PIN entry.
 * Copyright (C) 2002, 2008 Klarälvdalens Datakonsult AB (KDAB)
 * Copyright 2007 Ingo Klöcker
 * Copyright 2016 Intevation GmbH
 * Copyright (C) 2021, 2022 g10 Code GmbH
 *
 * Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 * Modified by Andre Heinecke <aheinecke@intevation.de>
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

#include "pinentrydialog.h"

#include <qnamespace.h>

#include <QAccessible>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>
#include <QVBoxLayout>

#include "accessibility.h"
#include "capslock/capslock.h"
#include "core/utils/MemoryUtils.h"
#include "pinentry.h"
#include "pinlineedit.h"
#include "util.h"

#ifdef Q_OS_WIN
#include <windows.h>
#if QT_VERSION >= 0x050700
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif
#endif

void raiseWindow(QWidget *w) {
#ifdef Q_OS_WIN
#if QT_VERSION >= 0x050700
  QWindowsWindowFunctions::setWindowActivationBehavior(
      QWindowsWindowFunctions::AlwaysActivateWindow);
#endif
#endif
  w->setWindowState((w->windowState() & ~Qt::WindowMinimized) |
                    Qt::WindowActive);
  w->activateWindow();
  w->raise();
}

QPixmap applicationIconPixmap(const QIcon &overlayIcon) {
  QPixmap pm = qApp->windowIcon().pixmap(48, 48);

  if (!overlayIcon.isNull()) {
    QPainter painter(&pm);
    const int emblemSize = 22;
    painter.drawPixmap(pm.width() - emblemSize, 0,
                       overlayIcon.pixmap(emblemSize, emblemSize));
  }

  return pm;
}

void PinEntryDialog::slotTimeout() {
  _timed_out = true;
  reject();
}

PinEntryDialog::PinEntryDialog(QWidget *parent, const char *name, int timeout,
                               bool modal, bool enable_quality_bar,
                               const QString &repeatString,
                               const QString &visibilityTT,
                               const QString &hideTT)
    : QDialog{parent},
      _have_quality_bar{enable_quality_bar},
      mVisibilityTT{visibilityTT},
      mHideTT{hideTT} {
  Q_UNUSED(name)

  if (modal) {
    setWindowModality(Qt::ApplicationModal);
    setModal(true);
  }

  QPalette red_text_palette;
  red_text_palette.setColor(QPalette::WindowText, Qt::red);

  auto *const main_layout = new QVBoxLayout{this};

  auto *const hbox = new QHBoxLayout;

  _icon = new QLabel(this);
  _icon->setPixmap(applicationIconPixmap());
  hbox->addWidget(_icon, 0, Qt::AlignVCenter | Qt::AlignLeft);

  auto *const grid = new QGridLayout;
  int row = 1;

  _error = new QLabel{this};
  _error->setTextFormat(Qt::PlainText);
  _error->setTextInteractionFlags(Qt::TextSelectableByMouse);
  _error->setPalette(red_text_palette);
  _error->hide();
  grid->addWidget(_error, row, 1, 1, 2);

  row++;
  _desc = new QLabel{this};
  _desc->setTextFormat(Qt::PlainText);
  _desc->setTextInteractionFlags(Qt::TextSelectableByMouse);
  _desc->hide();
  grid->addWidget(_desc, row, 1, 1, 2);

  row++;
  mCapsLockHint = new QLabel{this};
  mCapsLockHint->setTextFormat(Qt::PlainText);
  mCapsLockHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
  mCapsLockHint->setPalette(red_text_palette);
  mCapsLockHint->setAlignment(Qt::AlignCenter);
  mCapsLockHint->setVisible(false);
  grid->addWidget(mCapsLockHint, row, 1, 1, 2);

  row++;
  {
    _prompt = new QLabel(this);
    _prompt->setTextFormat(Qt::PlainText);
    _prompt->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _prompt->hide();
    grid->addWidget(_prompt, row, 1);

    auto *const l = new QHBoxLayout;
    _edit = new PinLineEdit(this);
    _edit->setMaxLength(256);
    _edit->setMinimumWidth(_edit->fontMetrics().averageCharWidth() * 20 + 48);
    _edit->setEchoMode(QLineEdit::Password);
    _prompt->setBuddy(_edit);
    l->addWidget(_edit, 1);

    if (!repeatString.isNull()) {
      mGenerateButton = new QPushButton{this};
      mGenerateButton->setIcon(QIcon(QLatin1String(":password-generate.svg")));
      mGenerateButton->setVisible(false);
      l->addWidget(mGenerateButton);
    }
    grid->addLayout(l, row, 2);
  }

  /* Set up the show password action */
  const QIcon visibility_icon = QIcon(QLatin1String(":visibility.svg"));
  const QIcon hide_icon = QIcon(QLatin1String(":hint.svg"));
#if QT_VERSION >= 0x050200
  if (!visibility_icon.isNull() && !hide_icon.isNull()) {
    mVisiActionEdit =
        _edit->addAction(visibility_icon, QLineEdit::TrailingPosition);
    mVisiActionEdit->setVisible(false);
    mVisiActionEdit->setToolTip(mVisibilityTT);
  } else
#endif
  {
    if (!mVisibilityTT.isNull()) {
      row++;
      mVisiCB = new QCheckBox{mVisibilityTT, this};
      grid->addWidget(mVisiCB, row, 1, 1, 2, Qt::AlignLeft);
    }
  }

  row++;
  mConstraintsHint = new QLabel{this};
  mConstraintsHint->setTextFormat(Qt::PlainText);
  mConstraintsHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
  mConstraintsHint->setVisible(false);
  grid->addWidget(mConstraintsHint, row, 2);

  row++;
  mFormattedPassphraseHintSpacer = new QLabel{this};
  mFormattedPassphraseHintSpacer->setVisible(false);
  mFormattedPassphraseHint = new QLabel{this};
  mFormattedPassphraseHint->setTextFormat(Qt::PlainText);
  mFormattedPassphraseHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
  mFormattedPassphraseHint->setVisible(false);
  grid->addWidget(mFormattedPassphraseHintSpacer, row, 1);
  grid->addWidget(mFormattedPassphraseHint, row, 2);

  if (!repeatString.isNull()) {
    row++;
    auto *repeat_label = new QLabel{this};
    repeat_label->setTextFormat(Qt::PlainText);
    repeat_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    repeat_label->setText(repeatString);
    grid->addWidget(repeat_label, row, 1);

    mRepeat = new PinLineEdit(this);
    mRepeat->setMaxLength(256);
    mRepeat->setEchoMode(QLineEdit::Password);
    repeat_label->setBuddy(mRepeat);
    grid->addWidget(mRepeat, row, 2);

    row++;
    mRepeatError = new QLabel{this};
    mRepeatError->setTextFormat(Qt::PlainText);
    mRepeatError->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mRepeatError->setPalette(red_text_palette);
    mRepeatError->hide();
    grid->addWidget(mRepeatError, row, 2);
  }

  if (enable_quality_bar) {
    row++;
    _quality_bar_label = new QLabel(this);
    _quality_bar_label->setTextFormat(Qt::PlainText);
    _quality_bar_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _quality_bar_label->setAlignment(Qt::AlignVCenter);
    grid->addWidget(_quality_bar_label, row, 1);

    _quality_bar = new QProgressBar(this);
    _quality_bar->setAlignment(Qt::AlignCenter);
    _quality_bar_label->setBuddy(_quality_bar);
    grid->addWidget(_quality_bar, row, 2);
  }

  hbox->addLayout(grid, 1);
  main_layout->addLayout(hbox);

  auto *const buttons = new QDialogButtonBox(this);
  buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  _ok = buttons->button(QDialogButtonBox::Ok);
  _cancel = buttons->button(QDialogButtonBox::Cancel);

  if (style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
    _ok->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
    _cancel->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
  }

  main_layout->addStretch(1);
  main_layout->addWidget(buttons);
  main_layout->setSizeConstraint(QLayout::SetFixedSize);

  if (timeout > 0) {
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &PinEntryDialog::slotTimeout);
    _timer->start(timeout * 1000);
  }

  connect(buttons, &QDialogButtonBox::accepted, this,
          &PinEntryDialog::onAccept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(_edit, &QLineEdit::textChanged, this, &PinEntryDialog::updateQuality);
  connect(_edit, &QLineEdit::textChanged, this, &PinEntryDialog::textChanged);
  connect(_edit, &PinLineEdit::backspacePressed, this,
          &PinEntryDialog::onBackspace);
  if (mGenerateButton != nullptr) {
    connect(mGenerateButton, &QPushButton::clicked, this,
            &PinEntryDialog::generatePin);
  }
  if (mVisiActionEdit != nullptr) {
    connect(mVisiActionEdit, &QAction::triggered, this,
            &PinEntryDialog::toggleVisibility);
  }
  if (mVisiCB != nullptr) {
    connect(mVisiCB, &QCheckBox::toggled, this,
            &PinEntryDialog::toggleVisibility);
  }
  if (mRepeat != nullptr) {
    connect(mRepeat, &QLineEdit::textChanged, this,
            &PinEntryDialog::textChanged);
  }

  auto *caps_lock_watcher = new CapsLockWatcher{this};
  connect(caps_lock_watcher, &CapsLockWatcher::stateChanged, this,
          [this](bool locked) { mCapsLockHint->setVisible(locked); });

  connect(qApp, &QApplication::focusChanged, this,
          &PinEntryDialog::focusChanged);
  connect(qApp, &QApplication::applicationStateChanged, this,
          &PinEntryDialog::checkCapsLock);
  checkCapsLock();

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

  /* This is mostly an issue on Windows where this results
     in the pinentry popping up nicely with an animation and
     comes to front. It is not ifdefed for Windows only since
     window managers on Linux like KWin can also have this
     result in an animation when the pinentry is shown and
     not just popping it up.
  */
  if (qApp->platformName() != QLatin1String("wayland")) {
    setWindowState(Qt::WindowMinimized);
    QTimer::singleShot(0, this, [this]() { raiseWindow(this); });
  } else {
    raiseWindow(this);
  }
}

void PinEntryDialog::keyPressEvent(QKeyEvent *e) {
  const auto return_pressed =
      (!e->modifiers() &&
       (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) ||
      (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter);
  if (return_pressed && _edit->hasFocus() && (mRepeat != nullptr)) {
    // if the user pressed Return in the first input field, then move the
    // focus to the repeat input field and prevent further event processing
    // by QDialog (which would trigger the default button)
    mRepeat->setFocus();
    e->ignore();
    return;
  }

  QDialog::keyPressEvent(e);
}

void PinEntryDialog::keyReleaseEvent(QKeyEvent *event) {
  QDialog::keyReleaseEvent(event);
  checkCapsLock();
}

void PinEntryDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  _edit->setFocus();
}

void PinEntryDialog::setDescription(const QString &txt) {
  _desc->setVisible(!txt.isEmpty());
  _desc->setText(txt);
  _icon->setPixmap(applicationIconPixmap());
  setError(QString());
}

QString PinEntryDialog::description() const { return _desc->text(); }

void PinEntryDialog::setError(const QString &txt) {
  if (!txt.isNull()) {
    _icon->setPixmap(
        applicationIconPixmap(QIcon{QStringLiteral(":data-error.svg")}));
  }
  _error->setText(txt);
  _error->setVisible(!txt.isEmpty());
}

QString PinEntryDialog::error() const { return _error->text(); }

void PinEntryDialog::setPin(const QString &txt) { _edit->setPin(txt); }

QString PinEntryDialog::pin() const { return _edit->pin(); }

void PinEntryDialog::setPrompt(const QString &txt) {
  _prompt->setText(txt);
  _prompt->setVisible(!txt.isEmpty());
  if (txt.contains("PIN")) _disable_echo_allowed = false;
}

QString PinEntryDialog::prompt() const { return _prompt->text(); }

void PinEntryDialog::setOkText(const QString &txt) {
  _ok->setText(txt);
  _ok->setVisible(!txt.isEmpty());
}

void PinEntryDialog::setCancelText(const QString &txt) {
  _cancel->setText(txt);
  _cancel->setVisible(!txt.isEmpty());
}

void PinEntryDialog::setQualityBar(const QString &txt) {
  if (_have_quality_bar) {
    _quality_bar_label->setText(txt);
  }
}

void PinEntryDialog::setQualityBarTT(const QString &txt) {
  if (_have_quality_bar) {
    _quality_bar->setToolTip(txt);
  }
}

void PinEntryDialog::setGenpinLabel(const QString &txt) {
  if (mGenerateButton == nullptr) {
    return;
  }
  mGenerateButton->setVisible(!txt.isEmpty());
  if (!txt.isEmpty()) {
    Accessibility::setName(mGenerateButton, txt);
  }
}

void PinEntryDialog::setGenpinTT(const QString &txt) {
  if (mGenerateButton != nullptr) {
    mGenerateButton->setToolTip(txt);
  }
}

void PinEntryDialog::setCapsLockHint(const QString &txt) {
  mCapsLockHint->setText(txt);
}

void PinEntryDialog::setFormattedPassphrase(
    const PinEntryDialog::FormattedPassphraseOptions &options) {
  mFormatPassphrase = options.formatPassphrase;
  mFormattedPassphraseHint->setTextFormat(Qt::RichText);
  mFormattedPassphraseHint->setText(QLatin1String("<html>") +
                                    options.hint.toHtmlEscaped() +
                                    QLatin1String("</html>"));
  Accessibility::setName(mFormattedPassphraseHint, options.hint);
  // toggleFormattedPassphrase();
}

void PinEntryDialog::setConstraintsOptions(const ConstraintsOptions &options) {
  mEnforceConstraints = options.enforce;
  mConstraintsHint->setText(options.shortHint);
  if (!options.longHint.isEmpty()) {
    mConstraintsHint->setToolTip(
        QLatin1String("<html>") +
        options.longHint.toHtmlEscaped().replace(QLatin1String("\n\n"),
                                                 QLatin1String("<br>")) +
        QLatin1String("</html>"));
    Accessibility::setDescription(mConstraintsHint, options.longHint);
  }
  mConstraintsErrorTitle = options.errorTitle;

  mConstraintsHint->setVisible(mEnforceConstraints &&
                               !options.shortHint.isEmpty());
}

void PinEntryDialog::toggleFormattedPassphrase() {
  const bool enable_formatting =
      mFormatPassphrase && _edit->echoMode() == QLineEdit::Normal;
  _edit->setFormattedPassphrase(enable_formatting);
  if (mRepeat != nullptr) {
    mRepeat->setFormattedPassphrase(enable_formatting);
    const bool hint_about_to_be_hidden =
        mFormattedPassphraseHint->isVisible() && !enable_formatting;
    if (hint_about_to_be_hidden) {
      // set hint spacer to current height of hint label before hiding the hint
      mFormattedPassphraseHintSpacer->setMinimumHeight(
          mFormattedPassphraseHint->height());
      mFormattedPassphraseHintSpacer->setVisible(true);
    } else if (enable_formatting) {
      mFormattedPassphraseHintSpacer->setVisible(false);
    }
    mFormattedPassphraseHint->setVisible(enable_formatting);
  }
}

void PinEntryDialog::onBackspace() {
  cancelTimeout();

  if (_disable_echo_allowed) {
    _edit->setEchoMode(QLineEdit::NoEcho);
    if (mRepeat != nullptr) {
      mRepeat->setEchoMode(QLineEdit::NoEcho);
    }
  }
}

void PinEntryDialog::updateQuality(const QString &txt) {
  int length;
  int percent;
  QPalette pal;

  _disable_echo_allowed = false;

  if (!_have_quality_bar) {
    return;
  }
  const QByteArray utf8_pin = txt.toUtf8();
  const char *pin = utf8_pin.constData();
  length = strlen(pin);
  percent = length != 0 ? pinentry_inq_quality(txt) : 0;
  if (length == 0) {
    _quality_bar->reset();
  } else {
    pal = _quality_bar->palette();
    if (percent < 0) {
      pal.setColor(QPalette::Highlight, QColor("red"));
      percent = -percent;
    } else {
      pal.setColor(QPalette::Highlight, QColor("green"));
    }
    _quality_bar->setPalette(pal);
    _quality_bar->setValue(percent);
  }
}

void PinEntryDialog::setPinentryInfo(struct pinentry peinfo) {
  _pinentry_info =
      GpgFrontend::SecureCreateUniqueObject<struct pinentry>(peinfo);
}

void PinEntryDialog::focusChanged(QWidget *old, QWidget *now) {
  // Grab keyboard. It might be a little weird to do it here, but it works!
  // Previously this code was in showEvent, but that did not work in Qt4.
  if (!_pinentry_info || (_pinentry_info->grab != 0)) {
    if (_grabbed && (old != nullptr) && (old == _edit || old == mRepeat)) {
      old->releaseKeyboard();
      _grabbed = false;
    }
    if (!_grabbed && (now != nullptr) && (now == _edit || now == mRepeat)) {
      now->grabKeyboard();
      _grabbed = true;
    }
  }
}

void PinEntryDialog::textChanged(const QString &text) {
  Q_UNUSED(text);

  cancelTimeout();

  if ((mVisiActionEdit != nullptr) && sender() == _edit) {
    mVisiActionEdit->setVisible(!_edit->pin().isEmpty());
  }
  if (mGenerateButton != nullptr) {
    mGenerateButton->setVisible(_edit->pin().isEmpty()
#ifndef QT_NO_ACCESSIBILITY
                                && !mGenerateButton->accessibleName().isEmpty()
#endif
    );
  }
}

void PinEntryDialog::generatePin() {
  unique_malloced_ptr<char> pin{pinentry_inq_genpin(_pinentry_info.get())};
  if (pin) {
    if (_edit->echoMode() == QLineEdit::Password) {
      if (mVisiActionEdit != nullptr) {
        mVisiActionEdit->trigger();
      }
      if (mVisiCB != nullptr) {
        mVisiCB->setChecked(true);
      }
    }
    const auto pin_str = QString::fromUtf8(pin.get());
    _edit->setPin(pin_str);
    mRepeat->setPin(pin_str);
    // explicitly focus the first input field and select the generated password
    _edit->setFocus();
    _edit->selectAll();
  }
}

void PinEntryDialog::toggleVisibility() {
  if (sender() != mVisiCB) {
    if (_edit->echoMode() == QLineEdit::Password) {
      if (mVisiActionEdit != nullptr) {
        mVisiActionEdit->setIcon(QIcon(QLatin1String(":hint.svg")));
        mVisiActionEdit->setToolTip(mHideTT);
      }
      _edit->setEchoMode(QLineEdit::Normal);
      if (mRepeat != nullptr) {
        mRepeat->setEchoMode(QLineEdit::Normal);
      }
    } else {
      if (mVisiActionEdit != nullptr) {
        mVisiActionEdit->setIcon(QIcon(QLatin1String(":visibility.svg")));
        mVisiActionEdit->setToolTip(mVisibilityTT);
      }
      _edit->setEchoMode(QLineEdit::Password);
      if (mRepeat != nullptr) {
        mRepeat->setEchoMode(QLineEdit::Password);
      }
    }
  } else {
    if (mVisiCB->isChecked()) {
      if (mRepeat != nullptr) {
        mRepeat->setEchoMode(QLineEdit::Normal);
      }
      _edit->setEchoMode(QLineEdit::Normal);
    } else {
      if (mRepeat != nullptr) {
        mRepeat->setEchoMode(QLineEdit::Password);
      }
      _edit->setEchoMode(QLineEdit::Password);
    }
  }
  toggleFormattedPassphrase();
}

QString PinEntryDialog::repeatedPin() const {
  if (mRepeat != nullptr) {
    return mRepeat->pin();
  }
  return QString();
}

bool PinEntryDialog::timedOut() const { return _timed_out; }

void PinEntryDialog::setRepeatErrorText(const QString &err) {
  if (mRepeatError != nullptr) {
    mRepeatError->setText(err);
  }
}

void PinEntryDialog::cancelTimeout() {
  if (_timer != nullptr) {
    _timer->stop();
  }
}

void PinEntryDialog::checkCapsLock() {
  const auto state = capsLockState();
  if (state != LockState::Unknown) {
    mCapsLockHint->setVisible(state == LockState::On);
  }
}

void PinEntryDialog::onAccept() {
  cancelTimeout();

  if ((mRepeat != nullptr) && mRepeat->pin() != _edit->pin()) {
#ifndef QT_NO_ACCESSIBILITY
    if (QAccessible::isActive()) {
      QMessageBox::information(this, mRepeatError->text(),
                               mRepeatError->text());
    } else
#endif
    {
      mRepeatError->setVisible(true);
    }
    return;
  }

  const auto result = checkConstraints();
  if (result != PassphraseNotOk) {
    accept();
  }
}

PinEntryDialog::PassphraseCheckResult PinEntryDialog::checkConstraints() {
  if (!mEnforceConstraints) {
    return PassphraseNotChecked;
  }

  const auto passphrase = _edit->pin().toUtf8();
  unique_malloced_ptr<char> error{pinentry_inq_checkpin(
      _pinentry_info.get(), passphrase.constData(), passphrase.size())};

  if (!error) {
    return PassphraseOk;
  }

  const auto message_lines =
      QString::fromUtf8(QByteArray::fromPercentEncoding(error.get()))
          .split(QChar{'\n'});
  if (message_lines.isEmpty()) {
    // shouldn't happen because pinentry_inq_checkpin() either returns NULL or a
    // non-empty string
    return PassphraseOk;
  }
  const auto &first_line = message_lines.first();
  const auto index_of_first_non_empty_additional_line =
      message_lines.indexOf(QRegularExpression{QStringLiteral(".*\\S.*")}, 1);
  const auto additional_lines =
      index_of_first_non_empty_additional_line > 0
          ? message_lines.mid(index_of_first_non_empty_additional_line)
                .join(QChar{'\n'})
          : QString{};
  QMessageBox message_box{this};
  message_box.setIcon(QMessageBox::Information);
  message_box.setWindowTitle(mConstraintsErrorTitle);
  message_box.setText(first_line);
  message_box.setInformativeText(additional_lines);
  message_box.setStandardButtons(QMessageBox::Ok);
  message_box.exec();
  return PassphraseNotOk;
}
