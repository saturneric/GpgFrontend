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

#include "CreateFileSystemItemDialog.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace GpgFrontend::UI {

namespace {

auto HasPathSeparator(const QString& text) -> bool {
  return text.contains("/") || text.contains("\\");
}

auto IsReservedName(const QString& name) -> bool {
#ifdef Q_OS_WIN
  static const QSet<QString> kReservedNames = {
      "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
      "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
      "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
  };

  auto upper = QFileInfo(name).completeBaseName().toUpper();
  return kReservedNames.contains(upper);
#else
  Q_UNUSED(name);
  return false;
#endif
}

}  // namespace

CreateFileSystemItemDialog::CreateFileSystemItemDialog(
    ItemType item_type, const QString& target_dir, QWidget* parent)
    : QDialog(parent),
      item_type_(item_type),
      target_dir_(QDir::cleanPath(target_dir)) {
  setModal(true);
  setWindowTitle(item_type_ == ItemType::kFOLDER ? tr("New Folder")
                                                 : tr("New File"));
  resize(460, 180);

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(18, 18, 18, 18);
  root_layout->setSpacing(12);

  title_label_ = new QLabel(item_type_ == ItemType::kFOLDER
                                ? tr("Create a new folder")
                                : tr("Create a new empty file"),
                            this);

  QFont title_font = title_label_->font();
  title_font.setBold(true);
  title_font.setPointSize(title_font.pointSize() + 1);
  title_label_->setFont(title_font);

  location_label_ = new QLabel(this);
  location_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  location_label_->setWordWrap(true);
  location_label_->setText(tr("Location: %1").arg(target_dir_));

  name_edit_ = new QLineEdit(this);
  name_edit_->setClearButtonEnabled(true);
  name_edit_->setPlaceholderText(item_type_ == ItemType::kFOLDER
                                     ? tr("Folder name")
                                     : tr("File name, e.g. notes.txt"));

  hint_label_ = new QLabel(this);
  hint_label_->setWordWrap(true);

  auto* form_layout = new QFormLayout();
  form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  form_layout->addRow(tr("Name:"), name_edit_);

  button_box_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

  button_box_->button(QDialogButtonBox::Ok)->setText(tr("Create"));

  root_layout->addWidget(title_label_);
  root_layout->addWidget(location_label_);
  root_layout->addLayout(form_layout);
  root_layout->addWidget(hint_label_);
  root_layout->addStretch();
  root_layout->addWidget(button_box_);

  connect(name_edit_, &QLineEdit::textChanged, this,
          &CreateFileSystemItemDialog::UpdateState);

  connect(button_box_, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);

  UpdateState();
  name_edit_->setFocus();
}

auto CreateFileSystemItemDialog::GetName() const -> QString {
  return name_edit_->text().trimmed();
}

auto CreateFileSystemItemDialog::GetPath() const -> QString {
  return QDir(target_dir_).absoluteFilePath(GetName());
}

void CreateFileSystemItemDialog::UpdateState() {
  const auto name = GetName();
  auto* ok_button = button_box_->button(QDialogButtonBox::Ok);

  QString message;
  bool valid = true;

  if (name.isEmpty()) {
    valid = false;
    message = item_type_ == ItemType::kFOLDER ? tr("Enter a folder name.")
                                              : tr("Enter a file name.");
  } else if (name == "." || name == "..") {
    valid = false;
    message = tr("This name is reserved.");
  } else if (HasPathSeparator(name)) {
    valid = false;
    message = tr("The name must not contain path separators.");
  } else if (IsReservedName(name)) {
    valid = false;
    message = tr("This name is reserved by the operating system.");
  } else if (QFileInfo::exists(GetPath())) {
    valid = false;
    message = tr("A file or folder with this name already exists.");
  } else {
    message =
        item_type_ == ItemType::kFOLDER
            ? tr("The folder will be created in the selected location.")
            : tr("An empty file will be created in the selected location.");
  }

  ok_button->setEnabled(valid);

  hint_label_->setText(message);
  hint_label_->setProperty("error", !valid && !name.isEmpty());
  hint_label_->style()->unpolish(hint_label_);
  hint_label_->style()->polish(hint_label_);
}

}  // namespace GpgFrontend::UI