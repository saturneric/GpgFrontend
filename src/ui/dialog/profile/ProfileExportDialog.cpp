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

#include "ui/UserInterfaceUtils.h"

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
  setMinimumWidth(520);

  auto* intro = new QLabel(
      tr("\"%1\" is written into a single file you can copy to another "
         "computer and import there.")
          .arg(display_name_),
      this);
  intro->setWordWrap(true);
  layout->addWidget(intro);

  auto* destination_row = new QHBoxLayout();
  destination_label_ = new QLabel(tr("— choose a file —"), this);
  destination_label_->setStyleSheet("color: gray;");
  destination_button_ = new QPushButton(tr("Choose..."), this);
  destination_row->addWidget(new QLabel(tr("Save to"), this));
  destination_row->addWidget(destination_label_, 1);
  destination_row->addWidget(destination_button_);
  layout->addLayout(destination_row);

  auto* contents = new QGroupBox(tr("What goes in"), this);
  auto* contents_layout = new QVBoxLayout(contents);

  contents_label_ = new QLabel(describe_contents(), contents);
  contents_label_->setWordWrap(true);
  contents_layout->addWidget(contents_label_);

  workspace_box_ = new QCheckBox(tr("Include my workspace files (%1)")
                                     .arg(HumanSize(areas_.value("workspace"))),
                                 contents);
  // Off unless asked for. The workspace has no size limit and is precisely
  // where the cleartext of things meant to be encrypted ends up; nobody should
  // discover after the fact that their drafts travelled inside a file they
  // emailed to someone.
  workspace_box_->setChecked(false);
  workspace_box_->setEnabled(areas_.value("workspace") > 0);
  contents_layout->addWidget(workspace_box_);
  layout->addWidget(contents);

  auto* protection = new QGroupBox(tr("Protection"), this);
  auto* protection_layout = new QVBoxLayout(protection);

  protect_with_pin_ =
      new QRadioButton(tr("Protect with a passphrase"), protection);
  protect_with_pin_->setChecked(true);
  protection_layout->addWidget(protect_with_pin_);

  auto* form = new QFormLayout();
  passphrase_edit_ = new QLineEdit(protection);
  passphrase_edit_->setEchoMode(QLineEdit::Password);
  confirm_edit_ = new QLineEdit(protection);
  confirm_edit_->setEchoMode(QLineEdit::Password);
  form->addRow(tr("Passphrase"), passphrase_edit_);
  form->addRow(tr("Repeat"), confirm_edit_);
  protection_layout->addLayout(form);

  protect_with_nothing_ = new QRadioButton(tr("No protection"), protection);
  protection_layout->addWidget(protect_with_nothing_);

  protection_hint_ = new QLabel(protection);
  protection_hint_->setWordWrap(true);
  auto hint_policy = protection_hint_->sizePolicy();
  hint_policy.setHeightForWidth(true);
  hint_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
  protection_hint_->setSizePolicy(hint_policy);
  protection_layout->addWidget(protection_hint_);

  layout->addWidget(protection);

  buttons_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons_->button(QDialogButtonBox::Ok)->setText(tr("Export"));
  layout->addWidget(buttons_);

  connect(destination_button_, &QPushButton::clicked, this,
          &ProfileExportDialog::slot_choose_destination);
  connect(protect_with_pin_, &QRadioButton::toggled, this,
          &ProfileExportDialog::slot_state_changed);
  connect(passphrase_edit_, &QLineEdit::textChanged, this,
          &ProfileExportDialog::slot_state_changed);
  connect(confirm_edit_, &QLineEdit::textChanged, this,
          &ProfileExportDialog::slot_state_changed);
  connect(buttons_, &QDialogButtonBox::accepted, this,
          &ProfileExportDialog::slot_accept);
  connect(buttons_, &QDialogButtonBox::rejected, this,
          &ProfileExportDialog::reject);

  slot_state_changed();
}

auto ProfileExportDialog::describe_contents() const -> QString {
  const auto line = [](const QString& name, qint64 bytes) {
    return QString("%1 — %2<br/>").arg(name, HumanSize(bytes));
  };

  return line(tr("Settings"), areas_.value("config")) +
         line(tr("Saved state, key groups and categories"),
              areas_.value("data_objs")) +
         line(tr("Keys stored inside this profile"),
              areas_.value("key_databases")) +
         "<span style='color: gray;'>" +
         tr("Logs and modules are never included. Keys kept outside this "
            "profile, such as the system GnuPG keyring, stay where they are.") +
         "</span>";
}

void ProfileExportDialog::slot_choose_destination() {
  auto suggestion =
      QString("%1/%2.gfprofile")
          .arg(GetDefaultUserFilePath(),
               display_name_.simplified().replace(' ', '-').toLower());

  const auto chosen = QFileDialog::getSaveFileName(
      this, tr("Export Profile"), suggestion,
      tr("GpgFrontend Profile File") + " (*.gfprofile)");
  if (chosen.isEmpty()) return;

  destination_ = chosen.endsWith(".gfprofile", Qt::CaseInsensitive)
                     ? chosen
                     : chosen + ".gfprofile";
  destination_label_->setText(QDir::toNativeSeparators(destination_));
  destination_label_->setStyleSheet({});
  slot_state_changed();
}

void ProfileExportDialog::slot_state_changed() {
  const auto with_pin = protect_with_pin_->isChecked();

  passphrase_edit_->setEnabled(with_pin);
  confirm_edit_->setEnabled(with_pin);

  const auto passphrase = passphrase_edit_->text();
  const auto matched = passphrase == confirm_edit_->text();

  if (with_pin) {
    protection_hint_->setText(
        "<span style='color: gray;'>" +
        tr("The file cannot be opened without this passphrase, and there is no "
           "way to recover it. Keys wrapped by this computer's keychain are "
           "not used: the file has to open on another computer.") +
        "</span>" +
        (matched || confirm_edit_->text().isEmpty()
             ? QString{}
             : "<br/><b>" + tr("The two entries do not match.") + "</b>"));
  } else {
    // Said plainly, because it is the whole story: the profile's own key is
    // inside this file, and without protection so is everything it opens.
    protection_hint_->setText(
        "<b>" +
        tr("Anyone who gets this file can read your keys and everything in "
           "the profile, and can change it before you import it.") +
        "</b>");
  }

  buttons_->button(QDialogButtonBox::Ok)
      ->setEnabled(!destination_.isEmpty() &&
                   (!with_pin || (!passphrase.isEmpty() && matched)));
}

void ProfileExportDialog::slot_accept() {
  if (destination_.isEmpty()) return;
  accept();
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
  return GFBuffer(passphrase_edit_->text());
}

}  // namespace GpgFrontend::UI
