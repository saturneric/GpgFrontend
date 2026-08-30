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
#include "ui/dialog/SecretPrompt.h"
#include "ui/dialog/profile/ProfilePackageMeta.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/ProfileController.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/MetaListPanel.h"
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

ProfileExportDialog::ProfileExportDialog(QString display_name,
                                         const ProfileAccessor& storage,
                                         QWidget* parent)
    : GeneralDialog("profile_export_dialog", parent),
      display_name_(std::move(display_name)),
      areas_(MeasureProfileAreas(storage)) {
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

  // The same badge, heading and rule the passphrase prompt uses. Handing over a
  // whole profile deserves to look like the same class of act as unlocking one,
  // and sharing the furniture is what makes the two read as one family.
  layout->addLayout(CreateDialogHeader(
      QStringLiteral(":/icons/lock.png"),
      tr("Export \"%1\"").arg(display_name_),
      tr("Everything below is written into a single file you can copy to "
         "another computer and import there."),
      this));

  build_destination(layout);
  build_contents(layout);
  build_protection(layout);

  // One footer, not two rows. The warning and the sentence saying what is about
  // to happen are both about the same moment, and giving each its own reserved
  // height left a block of blank space above the button whenever there was
  // neither -- which at open is always. Reserving the height once, on the block
  // rather than on either label, still keeps the Export button from moving
  // under the cursor aiming at it.
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

  footer->setMinimumHeight(summary_label_->fontMetrics().lineSpacing() * 2);
  layout->addWidget(footer);

  buttons_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons_->button(QDialogButtonBox::Ok)->setText(tr("Export"));
  layout->addWidget(buttons_);

  connect(destination_button_, &QPushButton::clicked, this,
          &ProfileExportDialog::slot_choose_destination);
  connect(contents_, &MetaListPanel::SignalRowToggled, this,
          [this](int /*index*/, bool checked) {
            // The only checkable row in the list is the workspace; see
            // BuildProfileExportContents().
            include_workspace_ = checked;
            slot_state_changed();
          });
  connect(entry_, &SecretEntryPanel::SignalStateChanged, this,
          &ProfileExportDialog::slot_state_changed);
  connect(buttons_, &QDialogButtonBox::accepted, this,
          &ProfileExportDialog::accept);
  connect(buttons_, &QDialogButtonBox::rejected, this,
          &ProfileExportDialog::reject);

  slot_state_changed();
}

void ProfileExportDialog::build_destination(QVBoxLayout* layout) {
  auto* destination = new QWidget(this);
  auto* destination_layout = new QVBoxLayout(destination);
  destination_layout->setContentsMargins(0, 0, 0, 0);
  destination_layout->setSpacing(8);

  // Where the file goes is described in the same rows the file is described in
  // when it is opened again later, so the two ends of a package's life name it
  // the same way.
  destination_hint_ = new QLabel(tr("No file chosen yet."), destination);
  SetLabelTextColor(destination_hint_,
                    MutedTextColor(destination_hint_->palette()));
  destination_layout->addWidget(destination_hint_);

  destination_list_ = new MetaListPanel(destination);
  destination_list_->hide();
  destination_layout->addWidget(destination_list_);

  destination_button_ = new QPushButton(tr("Choose..."), destination);
  auto* button_row = new QHBoxLayout();
  button_row->addWidget(destination_button_);
  button_row->addStretch(1);
  destination_layout->addLayout(button_row);

  layout->addWidget(CreateCard(tr("Save to"), destination, this));
}

auto ProfileExportDialog::contents_rows() const -> QVector<MetaListRow> {
  return ToMetaListRows(BuildProfileExportContents(areas_, include_workspace_));
}

void ProfileExportDialog::build_contents(QVBoxLayout* layout) {
  contents_ = new MetaListPanel(this);
  contents_->SetRows(contents_rows());

  layout->addWidget(CreateCard(tr("What goes in"), contents_, this));
}

void ProfileExportDialog::build_protection(QVBoxLayout* layout) {
  auto* protection = new QWidget(this);
  auto* protection_layout = new QVBoxLayout(protection);
  protection_layout->setContentsMargins(0, 0, 0, 0);
  protection_layout->setSpacing(8);

  // The same fields, reveal toggle, strength meter and floor the application
  // PIN is chosen with. A generator is offered here and not there because this
  // secret is going to be written down rather than remembered.
  //
  // There is no second option beside it. The file carries the profile's own
  // key, so an unsealed one hands over everything the profile ever encrypted --
  // and a choice offered is a choice somebody makes by accident.
  auto texts = DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage,
                                        SecretPromptMode::kSET);
  texts.hint =
      tr("The file cannot be opened without this passphrase, and it cannot be "
         "recovered.");

  SecretEntryPanel::Config config;
  config.ask_current = false;
  config.ask_new = true;
  config.offer_generation = true;
  config.texts = texts;
  entry_ = new SecretEntryPanel(config, protection);
  protection_layout->addWidget(entry_);

  auto* mechanism = new MetaListPanel(protection);
  mechanism->SetRows(BuildExportProtectionRows());
  protection_layout->addWidget(mechanism);

  layout->addWidget(CreateCard(tr("Protection"), protection, this));
}

auto ProfileExportDialog::choice() const -> ProfileExportChoice {
  ProfileExportChoice choice;
  choice.has_destination = !destination_.isEmpty();
  choice.include_workspace = IncludeWorkspace();
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

  // Free space as a fact, not a prediction. What the file will actually occupy
  // is unknowable before it is packed, because the payload is compressed on the
  // way out; what the volume has right now is simply true.
  const QStorageInfo storage(info.absolutePath());
  free_bytes_ = storage.isValid() ? storage.bytesAvailable() : -1;

  destination_list_->SetRows(
      BuildProfilePackageDestinationRows(info, free_bytes_));
  destination_list_->show();
  destination_hint_->hide();

  slot_state_changed();
}

void ProfileExportDialog::slot_state_changed() {
  const auto current = choice();

  // Re-render the sizes so the total visibly moves when the workspace row is
  // ticked; a number that does not react to the checkbox above it teaches the
  // user that the list is decoration.
  contents_->RefreshValues(contents_rows());

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
    SetLabelTextColor(warning_label_, WarningColor(palette()));
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
  return include_workspace_;
}

auto ProfileExportDialog::Protection() const -> ProfilePackageProtection {
  return ProfilePackageProtection::kPIN;
}

auto ProfileExportDialog::Passphrase() const -> GFBuffer {
  return entry_->Secret();
}

}  // namespace GpgFrontend::UI
