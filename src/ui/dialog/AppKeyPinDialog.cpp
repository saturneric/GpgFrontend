/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "AppKeyPinDialog.h"

#include "ui/function/UIStyle.h"
#include "ui/widgets/MetaListPanel.h"
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

AppKeyPinDialog::AppKeyPinDialog(Mode mode, QWidget* parent)
    : AppKeyPinDialog(
          mode, DefaultSecretPromptTexts(SecretPromptSubject::kAppKey, mode),
          parent) {}

AppKeyPinDialog::AppKeyPinDialog(Mode mode, SecretPromptTexts texts,
                                 QWidget* parent)
    : QDialog(parent), mode_(mode), texts_(std::move(texts)) {
  init_ui();
}

void AppKeyPinDialog::init_ui() {
  const bool asks_for_new = mode_ != Mode::kUNLOCK;

  setWindowTitle(texts_.window_title);
  setModal(true);
  setMinimumWidth(470);

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(24, 22, 24, 18);
  main_layout->setSpacing(16);

  // The badge, heading, explanation and rule every serious dialog opens with.
  main_layout->addLayout(CreateDialogHeader(QStringLiteral(":/icons/lock.png"),
                                            texts_.window_title,
                                            texts_.subtitle, this));

  // Which file this is about, when it is about one. Named before the field, so
  // nobody types a passphrase at a prompt they have not identified -- and named
  // in the same rows, in the same order, as every other dialog that names a
  // profile file.
  if (!texts_.context_rows.isEmpty()) {
    auto* context = new MetaListPanel(this);
    context->SetRows(texts_.context_rows);
    main_layout->addWidget(context);
  }

  SecretEntryPanel::Config config;
  config.ask_current = mode_ != Mode::kSET;
  config.ask_new = asks_for_new;
  config.texts = texts_;
  entry_ = new SecretEntryPanel(config, this);
  main_layout->addWidget(entry_);

  if (!texts_.warning.isEmpty()) {
    // The one thing a user must understand before choosing a secret: there is
    // no recovery path, because the thing is encrypted with it and nothing
    // else. A native warning glyph carries the alarm so the text needn't shout.
    auto* warning_icon = new QLabel(this);
    warning_icon->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(18, 18));
    warning_icon->setFixedWidth(18);
    warning_icon->setAlignment(Qt::AlignTop);

    auto* warning_label = new QLabel(texts_.warning, this);
    warning_label->setWordWrap(true);
    SetLabelTextColor(warning_label, DangerColor(palette()));

    auto* warning_row = new QHBoxLayout();
    warning_row->setSpacing(8);
    warning_row->addWidget(warning_icon, 0, Qt::AlignTop);
    warning_row->addWidget(warning_label, 1);
    main_layout->addLayout(warning_row);
  }

  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  accept_button_ = buttons->button(QDialogButtonBox::Ok);
  accept_button_->setDefault(true);
  accept_button_->setText(texts_.accept_button);

  if (!texts_.cancel_button.isEmpty()) {
    if (auto* cancel = buttons->button(QDialogButtonBox::Cancel);
        cancel != nullptr) {
      cancel->setText(texts_.cancel_button);
    }
  }

  // The escape hatch for a forgotten PIN, and only for the application key:
  // there is nothing this application could reset about somebody else's file.
  // Kept hidden until the caller decides the user is genuinely stuck
  // (RevealResetOption), so it never tempts anyone who merely mistyped once.
  // ResetRole lets the platform style seat it apart from Unlock/Quit — on the
  // left of the row on most styles. It only signals intent by closing with
  // kResetRequested; the caller confirms and resets.
  if (mode_ == Mode::kUNLOCK && texts_.context_rows.isEmpty()) {
    reset_button_ = buttons->addButton(tr("Forgot PIN? Reset…"),
                                       QDialogButtonBox::ResetRole);
    reset_button_->setVisible(false);
    // Colour the text as destructive through the palette, matching the warning
    // label and keeping clear of non-native QSS.
    auto reset_palette = reset_button_->palette();
    reset_palette.setColor(QPalette::ButtonText, DangerColor(palette()));
    reset_button_->setPalette(reset_palette);
    connect(reset_button_, &QPushButton::clicked, this,
            [this]() { done(kResetRequested); });
  }

  main_layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(entry_, &SecretEntryPanel::SignalStateChanged, this,
          [this]() { accept_button_->setEnabled(entry_->Acceptable()); });
  accept_button_->setEnabled(entry_->Acceptable());

  entry_->FocusFirstField();
  adjustSize();
}

auto AppKeyPinDialog::AskPackagePassphrase(QWidget* parent, Mode mode,
                                           bool retry,
                                           QVector<MetaListRow> context_rows)
    -> std::optional<GFBuffer> {
  auto texts =
      DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage, mode);
  texts.context_rows = std::move(context_rows);

  AppKeyPinDialog dialog(mode, texts, parent);
  if (retry) {
    dialog.SetErrorText(
        QObject::tr("That passphrase did not open this file. Try again."));
  }

  if (dialog.exec() != QDialog::Accepted) return {};

  auto secret = dialog.Pin();
  if (secret.Empty()) return {};
  return secret;
}

auto AppKeyPinDialog::Pin() const -> GFBuffer {
  return entry_ != nullptr ? entry_->Secret() : GFBuffer();
}

auto AppKeyPinDialog::CurrentPin() const -> GFBuffer {
  if (mode_ != Mode::kCHANGE || entry_ == nullptr) return {};
  return entry_->CurrentSecret();
}

void AppKeyPinDialog::SetErrorText(const QString& text) {
  if (entry_ != nullptr) entry_->SetErrorText(text);
}

void AppKeyPinDialog::Clear() {
  if (entry_ != nullptr) entry_->Clear();
}

void AppKeyPinDialog::RevealResetOption() {
  if (reset_button_ != nullptr) reset_button_->setVisible(true);
}

void AppKeyPinDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);

  // Prefer the screen the cursor is on, so on a multi-monitor setup the prompt
  // appears where the user is looking; fall back to this widget's screen and
  // then the primary one.
  const auto* screen = QGuiApplication::screenAt(QCursor::pos());
  if (screen == nullptr) screen = this->screen();
  if (screen == nullptr) screen = QGuiApplication::primaryScreen();
  if (screen == nullptr) return;

  auto geometry = frameGeometry();
  geometry.moveCenter(screen->availableGeometry().center());
  move(geometry.topLeft());
}

}  // namespace GpgFrontend::UI
