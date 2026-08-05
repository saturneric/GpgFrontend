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

#include "ui/dialog/profile/ProfileManagerDialog.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>

#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "core/model/SettingsObject.h"
#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/FilesystemUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/profile/ProfileCreateDialog.h"
#include "ui/dialog/profile/ProfileExportDialog.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/ProfileController.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kColumnName = 0;
constexpr int kColumnId = 1;
constexpr int kColumnKind = 2;
constexpr int kColumnLastOpened = 3;
constexpr int kColumnStatus = 4;

}  // namespace

ProfileManagerDialog::ProfileManagerDialog(QWidget* parent)
    : GeneralDialog("profile_manager_dialog", parent) {
  init_ui();
  reload();
  setWindowTitle(tr("Open Profile"));
  setModal(true);

  // GeneralDialog deletes itself on close, which would free this one out from
  // under the caller that allocated it on the stack.
  setAttribute(Qt::WA_DeleteOnClose, false);

  movePosition2CenterOfParent();
}

void ProfileManagerDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);

  hint_ = new QLabel(
      tr("Each profile keeps its own settings, keys and saved state. Opening "
         "one starts a new window; this window stays exactly as it is."),
      this);
  hint_->setWordWrap(true);
  hint_->setStyleSheet("color: gray;");

  // Word-wrapped labels report one line of height unless asked to declare
  // heightForWidth, and the dialog is sized from what its layout reports.
  auto hint_policy = hint_->sizePolicy();
  hint_policy.setHeightForWidth(true);
  hint_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
  hint_->setSizePolicy(hint_policy);
  layout->addWidget(hint_);

  table_ = new QTableWidget(this);
  table_->setColumnCount(5);
  table_->setHorizontalHeaderLabels(
      {tr("Name"), tr("Folder"), tr("Type"), tr("Last Opened"), tr("Status")});
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->hide();
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setMinimumSize(680, 320);
  layout->addWidget(table_);

  auto* buttons = new QHBoxLayout();
  open_button_ = new QPushButton(tr("Open"), this);
  open_button_->setDefault(true);
  create_button_ = new QPushButton(tr("New..."), this);
  import_button_ = new QPushButton(tr("Import..."), this);
  export_button_ = new QPushButton(tr("Export..."), this);
  delete_button_ = new QPushButton(tr("Delete"), this);
  reveal_button_ = new QPushButton(tr("Open Folder"), this);

  import_button_->setToolTip(
      tr("Read a profile file into a new profile on this computer"));
  // Only the profile this window has open can be exported: its key is in
  // memory here, and a profile that is not open may have its key sealed by
  // this computer's keychain, which a file carried elsewhere could not undo.
  export_button_->setToolTip(
      tr("Write the profile this window is using into a single file"));

  buttons->addWidget(open_button_);
  buttons->addWidget(create_button_);
  buttons->addWidget(import_button_);
  buttons->addWidget(export_button_);
  buttons->addWidget(delete_button_);
  buttons->addWidget(reveal_button_);
  buttons->addStretch();

  auto* close = new QPushButton(tr("Close"), this);
  buttons->addWidget(close);
  layout->addLayout(buttons);

  connect(open_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_open);
  connect(import_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_import);
  connect(export_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_export);
  connect(create_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_create);
  connect(delete_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_delete);
  connect(reveal_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_reveal);
  connect(close, &QPushButton::clicked, this, &ProfileManagerDialog::accept);
  connect(table_, &QTableWidget::itemSelectionChanged, this,
          &ProfileManagerDialog::slot_selection_changed);
  connect(table_, &QTableWidget::itemDoubleClicked, this,
          &ProfileManagerDialog::slot_open);
}

void ProfileManagerDialog::reload() {
  data_ = LoadProfiles();
  const auto current = ProfileSession::Instance().Profile().Id();

  table_->setRowCount(static_cast<int>(data_.profiles.size()));
  for (int row = 0; row < data_.profiles.size(); ++row) {
    const auto& e = data_.profiles.at(row);
    const auto is_current = e.id == current;

    auto* name = new QTableWidgetItem(is_current ? tr("%1  (open)").arg(e.name)
                                                 : e.name);
    if (is_current) {
      auto font = name->font();
      font.setBold(true);
      name->setFont(font);
    }
    name->setToolTip(e.root);

    table_->setItem(row, kColumnName, name);
    table_->setItem(row, kColumnId, new QTableWidgetItem(e.id));
    table_->setItem(row, kColumnKind,
                    new QTableWidgetItem(ProfileKindDisplayName(e.kind)));
    table_->setItem(row, kColumnLastOpened,
                    new QTableWidgetItem(e.last_opened));

    // "in use elsewhere" is worth showing here rather than only on a failed
    // switch: it explains the refusal before the user runs into it
    QString status;
    if (is_current) {
      status = tr("This window");
    } else if (!QFileInfo::exists(e.root)) {
      status = tr("Not created yet");
    } else if (ProfileLock::Probe(e.root).status ==
               ProfileLockStatus::kHELD_ELSEWHERE) {
      status = tr("Open in another window");
    }
    table_->setItem(row, kColumnStatus, new QTableWidgetItem(status));
  }

  table_->resizeColumnsToContents();
  slot_selection_changed();
}

auto ProfileManagerDialog::selected() const
    -> std::optional<ProfileRegistryEntry> {
  const auto row = table_->currentRow();
  if (row < 0 || row >= data_.profiles.size()) return {};
  return data_.profiles.at(row);
}

void ProfileManagerDialog::slot_selection_changed() {
  const auto entry = selected();
  const auto current = ProfileSession::Instance().Profile().Id();

  const auto has = entry.has_value();
  open_button_->setEnabled(has && entry->id != current);
  reveal_button_->setEnabled(has && QFileInfo::exists(entry->root));

  // Classic and portable are resolved by the application itself, so there is
  // nothing coherent for "delete" to mean; the current profile cannot be
  // deleted out from under the process that is using it.
  delete_button_->setEnabled(has && !entry->implicit && entry->id != current);
}

void ProfileManagerDialog::slot_open() {
  const auto entry = selected();
  if (!entry.has_value()) return;
  if (entry->id == ProfileSession::Instance().Profile().Id()) return;

  // No confirmation: nothing is closed, nothing is changed, and a window the
  // user did not want is closed again in one click.
  const auto result = OpenProfileInNewWindow({.profile_id = entry->id});
  if (!result.Ok()) {
    QMessageBox::warning(this, tr("Cannot Open Profile"), result.detail,
                         QMessageBox::Ok);
    reload();
    return;
  }

  accept();
}

void ProfileManagerDialog::slot_export() {
  const auto& session = ProfileSession::Instance();
  const auto& profile = session.Profile();

  ProfileExportDialog dialog(CurrentProfileDisplayName(), profile.Root(), this);
  if (dialog.exec() != QDialog::Accepted) return;

  // Stored key database paths are normalised to the `@profile/` form first.
  // They resolve to exactly the directories they did before, so the live
  // profile is unchanged in everything but wording — and the copy that
  // travels now finds its keys at whatever root it is opened under.
  auto stored = SettingsObject("key_database_list");
  auto list = KeyDatabaseListSO(stored);
  const auto packed =
      RewriteKeyDatabaseListForPacking(list.key_databases, profile.Root());
  if (packed.size() == list.key_databases.size()) {
    for (int i = 0; i < packed.size(); ++i) {
      if (packed.at(i).path != list.key_databases.at(i).path) {
        list.key_databases = packed;
        stored.Store(list.ToJson());
        break;
      }
    }
  }

  const auto& marker = session.Marker();

  ProfileExportRequest request;
  request.profile_root = profile.Root();
  request.profiles_root = qApp->property("GFProfilesRoot").toString();
  request.dest_path = dialog.DestinationPath();
  request.include_workspace = dialog.IncludeWorkspace();
  request.protection = dialog.Protection();
  request.passphrase = dialog.Passphrase();

  // Read here rather than inside the packing: the key manager and QSettings
  // both belong to this thread, and the packing does not run on it.
  request.app_key = session.Keys().RootKey();
  auto settings = GetSettings();
  request.settings = SnapshotSettings(settings);

  request.manifest.schema_version = marker.schema_version > 0
                                        ? marker.schema_version
                                        : GetAppProfileSchemaVersion();
  request.manifest.min_reader_version = marker.min_reader_version > 0
                                            ? marker.min_reader_version
                                            : GetAppProfileSchemaVersion();
  request.manifest.app_profile = GetAppProfileName();
  request.manifest.display_name = CurrentProfileDisplayName();
  request.manifest.profile_id = profile.Id();
  request.manifest.self_contained = profile.Policy().self_contained;
  request.manifest.key_databases = DescribeKeyDatabasesForManifest(packed);

  if (request.app_key.Empty()) {
    QMessageBox::critical(
        this, tr("Cannot Export Profile"),
        tr("The application key is not available, so the profile could not be "
           "packed."),
        QMessageBox::Ok);
    return;
  }

  auto result = std::make_shared<ProfilePackageWriteResult>();
  GpgOperaHelper::WaitForOpera(
      this, tr("Exporting Profile"), [=](const OperaWaitingHd& op_hd) {
        RunOperaAsync(
            [=](const DataObjectPtr&) -> GFError {
              *result = ExportProfilePackage(request);
              return result->ok ? 0 : -1;
            },
            [=](GFError, const DataObjectPtr&) {
              op_hd();

              if (!result->ok) {
                QMessageBox::critical(this, tr("Cannot Export Profile"),
                                      result->error, QMessageBox::Ok);
                return;
              }

              QMessageBox::information(
                  this, tr("Profile Exported"),
                  tr("\"%1\" was written to:")
                          .arg(request.manifest.display_name) +
                      "\n" + QDir::toNativeSeparators(request.dest_path) +
                      "\n\n" +
                      (request.protection == ProfilePackageProtection::kPIN
                           ? tr("It can only be opened with the passphrase you "
                                "chose. There is no way to recover it.")
                           : tr("It is not protected: anyone who gets this "
                                "file can read the keys inside it.")),
                  QMessageBox::Ok);
            },
            "export_profile_package");
      });
}

auto ProfileManagerDialog::ask_import_name(const QString& suggestion)
    -> QString {
  bool accepted = false;
  const auto name = QInputDialog::getText(
      this, tr("Name This Profile"),
      tr("What should this profile be called on this computer?"),
      QLineEdit::Normal, suggestion, &accepted);
  if (!accepted) return {};

  return name.trimmed().isEmpty() ? suggestion : name.trimmed();
}

void ProfileManagerDialog::slot_import() {
  const auto path = QFileDialog::getOpenFileName(
      this, tr("Import Profile"), GetDefaultUserFilePath(),
      tr("GpgFrontend Profile") + " (*.gfprofile)");
  if (path.isEmpty()) return;

  // The header is read first because it is cheap and says whether a passphrase
  // is needed at all — asking for one before knowing that would be a question
  // with no right answer.
  const auto inspection = InspectProfilePackage(path);
  if (!inspection.Ok()) {
    QMessageBox::critical(this, tr("Cannot Import Profile"), inspection.detail,
                          QMessageBox::Ok);
    return;
  }

  GFBuffer passphrase;
  if (inspection.header.protection == ProfilePackageProtection::kPIN) {
    bool accepted = false;
    auto entered = QInputDialog::getText(
        this, tr("Import Profile"),
        tr("Enter the passphrase that protects this file:"),
        QLineEdit::Password, {}, &accepted);
    if (!accepted || entered.isEmpty()) return;

    passphrase = GFBuffer(entered);
    entered.fill('X');
    entered.clear();
  }

  const auto roots = CurrentProfileRoots();
  const auto staging =
      MakeProfilePackageScratchDir(roots.profiles_root, "extract");
  if (staging.isEmpty()) {
    QMessageBox::critical(this, tr("Cannot Import Profile"),
                          tr("A temporary folder could not be made."),
                          QMessageBox::Ok);
    return;
  }

  auto result = std::make_shared<ProfilePackageReadResult>();
  GpgOperaHelper::WaitForOpera(
      this, tr("Reading Profile"), [=](const OperaWaitingHd& op_hd) {
        RunOperaAsync(
            [=](const DataObjectPtr&) -> GFError {
              *result = ReadProfilePackage(path, staging, passphrase);
              return result->Ok() ? 0 : -1;
            },
            [=](GFError, const DataObjectPtr&) {
              op_hd();
              finish_import(path, staging, *result);
            },
            "read_profile_package");
      });
}

void ProfileManagerDialog::finish_import(
    const QString& package_path, const QString& staging_dir,
    const ProfilePackageReadResult& result) {
  // Whatever happens below, the extracted tree does not outlive this call: it
  // holds an unprotected copy of the package's application key.
  struct ScratchGuard {
    QString path;
    ~ScratchGuard() {
      if (!path.isEmpty()) QDir(path).removeRecursively();
    }
  } const guard{staging_dir};

  if (!result.Ok()) {
    const auto title = result.status == ProfilePackageReadStatus::kTAMPERED
                           ? tr("This File Has Been Altered")
                           : tr("Cannot Import Profile");
    QMessageBox::critical(
        this, title,
        result.detail + "\n\n" + QDir::toNativeSeparators(package_path),
        QMessageBox::Ok);
    return;
  }

  // A package written by a newer build may describe a layout this one cannot
  // read; checked before anything is adopted rather than after.
  ProfileMarker as_marker;
  as_marker.schema_version = result.manifest.schema_version;
  as_marker.min_reader_version = result.manifest.min_reader_version;
  as_marker.profile = result.manifest.app_profile;
  as_marker.last_writer_version = result.manifest.writer_version;

  if (CheckProfileCompatibility(as_marker, true,
                                GetAppProfileSchemaVersion()) ==
      ProfileCompatibility::kTOO_NEW) {
    QMessageBox::critical(
        this, tr("Cannot Import Profile"),
        tr("This profile was made by a newer version of GpgFrontend (%1).")
            .arg(result.manifest.writer_version),
        QMessageBox::Ok);
    return;
  }

  const auto name = ask_import_name(result.manifest.display_name.isEmpty()
                                        ? result.manifest.profile_id
                                        : result.manifest.display_name);
  if (name.isEmpty()) return;

  // The folder is named by a fresh id, not by the name: the id is a directory
  // name and an identity, and neither should have to survive being typed.
  const auto id =
      QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');

  const auto roots = CurrentProfileRoots();
  const auto error = AdoptExtractedProfile(
      staging_dir, roots.profiles_root + "/" + id, id, name, result.manifest);
  if (!error.isEmpty()) {
    QMessageBox::critical(this, tr("Cannot Import Profile"), error,
                          QMessageBox::Ok);
    return;
  }

  reload();

  auto message = tr("\"%1\" is ready.").arg(name);
  if (!result.manifest.workspace_included) {
    message += "\n\n" + tr("The file did not carry any workspace files.");
  }
  for (const auto& database : result.manifest.key_databases) {
    if (!database.external) continue;
    message += "\n\n" +
               tr("\"%1\" pointed at keys kept outside the profile, which do "
                  "not travel. It will show as unavailable until you point it "
                  "somewhere on this computer.")
                   .arg(database.name);
    break;
  }

  if (QMessageBox::question(
          this, tr("Profile Imported"),
          message + "\n\n" + tr("Open it now? It opens in a new window."),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::Yes) != QMessageBox::Yes) {
    return;
  }

  const auto opened = OpenProfileInNewWindow({.profile_id = id});
  if (!opened.Ok()) {
    QMessageBox::warning(this, tr("Cannot Open Profile"), opened.detail,
                         QMessageBox::Ok);
    return;
  }
  accept();
}

void ProfileManagerDialog::slot_create() {
  ProfileCreateDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) return;

  const auto id =
      QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');

  const auto result =
      CreateProfile(CurrentProfileRoots().profiles_root, id,
                    dialog.DisplayName(), dialog.SelfContained());

  if (!result.Ok()) {
    QMessageBox::critical(
        this, tr("Cannot Create Profile"),
        tr("The profile could not be created.") + "\n\n" + result.detail,
        QMessageBox::Ok);
    return;
  }

  reload();

  if (QMessageBox::question(this, tr("Profile Created"),
                            tr("\"%1\" is ready.").arg(dialog.DisplayName()) +
                                "\n\n" +
                                tr("Open it now? It opens in a new window."),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::Yes) != QMessageBox::Yes) {
    return;
  }

  const auto opened = OpenProfileInNewWindow({.profile_id = id});
  if (!opened.Ok()) {
    QMessageBox::warning(this, tr("Cannot Open Profile"), opened.detail,
                         QMessageBox::Ok);
    return;
  }

  accept();
}

void ProfileManagerDialog::slot_delete() {
  const auto entry = selected();
  if (!entry.has_value() || entry->implicit) return;

  // Deliberately two confirmations. Everything the profile's key encrypted
  // becomes permanently unreadable, and there is no undo anywhere below this.
  if (QMessageBox::warning(
          this, tr("Delete Profile"),
          tr("Delete \"%1\"?").arg(entry->name) + "\n\n" +
              tr("Its keys, settings and saved state are removed from this "
                 "computer permanently."),
          QMessageBox::Cancel | QMessageBox::Yes,
          QMessageBox::Cancel) != QMessageBox::Yes) {
    return;
  }
  if (QMessageBox::warning(
          this, tr("Delete Profile"),
          tr("This cannot be undone. Anything stored only in \"%1\" will be "
             "lost.")
              .arg(entry->name),
          QMessageBox::Cancel | QMessageBox::Yes,
          QMessageBox::Cancel) != QMessageBox::Yes) {
    return;
  }

  if (QFileInfo::exists(ProfileLock::PathFor(entry->root))) {
    QMessageBox::warning(this, tr("Profile Is Open"),
                         tr("\"%1\" is open in another window. Close it first.")
                             .arg(entry->name),
                         QMessageBox::Ok);
    return;
  }

  // The credential entry belongs to the profile and would otherwise be orphaned
  // in the store forever, with no way left to tell what it protected.
  if (auto* store = GetSystemSecretStore(); store != nullptr) {
    const auto marker = ReadProfileMarker(ProfileMarkerPathFor(entry->root));
    const auto account = DeriveAppKeyWrapAccount(
        entry->kind, entry->id, QDir(entry->root).canonicalPath(),
        marker.has_value() ? marker->profile_uuid : QString{});
    if (!account.isEmpty()) store->Remove(account);
  }

  if (!DeleteProfile(CurrentProfileRoots().profiles_root, entry->id)) {
    QMessageBox::critical(
        this, tr("Cannot Delete Profile"),
        tr("The profile folder could not be removed:") + "\n" + entry->root,
        QMessageBox::Ok);
  }

  reload();
}

void ProfileManagerDialog::slot_reveal() {
  const auto entry = selected();
  if (!entry.has_value()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(entry->root));
}

}  // namespace GpgFrontend::UI
