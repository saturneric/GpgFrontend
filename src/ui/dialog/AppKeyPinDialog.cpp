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

#include "ui/dialog/PassphraseStrength.h"

namespace GpgFrontend::UI {

namespace {

/// Shortest PIN accepted when setting one. The strength meter stays advice, but
/// a floor keeps a two-character PIN from being offered as real protection.
constexpr int kMinPinLength = 8;

/// Comfortable touch-sized height for the PIN fields; the platform default is
/// cramped for something the eye has to land on precisely.
constexpr int kFieldHeight = 34;

/// Semantic "irreversible danger" colour for the warning and error text. Kept
/// legible against both light and dark backgrounds, and applied through the
/// palette rather than a stylesheet so nothing depends on non-native QSS
/// rendering across platforms.
auto DangerColour() -> QColor { return {0xE5, 0x39, 0x35}; }

/// Recolour a label's text through its palette. Cross-platform and theme-safe,
/// with none of the boxed-panel artefacts a stylesheet would introduce.
void ColourLabel(QLabel* label, const QColor& colour) {
  auto palette = label->palette();
  palette.setColor(QPalette::WindowText, colour);
  label->setPalette(palette);
}

/// The window/heading title, phrased around what the mode does for the user.
auto TitleForMode(AppKeyPinDialog::Mode mode) -> QString {
  switch (mode) {
    case AppKeyPinDialog::Mode::kUNLOCK:
      return AppKeyPinDialog::tr("Unlock Application Key");
    case AppKeyPinDialog::Mode::kCHANGE:
      return AppKeyPinDialog::tr("Change Application PIN");
    case AppKeyPinDialog::Mode::kSET:
      return AppKeyPinDialog::tr("Set an Application PIN");
  }
  return {};
}

/// Dim a label to secondary emphasis without hard-coding a colour, so the
/// result stays legible in both light and dark themes. Derives from the
/// widget's own text colour and only lowers its alpha.
void DimLabel(QLabel* label, int alpha = 160) {
  auto palette = label->palette();
  auto colour = palette.color(QPalette::WindowText);
  colour.setAlpha(alpha);
  palette.setColor(QPalette::WindowText, colour);
  label->setPalette(palette);
}

}  // namespace

AppKeyPinDialog::AppKeyPinDialog(Mode mode, QWidget* parent)
    : QDialog(parent), mode_(mode) {
  const bool asks_for_new = mode_ != Mode::kUNLOCK;

  const auto title = TitleForMode(mode_);
  const auto subtitle =
      asks_for_new
          ? tr("This PIN encrypts the application key on disk. You will be "
               "asked for it every time the application starts.")
          : tr("This application's key is protected by a PIN. Enter it to "
               "continue.");

  setWindowTitle(title);
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

  auto* title_label = new QLabel(title, this);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSizeF(title_font.pointSizeF() * 1.25);
  title_label->setFont(title_font);

  auto* subtitle_label = new QLabel(subtitle, this);
  subtitle_label->setWordWrap(true);
  DimLabel(subtitle_label);

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

  auto* form_layout = new QFormLayout();
  form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form_layout->setHorizontalSpacing(12);
  form_layout->setVerticalSpacing(10);

  const auto make_field = [this]() -> QLineEdit* {
    auto* edit = new QLineEdit(this);
    edit->setEchoMode(QLineEdit::Password);
    edit->setMinimumHeight(kFieldHeight);
    edit->setClearButtonEnabled(true);
    return edit;
  };

  if (mode_ == Mode::kUNLOCK || mode_ == Mode::kCHANGE) {
    current_edit_ = make_field();
    form_layout->addRow(mode_ == Mode::kCHANGE ? tr("Current PIN") : tr("PIN"),
                        current_edit_);
  }

  if (asks_for_new) {
    new_edit_ = make_field();
    form_layout->addRow(mode_ == Mode::kCHANGE ? tr("New PIN") : tr("PIN"),
                        new_edit_);

    confirm_edit_ = make_field();
    form_layout->addRow(tr("Confirm"), confirm_edit_);
  }

  main_layout->addLayout(form_layout);

  // Reveal toggle, aligned under the fields and set apart as a quiet control
  // rather than another form row competing with the PIN labels.
  auto* show_box = new QCheckBox(tr("Show PIN"), this);
  auto* show_row = new QHBoxLayout();
  show_row->addStretch(1);
  show_row->addWidget(show_box);
  main_layout->addLayout(show_row);
  connect(show_box, &QCheckBox::toggled, this, [this](bool shown) {
    const auto echo = shown ? QLineEdit::Normal : QLineEdit::Password;
    for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
      if (edit != nullptr) edit->setEchoMode(echo);
    }
  });

  if (asks_for_new) {
    auto* strength_caption = new QLabel(tr("Strength"), this);
    DimLabel(strength_caption);
    strength_bar_ = new QProgressBar(this);
    strength_bar_->setRange(0, 100);
    strength_bar_->setTextVisible(false);
    strength_bar_->setFixedHeight(8);
    strength_label_ = new QLabel(this);

    auto* strength_layout = new QHBoxLayout();
    strength_layout->setSpacing(10);
    strength_layout->addWidget(strength_caption);
    strength_layout->addWidget(strength_bar_, 1);
    strength_layout->addWidget(strength_label_);
    main_layout->addLayout(strength_layout);

    // The one thing a user must understand before choosing a PIN: there is no
    // recovery path, because the key is encrypted with it and nothing else. A
    // native warning glyph carries the alarm so the text needn't shout.
    auto* warning_icon = new QLabel(this);
    warning_icon->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(18, 18));
    warning_icon->setFixedWidth(18);
    warning_icon->setAlignment(Qt::AlignTop);

    auto* warning_label = new QLabel(
        tr("If you forget this PIN, everything the application has encrypted "
           "becomes permanently unreadable. There is no recovery."),
        this);
    warning_label->setWordWrap(true);
    ColourLabel(warning_label, DangerColour());

    auto* warning_row = new QHBoxLayout();
    warning_row->setSpacing(8);
    warning_row->addWidget(warning_icon, 0, Qt::AlignTop);
    warning_row->addWidget(warning_label, 1);
    main_layout->addLayout(warning_row);
  }

  error_label_ = new QLabel(this);
  error_label_->setWordWrap(true);
  error_label_->setAlignment(Qt::AlignTop);
  // The message row does double duty: a dimmed hint when all is well, the retry
  // message in danger red on a failure. Keeping it always visible means the
  // reserved space carries guidance instead of reading as empty padding, and
  // the two-line floor keeps the fields and buttons from moving when a longer
  // message replaces a shorter one.
  error_label_->setMinimumHeight(error_label_->fontMetrics().lineSpacing() * 2);
  main_layout->addWidget(error_label_);

  // A short piece of guidance to sit in the message row while there is nothing
  // to correct, chosen per mode so the reserved space is never merely blank.
  hint_text_ = mode_ == Mode::kUNLOCK
                   ? tr("This PIN cannot be recovered if it is lost.")
                   : tr("Use at least %1 characters.").arg(kMinPinLength);
  show_default_hint();

  auto* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  accept_button_ = buttons->button(QDialogButtonBox::Ok);
  accept_button_->setDefault(true);
  accept_button_->setText(mode_ == Mode::kUNLOCK ? tr("Unlock") : tr("OK"));

  // Unlocking only ever happens at startup, where cancelling closes the
  // application rather than merely dismissing a dialog. Name the button after
  // what it does so the consequence is not a surprise.
  if (mode_ == Mode::kUNLOCK) {
    if (auto* cancel = buttons->button(QDialogButtonBox::Cancel);
        cancel != nullptr) {
      cancel->setText(tr("Quit"));
    }

    // The escape hatch for a forgotten PIN. Kept hidden until the caller
    // decides the user is genuinely stuck (RevealResetOption), so it never
    // tempts anyone who merely mistyped once. ResetRole lets the platform style
    // seat it apart from Unlock/Quit — on the left of the row on most styles.
    // It only signals intent by closing with kResetRequested; the caller
    // confirms and resets.
    reset_button_ = buttons->addButton(tr("Forgot PIN? Reset…"),
                                       QDialogButtonBox::ResetRole);
    reset_button_->setVisible(false);
    // Colour the text as destructive through the palette, matching the warning
    // label and keeping clear of non-native QSS.
    auto reset_palette = reset_button_->palette();
    reset_palette.setColor(QPalette::ButtonText, DangerColour());
    reset_button_->setPalette(reset_palette);
    connect(reset_button_, &QPushButton::clicked, this,
            [this]() { done(kResetRequested); });
  }

  main_layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit == nullptr) continue;
    connect(edit, &QLineEdit::textChanged, this, [this]() {
      // Any edit means the previous failure no longer describes what is typed,
      // so fall back to the hint; refresh_validity() re-raises an error if the
      // new input still has one.
      show_default_hint();
      refresh_strength();
      refresh_validity();
    });
  }

  refresh_strength();
  refresh_validity();

  if (current_edit_ != nullptr) {
    current_edit_->setFocus();
  } else if (new_edit_ != nullptr) {
    new_edit_->setFocus();
  }

  adjustSize();
}

auto AppKeyPinDialog::Pin() const -> GFBuffer {
  auto* source = mode_ == Mode::kUNLOCK ? current_edit_ : new_edit_;
  return source != nullptr ? GFBuffer(source->text()) : GFBuffer();
}

auto AppKeyPinDialog::CurrentPin() const -> GFBuffer {
  if (mode_ != Mode::kCHANGE || current_edit_ == nullptr) return {};
  return GFBuffer(current_edit_->text());
}

void AppKeyPinDialog::SetErrorText(const QString& text) {
  // An empty message is not "no message" but "back to guidance": the row is
  // always visible, so it falls back to the dimmed hint rather than a blank
  // gap.
  if (text.isEmpty()) {
    show_default_hint();
    return;
  }
  ColourLabel(error_label_, DangerColour());
  error_label_->setText(text);
  // No adjustSize(): the message row's height is reserved up front, so showing
  // an error must leave the rest of the dialog exactly where it was.
}

void AppKeyPinDialog::show_default_hint() {
  // Reset the colour from the dialog's own text, not the label's — the label
  // may still be carrying error red — then dim it so the hint reads as
  // secondary and stays legible in both light and dark themes.
  auto colour = palette().color(QPalette::WindowText);
  colour.setAlpha(160);
  ColourLabel(error_label_, colour);
  error_label_->setText(hint_text_);
}

void AppKeyPinDialog::Clear() {
  for (auto* edit : {current_edit_, new_edit_, confirm_edit_}) {
    if (edit != nullptr) edit->clear();
  }
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

void AppKeyPinDialog::refresh_validity() {
  if (accept_button_ == nullptr) return;

  if (mode_ == Mode::kUNLOCK) {
    accept_button_->setEnabled(!current_edit_->text().isEmpty());
    return;
  }

  const auto pin = new_edit_->text();
  const bool long_enough = pin.size() >= kMinPinLength;
  const bool confirmed = pin == confirm_edit_->text();
  const bool has_current =
      mode_ != Mode::kCHANGE || !current_edit_->text().isEmpty();

  accept_button_->setEnabled(long_enough && confirmed && has_current);

  // Say which of the two conditions is unmet, but only once something has been
  // typed — an empty form is not yet a mistake. When nothing is wrong the row
  // returns to its dimmed hint rather than going blank.
  if (!pin.isEmpty() && !long_enough) {
    SetErrorText(
        tr("The PIN must be at least %1 characters.").arg(kMinPinLength));
  } else if (long_enough && !confirm_edit_->text().isEmpty() && !confirmed) {
    SetErrorText(tr("The two PINs do not match."));
  } else {
    show_default_hint();
  }
}

void AppKeyPinDialog::refresh_strength() {
  if (strength_bar_ == nullptr || new_edit_ == nullptr) return;

  const auto strength = CalculatePassphraseStrength(new_edit_->text());
  const auto color = PassphraseStrengthColor(strength);

  strength_bar_->setValue(strength);
  strength_bar_->setStyleSheet(
      QStringLiteral(
          "QProgressBar { border: none; border-radius: 4px; "
          "background-color: rgba(128, 128, 128, 60); }"
          "QProgressBar::chunk { border-radius: 4px; background-color: %1; }")
          .arg(color));

  strength_label_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: 600;").arg(color));
  strength_label_->setText(PassphraseStrengthDescription(strength));
}

}  // namespace GpgFrontend::UI
