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
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

AppKeyPinDialog::AppKeyPinDialog(Mode mode, QWidget* parent)
    : AppKeyPinDialog(
          mode,
          DefaultSecretPromptTexts(SecretPromptSubject::kAppKey, mode, {}),
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

  // Header: a lock badge next to the heading and its one-line explanation.
  // Gives the dialog a face so it reads as "a secure prompt", not "a form".
  auto* icon_label = new QLabel(this);
  auto pixmap =
      QPixmap(QStringLiteral(":/icons/lock.png"))
          .scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  icon_label->setPixmap(pixmap);
  icon_label->setFixedSize(40, 40);

  auto* title_label = new QLabel(texts_.window_title, this);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSizeF(title_font.pointSizeF() * 1.25);
  title_label->setFont(title_font);

  auto* subtitle_label = new QLabel(texts_.subtitle, this);
  subtitle_label->setWordWrap(true);
  SetLabelTextColor(subtitle_label, MutedTextColor(subtitle_label->palette()));

  // Icon and title share one line; the wrapped description spans the full
  // width beneath them. Keeping the subtitle out of the icon's column lets the
  // vertical layout honour its height-for-width, so no line ever clips — a
  // wrapped QLabel nested inside a horizontal layout does not get that.
  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(14);
  title_row->addWidget(icon_label, 0, Qt::AlignVCenter);
  title_row->addWidget(title_label, 1);

  auto* header_layout = new QVBoxLayout();
  header_layout->setSpacing(6);
  header_layout->addLayout(title_row);
  header_layout->addWidget(subtitle_label);
  main_layout->addLayout(header_layout);

  // A native rule sets the identity apart from the input area without the
  // weight of a boxed group; the platform style draws it to match its own
  // separators on every OS and theme.
  auto* separator = new QFrame(this);
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Sunken);
  main_layout->addWidget(separator);

  // Which file this is about, when it is about one. Named before the field, so
  // nobody types a passphrase at a prompt they have not identified.
  if (!texts_.context.isEmpty()) {
    auto* context_layout = new QVBoxLayout();
    context_layout->setSpacing(2);

    auto* caption = new QLabel(texts_.context_caption, this);
    SetLabelTextColor(caption, MutedTextColor(caption->palette()));
    context_layout->addWidget(caption);

    auto* value = new QLabel(texts_.context, this);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    context_layout->addWidget(value);

    if (!texts_.context_detail.isEmpty()) {
      auto* detail = new QLabel(texts_.context_detail, this);
      detail->setWordWrap(true);
      SetLabelTextColor(detail, MutedTextColor(detail->palette()));
      context_layout->addWidget(detail);
    }

    // What the file says about itself, kept visibly apart from what this
    // application knows. The header is outside the sealed payload, so anyone
    // holding the file can write anything here; showing it is only worth doing
    // alongside the sentence saying so.
    if (!texts_.context_note.isEmpty()) {
      auto* note = new QLabel(texts_.context_note, this);
      note->setWordWrap(true);
      note->setTextFormat(Qt::PlainText);
      SetLabelTextColor(note, MutedTextColor(note->palette()));
      context_layout->addSpacing(4);
      context_layout->addWidget(note);
    }

    main_layout->addLayout(context_layout);
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
  if (mode_ == Mode::kUNLOCK && texts_.context.isEmpty()) {
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

auto AppKeyPinDialog::AskPackagePassphrase(QWidget* parent,
                                           const QString& package_file,
                                           Mode mode, bool retry,
                                           const QString& context_note)
    -> std::optional<GFBuffer> {
  auto texts =
      DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage, mode,
                               QFileInfo(package_file).fileName());

  // Facts this application can establish for itself, as against the header's
  // claims below them: where the file actually is, how big it actually is, and
  // when it was actually last written.
  const QFileInfo info(package_file);
  QStringList detail;
  detail << QDir::toNativeSeparators(info.absolutePath());
  if (info.exists()) {
    detail << QLocale().formattedDataSize(info.size(), 1,
                                          QLocale::DataSizeTraditionalFormat);
    detail << QLocale().toString(info.lastModified(), QLocale::ShortFormat);
  }
  texts.context_detail = detail.join(" · ");
  texts.context_note = context_note;

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
