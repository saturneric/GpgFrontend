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

#include "ui/dialog/profile/ProfileExportDialog.h"

#include <QFileDialog>
#include <QStorageInfo>

#include "core/profile/ProfilePackage.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SecretPrompt.h"
#include "ui/function/ProfileController.h"
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

namespace {

auto HumanSize(qint64 bytes) -> QString {
  return QLocale().formattedDataSize(bytes, 1,
                                     QLocale::DataSizeTraditionalFormat);
}

}  // namespace

ProfileExportDialog::ProfileExportDialog(QString display_name,
                                         QString profile_root, QWidget* parent)
    : GeneralDialog("profile_export_dialog", parent),
      display_name_(std::move(display_name)),
      profile_root_(std::move(profile_root)),
      areas_(MeasureProfileAreas(profile_root_)) {
  init_ui();
  setWindowTitle(tr("Export Profile"));
  setModal(true);

  // GeneralDialog deletes itself on close, and everything this dialog exists
  // to say is read after exec() returns.
  setAttribute(Qt::WA_DeleteOnClose, false);

  movePosition2CenterOfParent();
}

void ProfileExportDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);

  // Word-wrapped labels ask for almost no width, so the dialog would open as a
  // tall ribbon; stating the width once makes everything wrap against it.
  setMinimumWidth(560);

  build_header(layout);

  auto* destination_row = new QHBoxLayout();
  destination_row->setSpacing(10);

  auto* destination_column = new QVBoxLayout();
  destination_column->setSpacing(2);
  destination_label_ = new QLabel(tr("— choose a file —"), this);
  SetLabelTextColor(destination_label_,
                    MutedTextColor(destination_label_->palette()));
  destination_detail_ = new QLabel(this);
  destination_detail_->setWordWrap(true);
  SetLabelTextColor(destination_detail_,
                    MutedTextColor(destination_detail_->palette()));
  destination_detail_->hide();
  destination_column->addWidget(destination_label_);
  destination_column->addWidget(destination_detail_);

  destination_button_ = new QPushButton(tr("Choose..."), this);
  destination_row->addWidget(new QLabel(tr("Save to"), this));
  destination_row->addLayout(destination_column, 1);
  destination_row->addWidget(destination_button_, 0, Qt::AlignTop);
  layout->addLayout(destination_row);

  build_contents(layout);
  build_protection(layout);

  // One footer, not two rows. The warning and the sentence saying what is about
  // to happen are both about the same moment, and giving each its own reserved
  // height left a block of blank space above the button whenever there was
  // neither — which at open is always. Reserving the height once, on the block
  // rather than on either label, still keeps the Export button from moving
  // under the cursor aiming at it, without ever showing more emptiness than a
  // single message would occupy.
  auto* footer = new QWidget(this);
  auto* footer_row = new QHBoxLayout(footer);
  footer_row->setContentsMargins(0, 0, 0, 0);
  footer_row->setSpacing(8);

  warning_icon_ = new QLabel(footer);
  warning_icon_->setFixedWidth(18);
  warning_icon_->setAlignment(Qt::AlignTop);

  warning_label_ = new QLabel(footer);
  warning_label_->setWordWrap(true);
  SetLabelTextColor(warning_label_, DangerColor(palette()));

  summary_label_ = new QLabel(footer);
  summary_label_->setWordWrap(true);
  SetLabelTextColor(summary_label_, MutedTextColor(summary_label_->palette()));

  auto* footer_column = new QVBoxLayout();
  footer_column->setSpacing(4);
  footer_column->addWidget(warning_label_);
  footer_column->addWidget(summary_label_);

  footer_row->addWidget(warning_icon_, 0, Qt::AlignTop);
  footer_row->addLayout(footer_column, 1);

  footer->setMinimumHeight(summary_label_->fontMetrics().lineSpacing() * 3);
  layout->addWidget(footer);

  buttons_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons_->button(QDialogButtonBox::Ok)->setText(tr("Export"));
  layout->addWidget(buttons_);

  connect(destination_button_, &QPushButton::clicked, this,
          &ProfileExportDialog::slot_choose_destination);
  connect(protect_with_pin_, &QRadioButton::toggled, this,
          &ProfileExportDialog::slot_state_changed);
  connect(workspace_box_, &QCheckBox::toggled, this,
          &ProfileExportDialog::slot_state_changed);
  connect(entry_, &SecretEntryPanel::SignalStateChanged, this,
          &ProfileExportDialog::slot_state_changed);
  connect(buttons_, &QDialogButtonBox::accepted, this,
          &ProfileExportDialog::accept);
  connect(buttons_, &QDialogButtonBox::rejected, this,
          &ProfileExportDialog::reject);

  slot_state_changed();
}

void ProfileExportDialog::build_header(QVBoxLayout* layout) {
  // The same badge, heading and rule the PIN prompt uses. Handing over a whole
  // profile deserves to look like the same class of act as unlocking one, and
  // sharing the furniture is what makes the two read as one family.
  auto* icon_label = new QLabel(this);
  icon_label->setPixmap(
      QPixmap(QStringLiteral(":/icons/lock.png"))
          .scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  icon_label->setFixedSize(40, 40);

  auto* title_label = new QLabel(tr("Export \"%1\"").arg(display_name_), this);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSizeF(title_font.pointSizeF() * 1.25);
  title_label->setFont(title_font);

  auto* subtitle_label = new QLabel(
      tr("Everything below is written into a single file you can copy to "
         "another computer and import there."),
      this);
  subtitle_label->setWordWrap(true);
  SetLabelTextColor(subtitle_label, MutedTextColor(subtitle_label->palette()));

  auto* title_row = new QHBoxLayout();
  title_row->setSpacing(14);
  title_row->addWidget(icon_label, 0, Qt::AlignVCenter);
  title_row->addWidget(title_label, 1);

  auto* header_layout = new QVBoxLayout();
  header_layout->setSpacing(6);
  header_layout->addLayout(title_row);
  header_layout->addWidget(subtitle_label);
  layout->addLayout(header_layout);

  auto* separator = new QFrame(this);
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Sunken);
  layout->addWidget(separator);
}

void ProfileExportDialog::build_contents(QVBoxLayout* layout) {
  auto* contents = new QWidget(this);
  auto* contents_layout = new QVBoxLayout(contents);
  contents_layout->setContentsMargins(0, 0, 0, 0);

  // A grid rather than a run of wrapped rich text, because the only reason to
  // print sizes is so they can be compared, and they cannot be compared until
  // they line up in a column.
  contents_grid_ = new QGridLayout();
  contents_grid_->setHorizontalSpacing(10);
  // Roomier than a form: these are five things to read and compare, not five
  // things to fill in, and a list packed to the line height reads as a wall.
  contents_grid_->setVerticalSpacing(9);
  contents_grid_->setColumnStretch(1, 1);

  const auto rows = BuildProfileExportContents(areas_, false);
  for (int i = 0; i < rows.size(); ++i) {
    const auto& row = rows.at(i);

    auto* icon = new QLabel(contents);
    icon->setPixmap(QPixmap(row.icon).scaled(16, 16, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
    icon->setFixedWidth(16);
    contents_grid_->addWidget(icon, i, 0);

    // The workspace row *is* its checkbox. A checkbox sitting below a list that
    // never mentions the workspace is how someone ends up unsure whether the
    // list they just read was the whole story.
    if (row.optional) {
      workspace_box_ = new QCheckBox(row.label, contents);
      // Off unless asked for. The workspace has no size limit and is precisely
      // where the cleartext of things meant to be encrypted ends up; nobody
      // should discover after the fact that their drafts travelled inside a
      // file they emailed to someone.
      workspace_box_->setChecked(false);
      workspace_box_->setEnabled(row.bytes > 0);
      contents_grid_->addWidget(workspace_box_, i, 1);
      row_widgets_.append(workspace_box_);
    } else {
      auto* name = new QLabel(row.label, contents);
      contents_grid_->addWidget(name, i, 1);
      row_widgets_.append(name);
    }

    auto* size = new QLabel(HumanSize(row.bytes), contents);
    size->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    contents_grid_->addWidget(size, i, 2);
    size_labels_.append(size);
  }

  auto* rule = new QFrame(contents);
  rule->setFrameShape(QFrame::HLine);
  rule->setFrameShadow(QFrame::Plain);
  auto rule_palette = rule->palette();
  rule_palette.setColor(QPalette::WindowText, BorderColor(rule->palette()));
  rule->setPalette(rule_palette);
  contents_grid_->addWidget(rule, rows.size(), 0, 1, 3);

  auto* total_caption = new QLabel(tr("Total"), contents);
  auto caption_font = total_caption->font();
  caption_font.setBold(true);
  total_caption->setFont(caption_font);
  contents_grid_->addWidget(total_caption, rows.size() + 1, 1);

  total_label_ = new QLabel(contents);
  total_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  auto total_font = total_label_->font();
  total_font.setBold(true);
  total_label_->setFont(total_font);
  contents_grid_->addWidget(total_label_, rows.size() + 1, 2);

  contents_layout->addLayout(contents_grid_);

  auto* caveat = new QLabel(
      tr("Logs and modules are never included. Keys kept outside this profile, "
         "such as the system GnuPG keyring, stay where they are."),
      contents);
  caveat->setWordWrap(true);
  SetLabelTextColor(caveat, MutedTextColor(caveat->palette()));
  contents_layout->addWidget(caveat);

  layout->addWidget(CreateCard(tr("What goes in"), contents, this));
}

void ProfileExportDialog::build_protection(QVBoxLayout* layout) {
  auto* protection = new QWidget(this);
  auto* protection_layout = new QVBoxLayout(protection);
  protection_layout->setContentsMargins(0, 0, 0, 0);
  protection_layout->setSpacing(8);

  protect_with_pin_ =
      new QRadioButton(tr("Protect with a passphrase"), protection);
  protect_with_pin_->setChecked(true);
  protection_layout->addWidget(protect_with_pin_);

  // The same fields, reveal toggle, strength meter and floor the application
  // PIN is chosen with. A generator is offered here and not there because this
  // secret is going to be written down rather than remembered.
  auto texts = DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage,
                                        SecretPromptMode::kSET, {});
  texts.hint = tr(
      "The file cannot be opened without this passphrase, and there is no way "
      "to recover it. This computer's keychain is not used: the file has to "
      "open on another computer.");

  SecretEntryPanel::Config config;
  config.ask_current = false;
  config.ask_new = true;
  config.offer_generation = true;
  config.texts = texts;
  entry_ = new SecretEntryPanel(config, protection);
  protection_layout->addWidget(entry_);

  // Named rather than asserted. Someone who knows what these are can check the
  // claim; someone who does not still reads a specific mechanism rather than a
  // reassurance. The Argon2id parameters are deliberately left out: they will
  // change, and a number nobody can evaluate is noise.
  auto* mechanism = new QLabel(
      tr("XChaCha20-Poly1305, with a key derived from your passphrase using "
         "Argon2id."),
      protection);
  mechanism->setWordWrap(true);
  SetLabelTextColor(mechanism, MutedTextColor(mechanism->palette()));
  protection_layout->addWidget(mechanism);

  protect_with_nothing_ = new QRadioButton(tr("No protection"), protection);
  protection_layout->addWidget(protect_with_nothing_);

  layout->addWidget(CreateCard(tr("Protection"), protection, this));
}

auto ProfileExportDialog::choice() const -> ProfileExportChoice {
  ProfileExportChoice choice;
  choice.has_destination = !destination_.isEmpty();
  choice.include_workspace = IncludeWorkspace();
  choice.protect_with_passphrase = protect_with_pin_->isChecked();
  choice.passphrase_acceptable = entry_->Acceptable();
  choice.total_bytes = TotalProfileExportBytes(
      BuildProfileExportContents(areas_, choice.include_workspace));
  choice.free_bytes = free_bytes_;
  return choice;
}

void ProfileExportDialog::slot_choose_destination() {
  auto suggestion = QString("%1/%2%3").arg(
      GetDefaultUserFilePath(),
      display_name_.simplified().replace(' ', '-').toLower(),
      kProfilePackageExtension);

  const auto chosen = QFileDialog::getSaveFileName(
      this, tr("Export Profile"), suggestion, ProfilePackageNameFilter());
  if (chosen.isEmpty()) return;

  destination_ = chosen.endsWith(kProfilePackageExtension, Qt::CaseInsensitive)
                     ? chosen
                     : chosen + kProfilePackageExtension;

  const QFileInfo info(destination_);
  destination_label_->setText(info.fileName());
  SetLabelTextColor(destination_label_, palette().color(QPalette::WindowText));

  // Free space as a fact, not a prediction. What the file will actually occupy
  // is unknowable before it is packed, because the payload is compressed on the
  // way out; what the volume has right now is simply true.
  const QStorageInfo storage(info.absolutePath());
  free_bytes_ = storage.isValid() ? storage.bytesAvailable() : -1;

  auto detail = QDir::toNativeSeparators(info.absolutePath());
  if (free_bytes_ >= 0) {
    detail += " · " + tr("%1 free").arg(HumanSize(free_bytes_));
  }
  destination_detail_->setText(detail);
  destination_detail_->show();

  slot_state_changed();
}

void ProfileExportDialog::slot_state_changed() {
  const auto with_pin = protect_with_pin_->isChecked();
  entry_->setEnabled(with_pin);

  const auto current = choice();

  // Re-render the sizes so the total visibly moves when the workspace box does;
  // a number that does not react to the checkbox above it teaches the user that
  // the list is decoration.
  const auto rows =
      BuildProfileExportContents(areas_, current.include_workspace);
  for (int i = 0; i < rows.size() && i < size_labels_.size(); ++i) {
    const auto& row = rows.at(i);
    auto* label = size_labels_.at(i);
    label->setText(HumanSize(row.bytes));

    // Dimmed when the row carries nothing — either because it is empty or
    // because it was left out. Five rows at equal weight make the reader work
    // out which ones matter; dimming the ones that do not puts the emphasis
    // where the bytes actually are.
    const auto carries_something = row.included && row.bytes > 0;
    const auto colour = carries_something
                            ? palette().color(QPalette::WindowText)
                            : MutedTextColor(palette());
    SetLabelTextColor(label, colour);
    if (i < row_widgets_.size()) {
      if (auto* name = qobject_cast<QLabel*>(row_widgets_.at(i));
          name != nullptr) {
        SetLabelTextColor(name, colour);
      }
    }
  }
  total_label_->setText(HumanSize(current.total_bytes));

  const auto readiness = EvaluateProfileExport(current);

  // Only the first warning is shown. They are ordered by what it would cost to
  // ignore them, and a stack of alarms is read as one alarm.
  if (readiness.warnings.isEmpty()) {
    warning_icon_->clear();
    warning_label_->clear();
  } else {
    const auto warning = readiness.warnings.front();
    warning_icon_->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(18, 18));
    warning_label_->setText(DescribeProfileExportWarning(warning));
    SetLabelTextColor(warning_label_,
                      warning == ProfileExportWarning::kMayNotFit
                          ? WarningColor(palette())
                          : DangerColor(palette()));
  }

  // Before a destination exists there is no summary to give, so the footer
  // carries the one thing still to do rather than sitting blank.
  const auto summary = DescribeProfileExport(
      current,
      destination_.isEmpty() ? QString{} : QFileInfo(destination_).fileName());
  summary_label_->setText(summary.isEmpty()
                              ? tr("Choose where to save the file to continue.")
                              : summary);

  buttons_->button(QDialogButtonBox::Ok)->setEnabled(readiness.can_export);
}

auto ProfileExportDialog::DestinationPath() const -> QString {
  return destination_;
}

auto ProfileExportDialog::IncludeWorkspace() const -> bool {
  return workspace_box_->isChecked() && workspace_box_->isEnabled();
}

auto ProfileExportDialog::Protection() const -> ProfilePackageProtection {
  return protect_with_pin_->isChecked() ? ProfilePackageProtection::kPIN
                                        : ProfilePackageProtection::kNONE;
}

auto ProfileExportDialog::Passphrase() const -> GFBuffer {
  if (!protect_with_pin_->isChecked()) return {};
  return entry_->Secret();
}

}  // namespace GpgFrontend::UI
