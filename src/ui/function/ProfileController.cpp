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

#include "ui/function/ProfileController.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QUuid>
#include <algorithm>

#include "core/function/GlobalSettingStation.h"
#include "core/model/SettingsObject.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "ui/dialog/AppKeyPinDialog.h"
#include "ui/dialog/SecretPrompt.h"
#include "ui/dialog/profile/ProfileCreateDialog.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/platform/PlatformRelaunch.h"

namespace GpgFrontend::UI {

namespace {

/// Options whose value is a separate argument, so stripping the option also
/// strips what belongs to it.
const QStringList kProfileValueOptions = {"--profile"};

/// Whether "write this session back into its package" has been answered for
/// good. Kept beside the question rather than at each call site: a close and a
/// restart both ask it, a save re-enters through its own completion, and every
/// one of those would otherwise need its own copy of "already asked".
bool g_session_write_back_settled = false;  // NOLINT(*-non-const-global-*)

/**
 * @brief Start a second instance of this application, detached.
 *
 * Detached on purpose: the new window outlives whichever window opened it, and
 * neither is the other's parent in any sense that matters.
 *
 * @param args arguments for the new instance, argv[0] excluded
 * @return kSTARTED when the process was launched
 */
auto Launch(const QStringList& args) -> ProfileLaunchResult {
  ProfileLaunchResult result;
  LOG_I() << "opening a new window with args" << args;

#ifdef Q_OS_MACOS
  // A bundled application is launched through the workspace, not by running the
  // executable: that is what gives the new instance its own Dock entry and
  // activation, and it is already how the deep restart comes back.
  if (!RelaunchApplication(args)) {
    result.status = ProfileLaunchStatus::kFAILED;
    result.detail = QObject::tr("The new window could not be started.");
  }
#else
  if (!QProcess::startDetached(QCoreApplication::applicationFilePath(), args)) {
    result.status = ProfileLaunchStatus::kFAILED;
    result.detail = QObject::tr("The new window could not be started.");
  }
#endif
  return result;
}

auto AskImportName(QWidget* parent, const QString& suggestion) -> QString {
  bool accepted = false;
  const auto name = QInputDialog::getText(
      parent, QObject::tr("Name This Profile"),
      QObject::tr("What should this profile be called on this computer?"),
      QLineEdit::Normal, suggestion, &accepted);
  if (!accepted) return {};

  return name.trimmed().isEmpty() ? suggestion : name.trimmed();
}

void FinishImport(QWidget* parent, const QString& package_path,
                  const QString& staging_dir,
                  const ProfilePackageReadResult& result,
                  const std::function<void()>& on_changed,
                  const std::function<void()>& on_opened) {
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
                           ? QObject::tr("This File Has Been Altered")
                           : QObject::tr("Cannot Import Profile");
    QMessageBox::critical(
        parent, title,
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
        parent, QObject::tr("Cannot Import Profile"),
        QObject::tr(
            "This profile was made by a newer version of GpgFrontend (%1).")
            .arg(result.manifest.writer_version),
        QMessageBox::Ok);
    return;
  }

  const auto name = AskImportName(parent, result.manifest.display_name.isEmpty()
                                              ? result.manifest.profile_id
                                              : result.manifest.display_name);
  if (name.isEmpty()) return;

  // The folder is named by a fresh id, not by the name: the id is a directory
  // name and an identity, and neither should have to survive being typed.
  const auto roots = CurrentProfileRoots();
  const auto id = MintProfileDirectoryId(roots.profiles_root);
  if (id.isEmpty()) {
    QMessageBox::critical(parent, QObject::tr("Cannot Import Profile"),
                          QObject::tr("The profile could not be created."),
                          QMessageBox::Ok);
    return;
  }

  const auto error = AdoptExtractedProfile(
      staging_dir, roots.profiles_root + "/" + id, id, name, result.manifest);
  if (!error.isEmpty()) {
    QMessageBox::critical(parent, QObject::tr("Cannot Import Profile"), error,
                          QMessageBox::Ok);
    return;
  }

  if (on_changed) on_changed();

  auto message = QObject::tr("\"%1\" is ready.").arg(name);
  if (!result.manifest.workspace_included) {
    message +=
        "\n\n" + QObject::tr("The file did not carry any workspace files.");
  }
  for (const auto& database : result.manifest.key_databases) {
    if (!database.external) continue;
    message += "\n\n" +
               QObject::tr(
                   "\"%1\" pointed at keys kept outside the profile, which do "
                   "not travel. It will show as unavailable until you point it "
                   "somewhere on this computer.")
                   .arg(database.name);
    break;
  }

  if (QMessageBox::question(
          parent, QObject::tr("Profile Imported"),
          message + "\n\n" +
              QObject::tr("Open it now? It opens in a new window."),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::Yes) != QMessageBox::Yes) {
    return;
  }

  const auto opened = OpenProfileInNewWindow({.profile_id = id});
  if (!opened.Ok()) {
    QMessageBox::warning(parent, QObject::tr("Cannot Open Profile"),
                         opened.detail, QMessageBox::Ok);
    return;
  }
  if (on_opened) on_opened();
}

}  // namespace

auto StripProfileArgs(const QStringList& args) -> QStringList {
  QStringList out;
  if (args.isEmpty()) return out;

  out << args.first();
  for (int i = 1; i < args.size(); ++i) {
    const auto& arg = args.at(i);

    if (kProfileValueOptions.contains(arg)) {
      ++i;  // and its value
      continue;
    }
    if (arg.startsWith("--profile=")) {
      continue;
    }
    // a package named on the command line selected the *previous* profile; a
    // switch away from it must not silently re-open it
    if (!arg.startsWith('-') &&
        arg.endsWith(kProfilePackageExtension, Qt::CaseInsensitive)) {
      continue;
    }
    out << arg;
  }
  return out;
}

auto DescribeUnverifiedHeader(const ProfilePackageHeader& header) -> QString {
  QStringList said;
  if (!header.created.isEmpty()) {
    said << QObject::tr("made %1").arg(header.created);
  }
  if (!header.writer.isEmpty()) {
    said << (header.writer_stable
                 ? QObject::tr("by GpgFrontend %1").arg(header.writer)
                 : QObject::tr("by a development build of GpgFrontend %1")
                       .arg(header.writer));
  }
  if (said.isEmpty()) return {};

  // The caveat is not optional decoration on the claim, it is the reason the
  // claim is shown at all. Never returned without it.
  return QObject::tr("The file says it was %1.").arg(said.join(" ")) + "\n" +
         QObject::tr(
             "That comes from the file's unencrypted header, which anyone "
             "holding the file can change. Nothing is confirmed until the "
             "passphrase opens it.");
}

auto ProfilePackageNameFilter() -> QString {
  return QObject::tr("GpgFrontend Profile File") + " (*" +
         QLatin1String(kProfilePackageExtension) + ")";
}

auto BuildLaunchArgs(const QStringList& args, const ProfileTarget& target)
    -> QStringList {
  auto stripped = StripProfileArgs(args);
  if (!stripped.isEmpty()) stripped.removeFirst();  // argv[0]

  if (target.IsPackage()) {
    stripped << target.package_path;
  } else if (!target.profile_id.isEmpty() && target.profile_id != "classic" &&
             target.profile_id != "portable") {
    // The two implicit profiles are what the resolver picks when nothing is
    // named, so naming them would only pin a decision that is already made.
    stripped << "--profile" << target.profile_id;
  }
  return stripped;
}

auto CurrentProfilesRoot() -> QString {
  // Mirrored onto the application by the context: which root holds this
  // machine's persisted profiles is a property of the installation, and a
  // packaged profile has no opinion about it at all.
  return qApp->property("GFProfilesRoot").toString();
}

auto CurrentProfileRoots() -> ProfileRoots {
  const auto& profile = ProfileSession::Instance().Profile();

  ProfileRoots roots;
  roots.profiles_root = CurrentProfilesRoot();
  roots.classic_root =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

  // Only offer the portable entry where one actually exists. Listing it on an
  // installed copy would show a profile the user can never open.
  if (profile.Kind() == ProfileKind::kPORTABLE_ROOT ||
      roots.profiles_root.startsWith(ResolvePortableDataPath())) {
    roots.portable_root = ResolvePortableDataPath();
  }
  return roots;
}

auto LoadProfiles() -> ProfileRegistryData {
  const auto roots = CurrentProfileRoots();
  auto data = LoadProfileRegistry(roots.profiles_root, roots.classic_root,
                                  roots.portable_root);

  // The profile this window is using is always in the list, even when it is an
  // ad-hoc root named on the command line that the registry has never heard
  // of. A list of profiles that leaves out the one in front of the user
  // invites them to read some other row as the one they are in.
  const auto& profile = ProfileSession::Instance().Profile();
  if (!data.Find(profile.Id()).has_value()) {
    ProfileRegistryEntry entry;
    entry.id = profile.Id();
    entry.root = profile.Root();
    entry.name = CurrentProfileDisplayName();
    entry.kind = profile.Kind();
    entry.implicit = true;  // nothing to persist, and nothing to delete
    data.profiles.prepend(entry);
  }
  return data;
}

auto ProfileKindDisplayName(ProfileKind kind) -> QString {
  switch (kind) {
    case ProfileKind::kINSTALLED_ROOT:
      return QObject::tr("Default");
    case ProfileKind::kPORTABLE_ROOT:
      return QObject::tr("Portable");
    case ProfileKind::kPACKAGED:
      return QObject::tr("From a package");
    default:
      return QObject::tr("Local");
  }
}

auto CurrentProfileDisplayName() -> QString {
  const auto& session = ProfileSession::Instance();

  switch (session.Profile().Kind()) {
    case ProfileKind::kINSTALLED_ROOT:
      return QObject::tr("Default");
    case ProfileKind::kPORTABLE_ROOT:
      return QObject::tr("Portable");
    default:
      break;
  }

  // Read from the profile's own marker rather than the registry: the marker
  // travels with the profile and the session already holds it, while the
  // registry is a machine-local index that a read here would also reconcile.
  if (const auto& name = session.Marker().display_name; !name.isEmpty()) {
    return name;
  }
  return session.Profile().DisplayName();
}

auto ProfileTargetLockRoot(const ProfileTarget& target) -> QString {
  if (target.IsPackage()) {
    return ProfileSessionRoot(CurrentProfilesRoot(), target.package_path);
  }

  const auto roots = CurrentProfileRoots();
  const auto data = LoadProfileRegistry(roots.profiles_root, roots.classic_root,
                                        roots.portable_root);
  const auto entry = data.Find(target.profile_id);
  return entry.has_value() ? entry->root : QString{};
}

auto OpenProfileInNewWindow(const ProfileTarget& target)
    -> ProfileLaunchResult {
  ProfileLaunchResult result;

  auto resolved = target;
  QString name;

  if (target.IsPackage()) {
    if (!QFileInfo::exists(target.package_path)) {
      result.status = ProfileLaunchStatus::kNOT_FOUND;
      result.detail = QObject::tr("This file is no longer there:") + "\n" +
                      QDir::toNativeSeparators(target.package_path);
      return result;
    }
    resolved.package_path = QFileInfo(target.package_path).absoluteFilePath();
    name = QFileInfo(resolved.package_path).fileName();
  } else {
    if (target.profile_id == ProfileSession::Instance().Profile().Id()) {
      result.status = ProfileLaunchStatus::kALREADY_OPEN;
      result.detail = QObject::tr("This window is already using that profile.");
      return result;
    }

    const auto roots = CurrentProfileRoots();
    const auto data = LoadProfileRegistry(
        roots.profiles_root, roots.classic_root, roots.portable_root);
    const auto entry = data.Find(target.profile_id);
    if (!entry.has_value()) {
      result.status = ProfileLaunchStatus::kNOT_FOUND;
      result.detail = QObject::tr("There is no profile called \"%1\".")
                          .arg(target.profile_id);
      return result;
    }
    name = entry->name;
    // Nothing is recorded here: the process about to start stamps its own
    // marker once it has actually opened the profile, which is the only moment
    // at which "was opened" is true.
  }

  // Asked before the process is launched, so a root somebody already has is a
  // message here rather than a window that appears and immediately refuses. A
  // package is asked about in exactly the same way: its session root is derived
  // from its path, so both windows work out the same directory.
  const auto lock = ProfileLock::Probe(ProfileTargetLockRoot(resolved));
  if (lock.status == ProfileLockStatus::kHELD_ELSEWHERE) {
    result.status = ProfileLaunchStatus::kALREADY_OPEN;
    result.detail =
        lock.pid > 0
            ? QObject::tr(
                  "\"%1\" is open in another window (process %2 on %3).")
                  .arg(name)
                  .arg(lock.pid)
                  .arg(lock.host)
            : QObject::tr("\"%1\" is open in another window.").arg(name);
    return result;
  }

  return Launch(BuildLaunchArgs(qApp->arguments(), resolved));
}

auto IsCurrentPackageSession(const QString& path) -> bool {
  if (path.isEmpty()) return false;

  const auto* packaged = dynamic_cast<const PackagedProfile*>(
      &ProfileSession::Instance().Profile());
  if (packaged == nullptr) return false;

  // Both sides through QFileInfo, because one of these came from a file manager
  // and the other from this process's own command line, and the two spell the
  // same file differently often enough to matter.
  return QFileInfo(packaged->PackagePath()).absoluteFilePath() ==
         QFileInfo(path).absoluteFilePath();
}

auto MaybeWriteBackPackageSession(QWidget* parent,
                                  const std::function<void()>& on_done)
    -> bool {
  const auto& session = ProfileSession::Instance();
  const auto* packaged =
      dynamic_cast<const PackagedProfile*>(&session.Profile());
  if (packaged == nullptr || g_session_write_back_settled) return true;

  const auto file = QDir::toNativeSeparators(packaged->PackagePath());

  QMessageBox box(
      QMessageBox::Question, QObject::tr("Save Changes?"),
      QObject::tr("This profile was opened from a file. It is not kept on this "
                  "computer, and the copy it is running from is about to be "
                  "deleted.") +
          "\n\n" + file + "\n\n" +
          QObject::tr("Anything you changed is lost unless it is written back "
                      "into that file."),
      QMessageBox::NoButton, parent);
  auto* save =
      box.addButton(QObject::tr("Save Changes"), QMessageBox::AcceptRole);
  auto* discard =
      box.addButton(QObject::tr("Discard"), QMessageBox::DestructiveRole);
  box.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
  box.setDefaultButton(save);
  box.exec();

  if (box.clickedButton() == discard) {
    g_session_write_back_settled = true;
    return true;
  }
  // Cancel is not an answer: the next attempt to close asks again.
  if (box.clickedButton() != save) return false;

  auto request = packaged->WriteBackRequest();

  auto passphrase = request.passphrase;
  if (request.protection == ProfilePackageProtection::kPIN &&
      passphrase.Empty()) {
    // Only reachable when this process did not open the package itself — a deep
    // restart comes back on the same file with nothing carried over in memory.
    //
    // Asked for as kSET rather than kUNLOCK, because that is what it is: this
    // prompt does not verify anything against the file, it chooses the
    // passphrase the file is re-sealed with. Without a confirmation field a
    // single typo here seals the file under something nobody knows, including
    // the person who just typed it.
    //
    // The floor is lowered to one character for this prompt alone. A package
    // sealed before the floor existed may use four, and someone re-typing the
    // passphrase their file already has must not be forced to give it a new one
    // just to save their work.
    auto texts = DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage,
                                          SecretPromptMode::kSET,
                                          QFileInfo(file).fileName());
    texts.context_detail =
        QDir::toNativeSeparators(QFileInfo(file).absolutePath());
    texts.min_length = 1;

    AppKeyPinDialog dialog(SecretPromptMode::kSET, texts, parent);
    if (dialog.exec() != QDialog::Accepted) return false;

    passphrase = dialog.Pin();
    if (passphrase.Empty()) return false;
  }
  request.passphrase = passphrase;

  const auto& profile = session.Profile();

  // The stored key database paths are normalised to the `@profile/` form
  // first, exactly as an export does: they resolve to the same directories
  // either way, and the copy that travels then finds its keys wherever it is
  // opened next.
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

  // Read here rather than inside the packing: the key manager and QSettings
  // both belong to this thread, and the packing does not run on it.
  // One call, because which key comes from where is not this file's business
  // to remember: ResolveSecureAreaMembers() is where that lives.
  request.secure_members =
      ResolveSecureAreaMembers(session.Accessor(), session.Keys().RootKey());
  auto settings = GetSettings();
  request.settings = SnapshotSettings(settings);

  request.manifest.schema_version = marker.schema_version > 0
                                        ? marker.schema_version
                                        : GetAppProfileSchemaVersion();
  request.manifest.min_reader_version = marker.min_reader_version > 0
                                            ? marker.min_reader_version
                                            : GetAppProfileMinReaderSchema();
  request.manifest.app_profile = GetAppProfileName();
  request.manifest.display_name = CurrentProfileDisplayName();
  request.manifest.key_databases = DescribeKeyDatabasesForManifest(packed);

  if (request.secure_members.isEmpty()) {
    QMessageBox::critical(
        parent, QObject::tr("Cannot Save Changes"),
        QObject::tr("The application key is not available, so the profile "
                    "could not be packed."),
        QMessageBox::Ok);
    return false;
  }

  auto result = std::make_shared<ProfilePackageWriteResult>();
  GpgOperaHelper::WaitForOpera(
      parent, QObject::tr("Saving Profile"), [=](const OperaWaitingHd& op_hd) {
        RunOperaAsync(
            [=](const DataObjectPtr&) -> GFError {
              *result = ExportProfilePackage(request);
              return result->ok ? 0 : -1;
            },
            [=](GFError, const DataObjectPtr&) {
              op_hd();

              if (!result->ok) {
                // The window stays open: the session is still there, so the
                // changes are still there, and a retry is still possible.
                QMessageBox::critical(
                    parent, QObject::tr("Cannot Save Changes"),
                    result->error + "\n\n" + file, QMessageBox::Ok);
                return;
              }

              // What a package carries is an allow-list, so anything else in
              // the session folder is not saved. Logged rather than shown: the
              // save the user asked for succeeded, and the format leaving out
              // what it never carried is not a failure to interrupt them with
              // on the way out.
              if (!result->skipped.isEmpty()) {
                LOG_W() << "profile package write-back left out non-profile "
                           "members:"
                        << result->skipped.join(", ");
              }

              g_session_write_back_settled = true;
              on_done();
            },
            "write_back_profile_session");
      });

  return false;
}

auto AskProfilePackageAction(QWidget* parent, const QString& path)
    -> ProfilePackageAction {
  const auto file = QDir::toNativeSeparators(path);

  // Both offered rather than one of them guessed: the Profiles menu keeps
  // "Open" and "Import" apart precisely because they are routinely read as the
  // same act, and a double-click says which file is meant, not which act.
  QMessageBox box(
      QMessageBox::Question, QObject::tr("Open Profile File"),
      QObject::tr("This is a profile file.") + "\n\n" + file + "\n\n" +
          QObject::tr("Open it to work in it directly and leave it a file, "
                      "with nothing kept on this computer. Import it to "
                      "copy it into a profile kept here, after which the "
                      "file is not used again."),
      QMessageBox::NoButton, parent);
  auto* open_it = box.addButton(QObject::tr("Open"), QMessageBox::AcceptRole);
  auto* import_it =
      box.addButton(QObject::tr("Import"), QMessageBox::ActionRole);
  box.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
  box.setDefaultButton(open_it);
  box.exec();

  if (box.clickedButton() == open_it) return ProfilePackageAction::kOPEN;
  if (box.clickedButton() == import_it) return ProfilePackageAction::kIMPORT;
  return ProfilePackageAction::kCANCEL;
}

void ImportProfileInteractive(QWidget* parent,
                              const std::function<void()>& on_changed,
                              const std::function<void()>& on_opened) {
  const auto path = QFileDialog::getOpenFileName(
      parent, QObject::tr("Import Profile File"), GetDefaultUserFilePath(),
      ProfilePackageNameFilter());
  if (path.isEmpty()) return;

  ImportProfileInteractive(parent, path, on_changed, on_opened);
}

void ImportProfileInteractive(QWidget* parent, const QString& path,
                              const std::function<void()>& on_changed,
                              const std::function<void()>& on_opened) {
  if (path.isEmpty()) return;

  // The header is read first because it is cheap and says whether a passphrase
  // is needed at all — asking for one before knowing that would be a question
  // with no right answer.
  const auto inspection = InspectProfilePackage(path);
  if (!inspection.Ok()) {
    QMessageBox::critical(parent, QObject::tr("Cannot Import Profile"),
                          inspection.detail, QMessageBox::Ok);
    return;
  }

  GFBuffer passphrase;
  if (inspection.header.protection == ProfilePackageProtection::kPIN) {
    // The same prompt the startup path uses, so a passphrase is asked for the
    // same way wherever a profile file is opened. The header was already read
    // above, so its claims come free — and are shown as claims: it sits outside
    // the sealed payload, where anyone holding the file could have written it.
    const auto entered = AppKeyPinDialog::AskPackagePassphrase(
        parent, path, AppKeyPinDialog::Mode::kUNLOCK, false,
        DescribeUnverifiedHeader(inspection.header));
    if (!entered.has_value()) return;
    passphrase = *entered;
  }

  const auto roots = CurrentProfileRoots();
  const auto staging =
      MakeProfilePackageScratchDir(roots.profiles_root, "extract");
  if (staging.isEmpty()) {
    QMessageBox::critical(parent, QObject::tr("Cannot Import Profile"),
                          QObject::tr("A temporary folder could not be made."),
                          QMessageBox::Ok);
    return;
  }

  auto result = std::make_shared<ProfilePackageReadResult>();
  GpgOperaHelper::WaitForOpera(
      parent, QObject::tr("Reading Profile"), [=](const OperaWaitingHd& op_hd) {
        RunOperaAsync(
            [=](const DataObjectPtr&) -> GFError {
              *result = ReadProfilePackage(path, staging, passphrase);
              return result->Ok() ? 0 : -1;
            },
            [=](GFError, const DataObjectPtr&) {
              op_hd();
              FinishImport(parent, path, staging, *result, on_changed,
                           on_opened);
            },
            "read_profile_package");
      });
}

void CreateProfileInteractive(QWidget* parent,
                              const std::function<void()>& on_changed,
                              const std::function<void()>& on_opened) {
  ProfileCreateDialog dialog(parent);
  if (dialog.exec() != QDialog::Accepted) return;

  const auto profiles_root = CurrentProfileRoots().profiles_root;
  const auto id = MintProfileDirectoryId(profiles_root);
  if (id.isEmpty()) {
    QMessageBox::critical(parent, QObject::tr("Cannot Create Profile"),
                          QObject::tr("The profile could not be created."),
                          QMessageBox::Ok);
    return;
  }

  const auto result = CreateProfile(profiles_root, id, dialog.DisplayName(),
                                    dialog.SelfContained());

  if (!result.Ok()) {
    QMessageBox::critical(parent, QObject::tr("Cannot Create Profile"),
                          QObject::tr("The profile could not be created.") +
                              "\n\n" + result.detail,
                          QMessageBox::Ok);
    return;
  }

  if (on_changed) on_changed();

  if (QMessageBox::question(
          parent, QObject::tr("Profile Created"),
          QObject::tr("\"%1\" is ready.").arg(dialog.DisplayName()) + "\n\n" +
              QObject::tr("Open it now? It opens in a new window."),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::Yes) != QMessageBox::Yes) {
    return;
  }

  const auto opened = OpenProfileInNewWindow({.profile_id = id});
  if (!opened.Ok()) {
    QMessageBox::warning(parent, QObject::tr("Cannot Open Profile"),
                         opened.detail, QMessageBox::Ok);
    return;
  }

  if (on_opened) on_opened();
}

auto RecentProfiles(int limit) -> QList<ProfileRegistryEntry> {
  auto entries = LoadProfiles().profiles;

  const auto current = ProfileSession::Instance().Profile().Id();
  // Not QList::removeIf(): that arrived in Qt 6.1 and this has to build against
  // Qt 5 as well.
  entries.erase(std::remove_if(entries.begin(), entries.end(),
                               [&current](const ProfileRegistryEntry& e) {
                                 // Never offered: the one this window is
                                 // already running, and any profile that has
                                 // never been opened, which has no place in a
                                 // "recent" list.
                                 return e.id == current ||
                                        e.last_opened.isEmpty();
                               }),
                entries.end());

  // ISO-8601 in UTC, so lexicographic order is chronological order.
  std::sort(entries.begin(), entries.end(),
            [](const ProfileRegistryEntry& a, const ProfileRegistryEntry& b) {
              return a.last_opened > b.last_opened;
            });

  if (limit > 0 && entries.size() > limit)
    entries.erase(entries.begin() + limit, entries.end());
  return entries;
}

}  // namespace GpgFrontend::UI
