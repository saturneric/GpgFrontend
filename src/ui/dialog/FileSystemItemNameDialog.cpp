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

#include "FileSystemItemNameDialog.h"

#include <QDir>
#include <QFileInfo>

#include "ui/function/FileSystemItemRules.h"

namespace GpgFrontend::UI {

FileSystemItemNameDialog::FileSystemItemNameDialog(ItemType item_type,
                                                   const QString& target_dir,
                                                   QWidget* parent)
    : QDialog(parent),
      mode_(Mode::kCREATE),
      item_type_(item_type),
      target_dir_(QDir::cleanPath(target_dir)) {
  init_ui();
}

FileSystemItemNameDialog::FileSystemItemNameDialog(const QString& path,
                                                   QWidget* parent)
    : QDialog(parent), mode_(Mode::kRENAME) {
  const QFileInfo info(path);

  item_type_ = info.isDir() ? ItemType::kFOLDER : ItemType::kFILE;
  target_dir_ = QDir::cleanPath(info.absolutePath());
  original_name_ = info.fileName();
  original_path_ = info.absoluteFilePath();

  init_ui();
}

void FileSystemItemNameDialog::init_ui() {
  const auto renaming = mode_ == Mode::kRENAME;
  const auto folder = item_type_ == ItemType::kFOLDER;

  setModal(true);

  if (renaming) {
    setWindowTitle(folder ? tr("Rename Folder") : tr("Rename File"));
  } else {
    setWindowTitle(folder ? tr("New Folder") : tr("New File"));
  }

  resize(460, 180);

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(18, 18, 18, 18);
  root_layout->setSpacing(12);

  QString title_text;
  if (renaming) {
    title_text = folder ? tr("Rename this folder") : tr("Rename this file");
  } else {
    title_text =
        folder ? tr("Create a new folder") : tr("Create a new empty file");
  }

  title_label_ = new QLabel(title_text, this);

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
  name_edit_->setPlaceholderText(folder ? tr("Folder name")
                                        : tr("File name, e.g. notes.txt"));

  hint_label_ = new QLabel(this);
  hint_label_->setWordWrap(true);

  auto* form_layout = new QFormLayout();
  form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  form_layout->addRow(tr("Name:"), name_edit_);

  button_box_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

  button_box_->button(QDialogButtonBox::Ok)
      ->setText(renaming ? tr("Rename") : tr("Create"));

  root_layout->addWidget(title_label_);
  root_layout->addWidget(location_label_);
  root_layout->addLayout(form_layout);
  root_layout->addWidget(hint_label_);
  root_layout->addStretch();
  root_layout->addWidget(button_box_);

  connect(name_edit_, &QLineEdit::textChanged, this,
          &FileSystemItemNameDialog::UpdateState);

  connect(button_box_, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);

  UpdateState();
  name_edit_->setFocus();

  if (renaming) {
    name_edit_->setText(original_name_);

    // the extension is rarely what is being changed, so leave it out of the
    // selection and let the first keystroke replace the name alone.
    const auto base_name_length =
        QFileInfo(original_name_).completeBaseName().size();

    if (!folder && base_name_length > 0) {
      name_edit_->setSelection(0, static_cast<int>(base_name_length));
    } else {
      name_edit_->selectAll();
    }
  }
}

auto FileSystemItemNameDialog::GetName() const -> QString {
  return name_edit_->text().trimmed();
}

auto FileSystemItemNameDialog::GetPath() const -> QString {
  return QDir(target_dir_).absoluteFilePath(GetName());
}

auto FileSystemItemNameDialog::target_is_taken() const -> bool {
  const auto path = GetPath();

  if (!QFileInfo::exists(path)) return false;

  if (mode_ == Mode::kRENAME &&
      QFileInfo(path).canonicalFilePath() ==
          QFileInfo(original_path_).canonicalFilePath()) {
    return false;
  }

  return true;
}

void FileSystemItemNameDialog::UpdateState() {
  const auto name = GetName();
  const auto folder = item_type_ == ItemType::kFOLDER;
  auto* ok_button = button_box_->button(QDialogButtonBox::Ok);

  const auto status = ValidateFileSystemItemName(
      name, mode_ == Mode::kRENAME ? original_name_ : QString(),
      target_is_taken());

  QString message;

  switch (status) {
    case FileSystemItemNameStatus::kEMPTY:
      message = folder ? tr("Enter a folder name.") : tr("Enter a file name.");
      break;
    case FileSystemItemNameStatus::kDOT_NAME:
      message = tr("This name is reserved.");
      break;
    case FileSystemItemNameStatus::kPATH_SEPARATOR:
      message = tr("The name must not contain path separators.");
      break;
    case FileSystemItemNameStatus::kOS_RESERVED:
      message = tr("This name is reserved by the operating system.");
      break;
    case FileSystemItemNameStatus::kUNCHANGED:
      message = tr("Enter a name different from the current one.");
      break;
    case FileSystemItemNameStatus::kALREADY_EXISTS:
      message = tr("A file or folder with this name already exists.");
      break;
    case FileSystemItemNameStatus::kOK:
      if (mode_ == Mode::kRENAME) {
        message = folder ? tr("The folder will be renamed in place.")
                         : tr("The file will be renamed in place.");
        break;
      }

      message =
          folder
              ? tr("The folder will be created in the selected location.")
              : tr("An empty file will be created in the selected location.");
      break;
  }

  const auto valid = status == FileSystemItemNameStatus::kOK;

  ok_button->setEnabled(valid);

  hint_label_->setText(message);
  hint_label_->setProperty("error", !valid && !name.isEmpty());
  hint_label_->style()->unpolish(hint_label_);
  hint_label_->style()->polish(hint_label_);
}

}  // namespace GpgFrontend::UI
