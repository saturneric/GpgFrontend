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

#include "model/ProfilesController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QStandardPaths>
#include <QUuid>
#include <limits>

#include "core/GFConstants.h"
#include "core/model/GFBuffer.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileRegistry.h"
#include "core/profile/ProfileSession.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/BuildInfoUtils.h"
#include "files/FileOperations.h"
#include "files/StagingArea.h"
#include "model/ChangeRelay.h"
#include "shell/rules/ProfileNameRules.h"

namespace GpgFrontend::Mobile {

namespace {

/// The id the installed root goes by (ResolveProfileSelection()).
constexpr auto kInstalledRootId = "classic";

std::function<bool()> g_restarter;

auto InstalledRoot() -> QString {
  return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

auto ProfilesRoot() -> QString {
  const auto root =
      QCoreApplication::instance()->property("GFProfilesRoot").toString();
  return root.isEmpty() ? InstalledRoot() + "/profiles" : root;
}

auto ScratchDirectory() -> QString {
  return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
         "/profile-packages";
}

/// Where packages are kept while they are written or read: never the
/// profile's own folders, and gone once the work is done.
auto ScratchFile(const QString& suffix) -> QString {
  const auto dir = ScratchDirectory();
  QDir().mkpath(dir);
  return dir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) +
         suffix;
}

/// A package in the cache, removed once nothing holds it any more: the
/// page that asked for it, or the work still writing or reading it. A page
/// left half way leaves no package behind (one may hold a profile's keys).
struct ScratchGuard {
  QString path;
  explicit ScratchGuard(QString file) : path(std::move(file)) {}
  ScratchGuard(const ScratchGuard&) = delete;
  auto operator=(const ScratchGuard&) -> ScratchGuard& = delete;
  ~ScratchGuard() { QFile::remove(path); }
};
using Scratch = QSharedPointer<ScratchGuard>;

/// Runs @p work on a worker (never the I/O runner, which the package code
/// may not run on), then @p done here.
void OffThread(QObject* context, std::function<void()> work,
               std::function<void()> done) {
  QPointer<QObject> guard(context);
  auto* task = new Thread::Task(
      [work = std::move(work)](const DataObjectPtr&) -> int {
        work();
        return 0;
      },
      "mobile_profile_package", TransferParams(),
      [guard, done = std::move(done)](int, const DataObjectPtr&) {
        if (guard != nullptr) done();
      });
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);
}

auto IsInstalledRoot(const ProfileRegistryEntry& entry) -> bool {
  return entry.kind == ProfileKind::kINSTALLED_ROOT;
}

auto ProfileName(const ProfileRegistryEntry& entry) -> QString {
  // The desktop's name for it (CurrentProfileDisplayName()).
  if (IsInstalledRoot(entry)) return QObject::tr("Default");
  return entry.name;
}

auto Registry() -> ProfileRegistryData {
  return LoadProfileRegistry(ProfilesRoot(), InstalledRoot());
}

/// What the profiles are called now. Read on the UI thread: the profiles
/// root is the application object's property.
auto TakenNames() -> QStringList {
  QStringList names;
  for (const auto& entry : Registry().profiles) names << ProfileName(entry);
  return names;
}

/// @p name, or the first of "name (2)", "name (3)" ... not in @p names.
auto FreeName(const QString& name, const QStringList& names) -> QString {
  const auto taken = [&names](const QString& candidate) {
    return names.contains(candidate, Qt::CaseInsensitive);
  };
  if (!taken(name)) return name;
  for (int n = 2;; ++n) {
    const auto candidate = QStringLiteral("%1 (%2)").arg(name).arg(n);
    if (!taken(candidate)) return candidate;
  }
}

}  // namespace

/// The import under way: the package's copy here, and what it said.
struct ProfilesController::Pending {
  Scratch scratch;  ///< the copy in the cache
  QString package;  ///< its path
  bool sealed = false;
};

void ProfilesController::RemoveLeftPackages() {
  QDir(ScratchDirectory()).removeRecursively();
}

ProfilesController::ProfilesController(QObject* parent) : QObject(parent) {
  // The default profile's name is a translated one.
  connect(&LanguageRelay(), &ChangeRelay::SignalChanged, this,
          &ProfilesController::SignalChanged);
}

ProfilesController::~ProfilesController() = default;

auto ProfilesController::Profiles() const -> QVariantList {
  if (!ProfileSession::Loaded()) return {};
  const auto current = ProfileSession::Instance().Profile().Id();
  QVariantList list;
  for (const auto& entry : Registry().profiles) {
    // A phone has the installed root alone; the portable one is the
    // desktop's.
    if (entry.implicit && !IsInstalledRoot(entry)) continue;
    list.append(QVariantMap{
        {QStringLiteral("id"), entry.id},
        {QStringLiteral("name"), ProfileName(entry)},
        {QStringLiteral("current"), entry.id == current},
    });
  }
  return list;
}

auto ProfilesController::CurrentName() const -> QString {
  if (!ProfileSession::Loaded()) return {};
  const auto& session = ProfileSession::Instance();
  if (session.Profile().Kind() == ProfileKind::kINSTALLED_ROOT) {
    return tr("Default");
  }
  const auto& name = session.Marker().display_name;
  return name.isEmpty() ? session.Profile().DisplayName() : name;
}

auto ProfilesController::Busy() const -> bool { return busy_; }

auto ProfilesController::MinPassphraseLength() -> int {
  return kMinSecretLength;
}

QString ProfilesController::nameProblem(const QString& name) const {
  QStringList taken;
  for (const auto& entry : Registry().profiles) taken << ProfileName(entry);
  switch (Shell::CheckProfileName(name, taken)) {
    case Shell::ProfileNameProblem::kNone:
      return {};
    case Shell::ProfileNameProblem::kEmpty:
      return tr("Give the profile a name.");
    case Shell::ProfileNameProblem::kTooLong:
      return tr("Use at most %1 characters.")
          .arg(Shell::kMaxProfileNameLength);
    case Shell::ProfileNameProblem::kTaken:
      return tr("A profile with this name already exists.");
  }
  return {};
}

QString ProfilesController::create(const QString& name) {
  if (const auto problem = nameProblem(name); !problem.isEmpty()) {
    emit SignalFailed(problem);
    return {};
  }
  const auto root = ProfilesRoot();
  const auto id = MintProfileDirectoryId(root);
  if (id.isEmpty()) {
    emit SignalFailed(tr("The profile could not be created."));
    return {};
  }
  // Keys of its own: what one makes a second profile for.
  const auto result = CreateProfile(root, id, name.trimmed(), true);
  if (!result.Ok()) {
    emit SignalFailed(tr("The profile could not be created.") + "\n\n" +
                      result.detail);
    return {};
  }
  emit SignalChanged();
  return id;
}

QString ProfilesController::exportFileName() const {
  // A file name every provider takes.
  auto name = CurrentName();
  for (auto& c : name) {
    if (!c.isLetterOrNumber() && c != u' ' && c != u'-' && c != u'_') c = u'_';
  }
  name = name.trimmed();
  if (name.isEmpty()) name = QStringLiteral("profile");
  return name + QStringLiteral(".gfp");
}

void ProfilesController::exportTo(const QUrl& url, const QString& passphrase) {
  // Said, not dropped: the page waits for the answer.
  if (busy_ || !ProfileSession::Loaded()) {
    emit SignalExported(
        false, tr("Another profile package is still being written or read."));
    return;
  }
  if (passphrase.size() < kMinSecretLength) {
    emit SignalExported(false, tr("Use a passphrase of at least %1 characters.")
                                   .arg(kMinSecretLength));
    return;
  }

  // Gathered here, on the thread that owns the key and the settings.
  auto request = GatherOpenProfileExport(CurrentName());
  if (request.secure_members.isEmpty()) {
    emit SignalExported(false, tr("The application key is not available, so "
                                  "the profile could not be packed."));
    return;
  }
  // Written here first, then copied where the picker said: the package code
  // writes to a path, and the document is a content:// one.
  request.dest_path = ScratchFile(QStringLiteral(".gfp"));
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(passphrase);
  request.include_workspace = false;

  set_busy(true);
  const auto written = request.dest_path;
  // Held by each step: gone with the last one, whether or not this page
  // is still there to hear how it went.
  const auto scratch = Scratch::create(written);
  auto result = QSharedPointer<ProfilePackageWriteResult>::create();
  OffThread(
      this,
      [request = std::move(request), result, scratch]() mutable {
        *result = ExportProfilePackage(std::move(request));
      },
      [this, result, written, url, scratch]() {
        if (!result->ok) {
          set_busy(false);
          emit SignalExported(false, result->error);
          return;
        }
        CopyOffThread(written, LocalOrContent(url),
                      std::numeric_limits<qint64>::max(), this,
                      [this, scratch](bool copied) {
                        set_busy(false);
                        emit SignalExported(
                            copied, copied ? QString{}
                                           : tr("The file could not be saved "
                                                "there."));
                      });
      });
}

void ProfilesController::importFrom(const QUrl& url) {
  if (busy_ || !ProfileSession::Loaded()) {
    emit SignalImported(
        false, tr("Another profile package is still being written or read."),
        {}, {});
    return;
  }
  pending_ = std::make_unique<Pending>();
  pending_->package = ScratchFile(QStringLiteral(".gfp"));
  pending_->scratch = Scratch::create(pending_->package);
  set_busy(true);
  // Copied in first: a document may be read only for a while, and the
  // package code reads a path. The copy holds it too: a page left while it
  // is written does not leave it behind.
  CopyOffThread(LocalOrContent(url), pending_->package,
                std::numeric_limits<qint64>::max(), this,
                [this, scratch = pending_->scratch](bool copied) {
                  if (!copied || pending_ == nullptr) {
                    finish_import(false, tr("The file could not be read."));
                    return;
                  }
                  // The header alone: cheap, and it says whether a
                  // passphrase is needed.
                  const auto inspection =
                      InspectProfilePackage(pending_->package);
                  if (!inspection.Ok()) {
                    finish_import(false, inspection.detail);
                    return;
                  }
                  pending_->sealed = inspection.header.protection ==
                                     ProfilePackageProtection::kPIN;
                  if (pending_->sealed) {
                    set_busy(false);
                    emit SignalPassphraseNeeded({});
                    return;
                  }
                  read_package({});
                });
}

void ProfilesController::importWith(const QString& passphrase) {
  if (busy_ || pending_ == nullptr) {
    emit SignalImported(
        false, tr("Another profile package is still being written or read."),
        {}, {});
    return;
  }
  set_busy(true);
  read_package(passphrase);
}

void ProfilesController::cancelImport() {
  if (busy_) return;
  pending_.reset();
}

void ProfilesController::read_package(const QString& passphrase) {
  struct Outcome {
    ProfilePackageReadResult read;
    QString error;
    QString id;
    QString name;
  };

  const auto root = ProfilesRoot();
  const auto package = pending_->package;
  const auto scratch = pending_->scratch;
  const auto staging = MakeProfilePackageScratchDir(root, "extract");
  if (staging.isEmpty()) {
    finish_import(false, tr("A temporary folder could not be made."));
    return;
  }

  auto outcome = QSharedPointer<Outcome>::create();
  OffThread(
      this,
      [outcome, package, scratch, staging, root, taken = TakenNames(),
       secret = GFBuffer(passphrase)]() {
        // Whatever happens, the extracted tree does not stay behind: it holds
        // the profile's key unprotected.
        struct Cleanup {
          QString path;
          ~Cleanup() { QDir(path).removeRecursively(); }
        } const cleanup{staging};

        outcome->read = ReadProfilePackage(package, staging, secret);
        if (!outcome->read.Ok()) return;

        // A package from a newer build may describe a layout this one cannot
        // read: checked before anything is adopted.
        const auto& manifest = outcome->read.manifest;
        if (IsProfilePackageTooNew(manifest)) {
          outcome->error = QObject::tr(
                               "This profile was made by a newer version of "
                               "GpgFrontend (%1).")
                               .arg(manifest.writer_version);
          return;
        }

        const auto name =
            FreeName(manifest.display_name.isEmpty() ? manifest.profile_id
                                                     : manifest.display_name,
                     taken);
        const auto adopted = AdoptProfilePackage(staging, root, name, manifest);
        outcome->error = adopted.error;
        if (adopted.error.isEmpty()) {
          outcome->id = adopted.id;
          outcome->name = name;
        }
      },
      [this, outcome]() {
        if (outcome->read.status == ProfilePackageReadStatus::kBAD_PASSPHRASE &&
            pending_ != nullptr) {
          // Asked again, the copy kept.
          set_busy(false);
          emit SignalPassphraseNeeded(outcome->read.detail.isEmpty()
                                          ? tr("That passphrase does not "
                                               "open this file.")
                                          : outcome->read.detail);
          return;
        }
        if (!outcome->read.Ok()) {
          finish_import(false, outcome->read.detail);
          return;
        }
        finish_import(outcome->error.isEmpty(), outcome->error, outcome->id,
                      outcome->name);
      });
}

void ProfilesController::finish_import(bool succeeded, const QString& error,
                                       const QString& id, const QString& name) {
  pending_.reset();
  set_busy(false);
  if (succeeded) emit SignalChanged();
  emit SignalImported(succeeded, error, id, name);
}

bool ProfilesController::switchTo(const QString& id) {
  if (busy_ || !ProfileSession::Loaded()) return false;
  bool known = false;
  for (const auto& entry : Registry().profiles) {
    known |= entry.id == id && (!entry.implicit || IsInstalledRoot(entry));
  }
  if (!known) {
    emit SignalFailed(tr("There is no such profile."));
    return false;
  }
  if (id == ProfileSession::Instance().Profile().Id()) return true;

  const auto file = ChosenProfileFile(InstalledRoot());
  QDir().mkpath(QFileInfo(file).absolutePath());
  QFile out(file);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
      out.write(id.toUtf8()) != id.toUtf8().size()) {
    emit SignalFailed(tr("The choice could not be kept."));
    return false;
  }
  out.close();

  if (!g_restarter || !g_restarter()) {
    emit SignalFailed(
        tr("GpgFrontend could not start again. It opens this "
           "profile the next time it starts."));
    return false;
  }
  return true;
}

auto ProfilesController::ApplyChosenProfile(const QString& installed_root)
    -> bool {
  QFile in(ChosenProfileFile(installed_root));
  if (!in.open(QIODevice::ReadOnly)) return false;
  const auto id = QString::fromUtf8(in.read(256)).trimmed();
  in.close();

  if (id.isEmpty() || id == QLatin1StringView(kInstalledRootId)) {
    qunsetenv("GF_PROFILE");
    return false;
  }
  for (const auto& entry : ScanProfilesRoot(installed_root + "/profiles")) {
    if (entry.id == id) {
      qputenv("GF_PROFILE", id.toUtf8());
      return true;
    }
  }
  // Gone since: the default opens, and stays the choice.
  QFile::remove(in.fileName());
  return false;
}

auto ProfilesController::OpenDefaultProfile(const QString& installed_root)
    -> bool {
  const auto file = ChosenProfileFile(installed_root);
  if (QFile::exists(file) && !QFile::remove(file)) return false;
  return g_restarter && g_restarter();
}

auto ProfilesController::ChosenProfileFile(const QString& installed_root)
    -> QString {
  // Dot-named: never taken for a profile, and not inside any one of them.
  return installed_root + "/profiles/.chosen-profile";
}

void ProfilesController::SetRestarter(std::function<bool()> restarter) {
  g_restarter = std::move(restarter);
}

void ProfilesController::set_busy(bool busy) {
  if (busy_ == busy) return;
  busy_ = busy;
  emit SignalBusyChanged();
}

}  // namespace GpgFrontend::Mobile
