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

#include "KeyDatabaseEditDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

//
#include "ui_KeyDatabaseEditDialog.h"

namespace GpgFrontend::UI {

namespace {

/// Room the form's label column and the dialog's margins take from the width a
/// path has to fit into. Approximate by nature -- it is a budget for eliding
/// text, not a layout constraint.
constexpr int kPathLabelInset = 170;

}  // namespace

KeyDatabaseEditDialog::KeyDatabaseEditDialog(
    Mode mode, QContainer<KeyDatabaseInfo> key_db_infos, QWidget* parent)
    : GeneralDialog("KeyDatabaseEditDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabaseEditDialog>()),
      mode_(mode),
      channel_(-1),
      key_database_infos_(std::move(key_db_infos)),
      is_sandbox_(IsRunningInSandBox()) {
  ui_->setupUi(this);

  init_ui();
}

KeyDatabaseEditDialog::KeyDatabaseEditDialog(
    Mode mode, QContainer<KeyDatabaseInfo> key_db_infos, int index,
    QWidget* parent)
    : GeneralDialog("KeyDatabaseEditDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabaseEditDialog>()),
      mode_(mode),
      channel_(-1),
      key_database_infos_(std::move(key_db_infos)),
      is_sandbox_(IsRunningInSandBox()) {
  ui_->setupUi(this);

  if (index < 0 || index >= key_database_infos_.size()) {
    throw std::out_of_range("Index out of range in KeyDatabaseEditDialog");
  }

  const auto& key_db_info = key_database_infos_[index];
  default_name_ = key_db_info.name;
  default_path_ = key_db_info.origin_path;
  channel_ = index;

  LOG_D() << "edit key database, index: " << index << "name: " << default_name_
          << "path: " << default_path_ << "channel: " << channel_;

  init_ui();
}

void KeyDatabaseEditDialog::init_ui() {
  ui_->convert2RelativePathCheckBox->setChecked(
      GlobalSettingStation::GetInstance().IsProtableMode());
  ui_->keyDBNameLineEdit->setText(default_name_);
  if (!default_path_.isEmpty()) {
    path_ = QFileInfo(default_path_).absoluteFilePath();
  }

  ui_->keyDBBackendTypeComboBox->clear();

  const auto engines = GetGSS().AllSupportedEngines();
  for (const auto& engine : engines) {
    ui_->keyDBBackendTypeComboBox->addItem(engine, engine.toUpper());
  }

  if (is_editing()) {
    // findData rather than an index: "rPGP is index 1" holds only in a build
    // that shipped both engines, and the rpgp-only build is exactly the one
    // where getting this wrong is silent.
    const auto engine = ConvertOpenPGPEngine2String(
        OpenPGPContext::GetInstance(channel_).Engine());
    const auto engine_index =
        ui_->keyDBBackendTypeComboBox->findData(engine.toUpper());
    if (engine_index != -1) {
      ui_->keyDBBackendTypeComboBox->setCurrentIndex(engine_index);
    }

    // A database's engine is the format its keyring was written in, so it is
    // fixed for the life of the database whichever kind it is.
    ui_->keyDBBackendTypeComboBox->setEnabled(false);
  } else {
    const auto default_engine = GetSettings()
                                    .value("basic/default_engine", "GNUPG")
                                    .toString()
                                    .toUpper();
    const auto engine_index =
        ui_->keyDBBackendTypeComboBox->findData(default_engine);
    ui_->keyDBBackendTypeComboBox->setCurrentIndex(
        engine_index != -1 ? engine_index : 0);
  }

  ui_->keyDBNameLabel->setText(tr("Name"));
  ui_->keyDBPathLabel->setText(tr("Folder"));
  ui_->keyDBBackendTypeLabel->setText(tr("Engine"));
  ui_->selectKeyDBButton->setText(tr("Choose Folder…"));
  ui_->convert2RelativePathCheckBox->setText(tr("Convert to Relative Path"));
  ui_->keyDBNameLineEdit->setPlaceholderText(tr("e.g. Personal Keys"));

  // The headline says what the dialog is for, so the window title does not have
  // to be read to find out -- and so the hint under it reads as an explanation
  // of something rather than as a floating remark.
  auto headline_font = ui_->headlineLabel->font();
  headline_font.setBold(true);
  ui_->headlineLabel->setFont(headline_font);

  // Muted, but not italic: the hint is a full sentence the user is meant to
  // read, and a whole italic paragraph is harder to read than a plain one.
  ui_->modeHintLabel->setStyleSheet("color: palette(mid);");
  ui_->keyDBPathShowLabel->setStyleSheet("color: palette(mid);");

  QRegularExpression safe_string_re(R"([a-zA-Z0-9\s\-_]+)");
  auto* safe_validator = new QRegularExpressionValidator(safe_string_re, this);
  ui_->keyDBNameLineEdit->setValidator(safe_validator);

  // External modes only: let the user pick any existing folder themselves.
  connect(ui_->selectKeyDBButton, &QPushButton::clicked, this, [this](bool) {
    auto path = QFileDialog::getExistingDirectory(
        this, tr("Open Directory"),
        default_path_.isEmpty() ? QDir::homePath() : default_path_,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (path.trimmed().isEmpty()) return;

    LOG_D() << "selected key database path: " << path;

    // Refused rather than reported: this used to show the message and then take
    // the path anyway, so the warning said one thing and the dialog did
    // another.
    if (!check_custom_gnupg_key_database_path(path)) {
      QMessageBox::critical(this, tr("Illegal GnuPG Key Database Path"),
                            tr("Target GnuPG Key Database Path is not an "
                               "exists readable directory."));
      return;
    }

    if (path != path_) {
      path_ = QFileInfo(path).absoluteFilePath();
      update_path_display();
    }
  });

  // Managed modes: the folder is derived from the name as the user types.
  connect(ui_->keyDBNameLineEdit, &QLineEdit::textChanged, this,
          [this](const QString&) -> void {
            if (is_managed()) update_generated_path();
          });

  // Suggest a name for a new database, so the one field the user has to fill in
  // starts out valid.
  if (default_name_.isEmpty()) {
    ui_->keyDBNameLineEdit->setText(suggested_name());
  }

  apply_mode();

  this->setAttribute(Qt::WA_DeleteOnClose);
  connect(ui_->globalButtonBox, &QDialogButtonBox::accepted, this,
          &KeyDatabaseEditDialog::slot_button_box_accepted);
  connect(ui_->globalButtonBox, &QDialogButtonBox::rejected, this,
          &KeyDatabaseEditDialog::reject);
  slot_clear_err_msg();
  setModal(true);
}

void KeyDatabaseEditDialog::apply_mode() {
  const auto managed = is_managed();

  // A managed database lives where the profile keeps its key databases, so
  // there is no folder to choose and nothing to rewrite against the executable.
  // Never in the sandbox either, where the location is fixed for every
  // database.
  ui_->selectKeyDBButton->setVisible(!managed && !is_sandbox_);
  ui_->convert2RelativePathCheckBox->setVisible(!managed && !is_sandbox_);
  if (managed || is_sandbox_) {
    ui_->convert2RelativePathCheckBox->setChecked(false);
  }

  // Offered only where there is a decision to make. A build with one engine has
  // no choice to present, and a row that already exists cannot change format.
  const auto engine_choosable =
      !is_editing() && ui_->keyDBBackendTypeComboBox->count() > 1;
  ui_->keyDBBackendTypeLabel->setVisible(engine_choosable || is_editing());
  ui_->keyDBBackendTypeComboBox->setVisible(engine_choosable || is_editing());

  switch (mode_) {
    case Mode::kADD_MANAGED:
      setWindowTitle(tr("Add Key Database"));
      ui_->headlineLabel->setText(tr("New key database in your profile"));
      ui_->modeHintLabel->setText(
          tr("GpgFrontend picks the folder and keeps it inside your profile, "
             "so this key database travels with it. Just choose a name."));
      update_generated_path();
      break;
    case Mode::kRENAME_MANAGED:
      setWindowTitle(tr("Rename Key Database"));
      ui_->headlineLabel->setText(tr("Rename this key database"));
      ui_->modeHintLabel->setText(tr(
          "The folder it is kept in is named after it, and is renamed too."));
      update_generated_path();
      break;
    case Mode::kADD_EXTERNAL:
      setWindowTitle(tr("Add Key Database On This Computer"));
      ui_->headlineLabel->setText(tr("New key database on this computer"));
      ui_->modeHintLabel->setText(
          tr("Choose a folder yourself. A key database outside your profile "
             "belongs to this computer alone, and a profile package never "
             "carries it."));
      update_path_display();
      break;
    case Mode::kEDIT_EXTERNAL:
      setWindowTitle(tr("Edit Key Database"));
      ui_->headlineLabel->setText(tr("Key database on this computer"));
      ui_->modeHintLabel->setText(
          tr("This key database belongs to this computer alone, and a profile "
             "package never carries it."));
      update_path_display();
      break;
  }
}

void KeyDatabaseEditDialog::update_generated_path() {
  name_ = ui_->keyDBNameLineEdit->text().trimmed();
  path_ = ManagedKeyDatabasePath(GetGSS().GetAppDataPath(), name_);
  update_path_display();
}

auto KeyDatabaseEditDialog::suggested_name() const -> QString {
  // Named for the list it is joining. The two kinds sit in different tabs and
  // differ in the thing that matters most about a key database -- whether it
  // travels with the profile -- so a database called "Key DB 2" that turns out
  // to live on this computer alone is a name that hides the distinction the
  // rest of this screen exists to draw.
  //
  // Not translated, deliberately: this name becomes a directory, and the field
  // it goes into accepts only [a-zA-Z0-9\s\-_]. A localised suggestion could be
  // one the field itself then refuses.
  const auto prefix = is_managed() ? QStringLiteral("Key DB")
                                   : QStringLiteral("External Key DB");

  // The first unused number, rather than one more than the count. Removing the
  // first of two databases and adding another used to suggest the name the
  // remaining one already had, so the user met a duplicate-name error about a
  // name they had not chosen.
  for (int n = 1;; ++n) {
    const auto candidate = QString("%1 %2").arg(prefix).arg(n);

    const auto taken = std::any_of(
        key_database_infos_.begin(), key_database_infos_.end(),
        [&candidate](const KeyDatabaseInfo& info) -> bool {
          return info.name.compare(candidate, Qt::CaseInsensitive) == 0;
        });

    if (!taken) return candidate;
  }
}

void KeyDatabaseEditDialog::update_path_display() {
  auto* label = ui_->keyDBPathShowLabel;

  if (path_.isEmpty()) {
    // Two different reasons for there being no folder yet, and two different
    // things to do about it: a managed database derives its folder from a name
    // that has not been typed, while an external one is waiting for a folder to
    // be picked.
    label->setText(is_managed() ? tr("Choose a name first")
                                : tr("No folder chosen yet"));
    label->setToolTip({});
    return;
  }

  const auto native = QDir::toNativeSeparators(path_);

  // Elided against the dialog's width rather than the label's own. The label's
  // width is the wrong thing to ask twice over: before the first layout it does
  // not have one, and afterwards it is whatever this very function's last
  // answer made it -- a loop that settles wherever it happens to start.
  //
  // The dialog's width is the fact that does not depend on the text, so the
  // path is elided to fit beside the field column and the label then asks for
  // exactly that.
  const auto available = std::max(width() - kPathLabelInset, 240);

  const QFontMetrics metrics(label->font());
  label->setText(metrics.elidedText(native, Qt::ElideMiddle, available));

  // The whole path, for the cases the elision hid the interesting part of.
  label->setToolTip(native);
}

void KeyDatabaseEditDialog::resizeEvent(QResizeEvent* event) {
  GeneralDialog::resizeEvent(event);
  update_path_display();
}

void KeyDatabaseEditDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);

  // Queued, because the label still has its pre-layout width while this runs.
  QTimer::singleShot(0, this, [this]() { update_path_display(); });
}

void KeyDatabaseEditDialog::slot_button_box_accepted() {
  // Trimmed, because the path was built from the trimmed name while this was
  // not: a trailing space produced a database whose name and folder disagreed,
  // and on Windows a folder that cannot be created at all.
  name_ = ui_->keyDBNameLineEdit->text().trimmed();

  if (name_.isEmpty()) {
    slot_show_err_msg(tr("The key database name cannot be empty."));
    return;
  }

  // The name belongs to the derived database, which is not one the user makes:
  // a second thing answering to it would be found first by every lookup that
  // goes by name, and would take the default engine along with the identity.
  // There is no mode here that can produce it -- it is a checkbox in settings.
  if (IsReservedKeyDatabaseName(name_)) {
    slot_show_err_msg(tr("\"%1\" is the name of the key database GpgFrontend "
                         "derives from your OpenPGP engine. Turn that one on "
                         "with the checkbox in Key Database settings, or pick "
                         "another name.")
                          .arg(name_));
    return;
  }

  if (path_.isEmpty()) {
    slot_show_err_msg(tr("The key database path cannot be empty."));
    return;
  }

  // Case-insensitively, for the same reason the reserved name is: the name
  // becomes a folder, and "Work" and "work" are one folder on Windows and on
  // macOS. Two entries agreeing only in case would name the same keyring while
  // looking like two.
  for (const auto& info : key_database_infos_) {
    if (default_name_.compare(name_, Qt::CaseInsensitive) == 0) break;
    if (info.name.compare(name_, Qt::CaseInsensitive) == 0) {
      slot_show_err_msg(tr("A key database with the name '%1' already exists. "
                           "Please choose a different name.")
                            .arg(name_));
      return;
    }
  }

  if (ui_->convert2RelativePathCheckBox->isChecked() && !is_managed()) {
    path_ = QDir(GlobalSettingStation::GetInstance().GetAppDir())
                .relativeFilePath(path_);
  }

  backend_type_ =
      ui_->keyDBBackendTypeComboBox->currentText().toLower().trimmed();
  if (backend_type_.isEmpty()) {
    backend_type_ = "gnupg";
  }

  slot_clear_err_msg();
  emit SignalKeyDatabaseInfoAccepted(name_, backend_type_, path_);
  this->accept();
}

auto KeyDatabaseEditDialog::check_custom_gnupg_key_database_path(
    const QString& path) -> bool {
  if (path.isEmpty()) return false;

  QFileInfo const dir_info(path);
  return dir_info.exists() && dir_info.isReadable() && dir_info.isDir();
}

void KeyDatabaseEditDialog::slot_show_err_msg(const QString& error_msg) {
  ui_->errorLabel->setText(error_msg);
  ui_->errorLabel->setStyleSheet("color: red;");
  ui_->errorLabel->setHidden(false);
}

void KeyDatabaseEditDialog::slot_clear_err_msg() {
  ui_->errorLabel->setText({});
  ui_->errorLabel->setHidden(true);
}
};  // namespace GpgFrontend::UI
