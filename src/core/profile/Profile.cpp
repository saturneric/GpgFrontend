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

#include "core/profile/Profile.h"

#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProtectedFsProfileAccessor.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

/// The stored tokens. Unchanged from what every existing profile.json holds.
constexpr auto kTokenInstalledRoot = "classic";
constexpr auto kTokenPortableRoot = "portable";
constexpr auto kTokenPersist = "named";
constexpr auto kTokenPackaged = "package_linked";

auto MountStatusFor(ProfilePackageReadStatus status) -> ProfileMountStatus {
  switch (status) {
    case ProfilePackageReadStatus::kOK:
      return ProfileMountStatus::kOK;
    case ProfilePackageReadStatus::kNOT_A_PACKAGE:
      return ProfileMountStatus::kNOT_A_PACKAGE;
    case ProfilePackageReadStatus::kTOO_NEW:
      return ProfileMountStatus::kTOO_NEW;
    case ProfilePackageReadStatus::kBAD_PASSPHRASE:
      return ProfileMountStatus::kBAD_PASSPHRASE;
    case ProfilePackageReadStatus::kTAMPERED:
      return ProfileMountStatus::kTAMPERED;
    case ProfilePackageReadStatus::kMALFORMED:
      return ProfileMountStatus::kMALFORMED;
    case ProfilePackageReadStatus::kNO_SPACE:
      return ProfileMountStatus::kNO_SPACE;
    case ProfilePackageReadStatus::kTOO_LARGE:
      return ProfileMountStatus::kTOO_LARGE;
    case ProfilePackageReadStatus::kIO_FAILED:
      return ProfileMountStatus::kIO_FAILED;
  }
  // Not dead code, and not a safety net either: the trailing return means a
  // status added upstream lands here silently rather than failing to compile,
  // so anything new must be added above deliberately.
  return ProfileMountStatus::kIO_FAILED;
}

}  // namespace

auto ProfileKindToString(ProfileKind kind) -> QString {
  switch (kind) {
    case ProfileKind::kINSTALLED_ROOT:
      return kTokenInstalledRoot;
    case ProfileKind::kPORTABLE_ROOT:
      return kTokenPortableRoot;
    case ProfileKind::kPERSIST:
      return kTokenPersist;
    case ProfileKind::kPACKAGED:
      return kTokenPackaged;
  }
  return kTokenInstalledRoot;
}

auto ProfileKindFromString(const QString &s) -> ProfileKind {
  if (s == kTokenPortableRoot) return ProfileKind::kPORTABLE_ROOT;
  if (s == kTokenPersist) return ProfileKind::kPERSIST;
  if (s == kTokenPackaged) return ProfileKind::kPACKAGED;
  return ProfileKind::kINSTALLED_ROOT;
}

auto Profile::DisplayName() const -> QString { return Id(); }

void Profile::ApplyMarkerPolicy(const ProfileMarker &marker) {
  policy_.self_contained = marker.self_contained;
}

void Profile::Unmount(ProfileUnmountMode /*mode*/) {}

auto Profile::IsTransient() const -> bool { return false; }

auto Profile::AllowsSystemKeychain() const -> bool { return true; }

auto Profile::AllowsKeyRotation() const -> bool { return true; }

auto Profile::IsRegistrable() const -> bool { return false; }

auto Profile::SettingsFilePath() const -> QString {
  return Root() + "/config/config.ini";
}

auto Profile::LockRoot() const -> QString { return Root(); }

auto Profile::MakeAccessor() const -> QSharedPointer<ProfileAccessor> {
  return QSharedPointer<FsProfileAccessor>::create(Root(), SettingsFilePath());
}

auto Profile::MarkerPath() const -> QString {
  return ProfileMarkerPathFor(Root());
}

auto UnpackagedProfile::Mount(const ProfileMountContext & /*ctx*/)
    -> ProfileMountResult {
  // Already unpacked by definition; making the root exist is the whole job.
  // The individual areas are provisioned by the accessor, which is the thing
  // that knows what they are called.
  if (root_.isEmpty()) {
    return {ProfileMountStatus::kIO_FAILED, "this profile has no location"};
  }

  if (!QDir(root_).exists() && !QDir(root_).mkpath(".")) {
    return {ProfileMountStatus::kIO_FAILED, root_};
  }

  return {};
}

auto RootProfile::ProfilesDir() const -> QString { return root_ + "/profiles"; }

auto RootProfile::IsRegistrable() const -> bool { return false; }

InstalledRootProfile::InstalledRootProfile(QString root)
    : RootProfile(std::move(root)) {}

auto InstalledRootProfile::Kind() const -> ProfileKind {
  return ProfileKind::kINSTALLED_ROOT;
}

auto InstalledRootProfile::Id() const -> QString { return kTokenInstalledRoot; }

auto InstalledRootProfile::SettingsFilePath() const -> QString {
#ifdef Q_OS_WINDOWS
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
         "/config.ini";
#else
  // The one profile that keeps Qt's native store, keyed by the organization
  // and application name set in GpgFrontendApplication's constructor. Every
  // existing installation's settings are already there; migrating them would
  // silently orphan all of them.
  return {};
#endif
}

auto InstalledRootProfile::LaunchArguments() const -> QStringList { return {}; }

PortableRootProfile::PortableRootProfile(QString root)
    : RootProfile(std::move(root)) {
  policy_.self_contained = true;
}

auto PortableRootProfile::Kind() const -> ProfileKind {
  return ProfileKind::kPORTABLE_ROOT;
}

auto PortableRootProfile::Id() const -> QString { return kTokenPortableRoot; }

auto PortableRootProfile::AllowsSystemKeychain() const -> bool {
  // A portable directory exists to be plugged into another computer, and a key
  // wrapped with this one's credential store cannot be opened there.
  return false;
}

auto PortableRootProfile::LaunchArguments() const -> QStringList { return {}; }

void PortableRootProfile::ApplyMarkerPolicy(const ProfileMarker & /*marker*/) {
  // Deliberately ignored: for this shape the location is the decision, and a
  // marker saying otherwise came from a profile that was copied here.
}

PersistProfile::PersistProfile(QString id, QString root)
    : UnpackagedProfile(std::move(root)), id_(std::move(id)) {}

auto PersistProfile::Kind() const -> ProfileKind {
  return ProfileKind::kPERSIST;
}

auto PersistProfile::Id() const -> QString { return id_; }

auto PersistProfile::IsRegistrable() const -> bool { return true; }

auto PersistProfile::LaunchArguments() const -> QStringList {
  return {"--profile", id_};
}

PackagedProfile::PackagedProfile(QString package_path, QString profiles_root)
    : package_path_(std::move(package_path)),
      profiles_root_(std::move(profiles_root)) {
  // Derived from the package's own path rather than minted at random, so the
  // anchor is known before anything is extracted — which is what lets the
  // loader take the profile lock first, and lets a second window work out for
  // itself that this very package is already open.
  anchor_ = ProfileSessionRoot(profiles_root_, package_path_);
  id_ = QFileInfo(anchor_).fileName();
}

auto PackagedProfile::LockRoot() const -> QString { return anchor_; }

auto PackagedProfile::MakeAccessor() const -> QSharedPointer<ProfileAccessor> {
  // The one place where a packaged session's storage is decided. Cached rather
  // than rebuilt, because the loader asks for an accessor more than once and
  // every caller has to be handed the same storage — and because probing twice
  // could answer differently the second time.
  if (!storage_.isNull() || storage_refused_) return storage_;

  ProfileAccessorSpec spec;
  spec.digest = QFileInfo(anchor_).fileName();
  spec.anchor = anchor_;
  spec.policy = ResolveProfileStoragePolicy();

  // The size the package declares, when it declares one. Mount() peeks the
  // manifest before asking for storage precisely so that this is not a guess:
  // sizing from the *compressed* file hands a profile that compressed well a
  // storage too small to open in, which surfaces part way through unpacking or,
  // worse, at the close that has to write it back.
  //
  // Zero for a package written before the field existed, which is the case the
  // heuristic still covers.
  spec.budget_bytes =
      ProfileStorageBudget(QFileInfo(package_path_).size(), declared_bytes_);

  // The digest is the anchor's directory name, which is dot-prefixed so that
  // the profiles-folder scan skips it. Inside another base that leading dot
  // would only hide the session from the sweep that has to find it.
  while (spec.digest.startsWith('.')) spec.digest.remove(0, 1);

  auto result = MakeProfileAccessorFor(spec);
  storage_rejections_ = result.rejections;

  if (result.accessor.isNull()) {
    // Remembered, so that a second ask does not re-probe and does not report
    // the same refusal twice.
    storage_refused_ = true;
    return storage_;
  }

  storage_anchor_state_ = result.anchor_state;
  storage_ = result.accessor;
  return storage_;
}

void PackagedProfile::DiscardSessionStorage() {
  if (storage_) storage_->Release(ProfileStorageRelease::kSCRUB);
  storage_.reset();

  // The anchor was made only so that it could be locked. Leaving it behind
  // would litter the profiles folder with one empty directory per package that
  // failed to open, and the sweep deliberately ignores those.
  if (!anchor_.isEmpty()) QDir(anchor_).removeRecursively();
}

auto PackagedProfile::Kind() const -> ProfileKind {
  return ProfileKind::kPACKAGED;
}

auto PackagedProfile::Id() const -> QString { return id_; }

auto PackagedProfile::Root() const -> QString {
  // Empty until storage has been provisioned, and deliberately so: where a
  // session's tree lives is chosen, not derived, and anything that assumed
  // otherwise should fail loudly here rather than quietly read the anchor.
  return storage_.isNull() ? QString{} : storage_->PathOf(ProfileArea::kRoot);
}

auto PackagedProfile::DisplayName() const -> QString {
  if (!manifest_.display_name.isEmpty()) return manifest_.display_name;
  return QFileInfo(package_path_).completeBaseName();
}

auto PackagedProfile::IsTransient() const -> bool { return true; }

auto PackagedProfile::AllowsSystemKeychain() const -> bool {
  // This profile is inside a file that was written to be carried somewhere
  // else, quite possibly to an operating system where "the system credential
  // store" is not the same thing and often is not anything at all.
  return false;
}

auto PackagedProfile::AllowsKeyRotation() const -> bool {
  // A rotated key would be written into storage this process deletes on the
  // way out, taking everything it had encrypted with it.
  return false;
}

auto PackagedProfile::IsRegistrable() const -> bool { return false; }

auto PackagedProfile::LaunchArguments() const -> QStringList {
  return {package_path_};
}

auto PackagedProfile::Protection() const -> ProfilePackageProtection {
  return protection_;
}

auto PackagedProfile::Manifest() const -> const ProfilePackageManifest & {
  return manifest_;
}

auto PackagedProfile::Inspect() -> ProfileMountResult {
  const auto inspection = InspectProfilePackage(package_path_);
  if (!inspection.Ok()) {
    return {MountStatusFor(inspection.status), inspection.detail};
  }

  protection_ = inspection.header.protection;
  inspected_ = true;

  if (protection_ == ProfilePackageProtection::kPIN) {
    return {ProfileMountStatus::kNEEDS_PASSPHRASE, package_path_};
  }
  return {};
}

auto PackagedProfile::Mount(const ProfileMountContext &ctx)
    -> ProfileMountResult {
  if (anchor_.isEmpty()) {
    return {ProfileMountStatus::kIO_FAILED,
            "there is nowhere to open this package"};
  }

  if (!inspected_) {
    if (const auto inspection = Inspect();
        inspection.status != ProfileMountStatus::kOK &&
        inspection.status != ProfileMountStatus::kNEEDS_PASSPHRASE) {
      return inspection;
    }
  }

  // Ask the package how big it is before asking for anywhere to put it. This
  // is the only window in which the answer can matter: storage has to exist
  // before there is anywhere to unpack into, so a size learned afterwards would
  // arrive too late to size it.
  //
  // Best effort by design. A package that will not describe itself here is not
  // refused -- it may be one written before the field existed, and it may
  // simply be a wrong passphrase, which the real open below reports properly.
  if (declared_bytes_ == 0 && storage_.isNull()) {
    const auto peeked =
        PeekProfilePackageManifest(package_path_, ctx.passphrase);
    if (peeked.Ok()) declared_bytes_ = peeked.manifest.uncompressed_bytes;
  }

  // The storage is what decides where the tree may be unpacked, so it has to
  // exist before there is anywhere to unpack into.
  auto storage = MakeAccessor();
  if (storage.isNull()) {
    // Every reason, verbatim: which candidate was passed over and why is the
    // only part of this a user can act on.
    return {ProfileMountStatus::kNO_PROTECTED_STORAGE,
            storage_rejections_.join("\n")};
  }

  // Before anything is extracted, never after: the anchor is the only thing
  // that knows where this session's tree went, and a process that dies between
  // provisioning and recording it strands that tree somewhere no later sweep
  // would think to look.
  //
  // Taken from the factory rather than from the accessor. The accessor may be
  // wrapped -- a driver that holds an area in memory is still a
  // ProfileAccessor -- and a cast to the concrete type would silently return
  // null and skip this, which is exactly the failure the pointer exists to
  // prevent.
  if (!storage_anchor_state_.isEmpty() &&
      !WriteSessionPointer(anchor_, storage_anchor_state_)) {
    LOG_W() << "could not record where this session's storage went:" << anchor_;
  }

  const auto opened = OpenPackageSession(package_path_, *storage,
                                         ctx.passphrase, ctx.schema_version);
  if (!opened.Ok()) {
    return {MountStatusFor(opened.status), opened.detail};
  }

  manifest_ = opened.manifest;
  passphrase_ = ctx.passphrase;
  policy_.self_contained = manifest_.self_contained;
  mounted_ = true;

  if (const auto error = seal_extracted_key(); !error.isEmpty()) {
    // The tree holds an unprotected copy of the profile's key and we could not
    // protect it, so it does not get to exist.
    Unmount(ProfileUnmountMode::kNORMAL);
    return {ProfileMountStatus::kIO_FAILED, error};
  }

  return {};
}

auto PackagedProfile::seal_extracted_key() -> QString {
  // A package carries its key unprotected — the package's own passphrase is
  // what protected it. Inside the session that passphrase is still the only
  // secret in play, so the stored form is sealed with it rather than left as
  // plaintext for as long as the window is open.
  //
  // The alternative was a second secret, which would be a second thing to
  // forget, and this way the user is asked exactly once.
  //
  // Through the accessor rather than by path: the driver may hold this area in
  // memory, in which case there is no file here and never was. What this
  // decides is the *stored form* — what init_root_key() will Read() and hand
  // to UnsealKey() — not whether anything on a disk needs erasing.
  auto storage = MakeAccessor();
  if (storage.isNull()) return "the profile's key has nowhere to live";

  auto stored = storage->Read(ProfileArea::kSecure, kProfileRootKeyName);
  if (!stored) return "the profile's key could not be read";

  const bool wanted = protection_ == ProfilePackageProtection::kPIN;
  const bool sealed = AESCryptoHelper::IsEncryptedBuffer(*stored);

  // Idempotent: what is already in the right form is left exactly as it is.
  if (wanted == sealed) {
    record_key_protection(wanted);
    return {};
  }

  // Only one direction is reachable — a package always unpacks a plaintext
  // key — and unsealing one here would mean writing a plaintext key out.
  if (!wanted) return {};

  auto protect = ProfileSecureKeyManager::SealKey(passphrase_, {}, *stored);
  if (!protect) return "the profile's key could not be protected";

  // Proven to open before it replaces the only copy of the key: this is a copy
  // of key material, and a sealed file nothing can unseal is the same as no key
  // at all.
  auto proof = ProfileSecureKeyManager::UnsealKey(passphrase_, {}, *protect);
  if (!proof || *proof != *stored) {
    return "the profile's key did not survive being protected";
  }

  if (!storage->Write(ProfileArea::kSecure, kProfileRootKeyName, *protect)) {
    return "the profile's key could not be written";
  }

  record_key_protection(true);
  return {};
}

void PackagedProfile::record_key_protection(bool pin) {
  // The setting has to agree with the file, or the next thing to read it
  // resolves a protection the key on disk does not have. AdoptExtractedProfile
  // writes "none" for the import path, where a local copy deliberately does not
  // inherit the package's passphrase; a session does.
  auto settings = MakeAccessor()->Settings();
  settings.setValue("advanced/app_key_protection",
                    AppKeyProtectionToString(pin ? AppKeyProtection::kPIN
                                                 : AppKeyProtection::kNONE));
  settings.sync();
}

void PackagedProfile::Unmount(ProfileUnmountMode mode) {
  if (!mounted_) return;

  // Whatever the mode: the tree holds a copy of the profile's key material and
  // must not survive the process that unpacked it. Writing the session back to
  // the file it came from happens before this, and belongs to the application —
  // it needs the resident key and the live settings, which a profile has no
  // business reaching for.
  //
  // A forced unmount runs from the shutdown watchdog's thread against a
  // deadline, so it gets the cheap destruction rather than the thorough one.
  if (storage_) {
    storage_->Release(mode == ProfileUnmountMode::kFORCED
                          ? ProfileStorageRelease::kFAST
                          : ProfileStorageRelease::kSCRUB);
  }
  mounted_ = false;
}

auto PackagedProfile::WriteBackRequest() const -> ProfileExportRequest {
  ProfileExportRequest request;
  request.profile_root = Root();

  // Staged inside this session's own storage. The staging tree is a full
  // plaintext copy of the profile *including an unprotected application key*,
  // and putting that in the profiles folder would undo, at the last moment,
  // exactly what opening the package in protected storage was for.
  request.dest_path = package_path_;
  request.include_workspace = manifest_.workspace_included;
  request.protection = protection_;
  request.passphrase = passphrase_;

  // Keep the package's identity across the rewrite: this is the same package,
  // saved again, not a new one that happens to sit at the same path.
  request.manifest.package_id = manifest_.package_id;
  request.manifest.display_name = manifest_.display_name;
  request.manifest.profile_id = manifest_.profile_id;
  request.manifest.self_contained = manifest_.self_contained;

  return request;
}

namespace {

/// Options that consume the following argument, so a scan for a positional
/// never mistakes an option's value for one.
const QStringList kValueOptions = {"--profile", "--log-level", "-l"};

auto OptionValue(const QStringList &args, const QString &name) -> QString {
  const auto long_form = "--" + name;
  for (int i = 1; i < args.size(); ++i) {
    const auto &arg = args.at(i);
    if (arg == long_form) {
      return i + 1 < args.size() ? args.at(i + 1) : QString{};
    }
    if (arg.startsWith(long_form + "=")) {
      return arg.mid(long_form.size() + 1);
    }
  }
  return {};
}

auto PositionalPackage(const QStringList &args) -> QString {
  for (int i = 1; i < args.size(); ++i) {
    const auto &arg = args.at(i);
    if (kValueOptions.contains(arg)) {
      ++i;  // skip the value
      continue;
    }
    if (arg.startsWith('-')) continue;
    if (arg.endsWith(kProfilePackageExtension, Qt::CaseInsensitive)) return arg;
  }
  return {};
}

/// Reserved on Windows regardless of extension; creating one of these produces
/// a directory that exists on Linux and cannot on Windows, which is exactly the
/// kind of asymmetry a portable profile must not have.
auto IsWindowsDeviceName(const QString &id) -> bool {
  static const QStringList kReserved = {
      "con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4",
      "com5", "com6", "com7", "com8", "com9", "lpt1", "lpt2", "lpt3",
      "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};
  return kReserved.contains(id.toLower());
}

auto ResolvePersist(const QString &id, const ProfileSelectionInput &in,
                    const QString &profiles_root) -> ProfileSelectionResult {
  ProfileSelectionResult r;
  r.selection.profiles_root = profiles_root;

  // the fallback must be usable: an error still leaves the process on a sane
  // installed-root selection, so nothing downstream has to cope with a
  // half-resolved run
  r.selection.kind = ProfileKind::kINSTALLED_ROOT;
  r.selection.id = "classic";
  r.selection.root = in.installed_root;

  if (!IsValidProfileId(id)) {
    r.error = QString("'%1' is not a valid profile name.").arg(id);
    return r;
  }

  // A directory scan is authoritative, so an id that is not in it is a typo or
  // a profile that has been deleted. Falling through would provision a fresh
  // empty profile under that name, which looks to the user exactly like their
  // keys having disappeared.
  if (!in.known_ids.contains(id)) {
    r.error = QString("There is no profile named '%1'.").arg(id);
    return r;
  }

  r.selection.kind = ProfileKind::kPERSIST;
  r.selection.id = id;
  r.selection.root = profiles_root + "/" + id;
  return r;
}

/// This build's root profile: the bottom of the precedence ladder, and the only
/// answer at all where profiles are not offered.
auto ImplicitDefault(const ProfileSelectionInput &in,
                     const QString &profiles_root) -> ProfileSelectionResult {
  ProfileSelectionResult r;
  r.selection.profiles_root = profiles_root;
  if (in.portable_build) {
    r.selection.kind = ProfileKind::kPORTABLE_ROOT;
    r.selection.id = "portable";
    r.selection.root = in.portable_root;
  } else {
    r.selection.kind = ProfileKind::kINSTALLED_ROOT;
    r.selection.id = "classic";
    r.selection.root = in.installed_root;
  }
  return r;
}

}  // namespace

auto IsValidProfileId(const QString &id) -> bool {
  if (id.isEmpty() || id.size() > 64) return false;
  if (IsWindowsDeviceName(id)) return false;

  static const QRegularExpression kPattern(QStringLiteral("^[a-z0-9_-]+$"));
  if (!kPattern.match(id).hasMatch()) return false;

  // "." and ".." cannot match the pattern above, but a leading hyphen would
  // read as an option everywhere the id is passed on a command line
  return !id.startsWith('-');
}

auto ResolveProfileSelection(const ProfileSelectionInput &in)
    -> ProfileSelectionResult {
  const auto base = in.portable_build ? in.portable_root : in.installed_root;
  const auto profiles_root = base + "/profiles";

  // A build without profiles has exactly one thing to resolve to, so the whole
  // ladder below is skipped rather than each rung being guarded. Silently: a
  // deep restart hands this process its predecessor's argv, so a `--profile`
  // left over from an installation that once had profiles must not turn into a
  // startup error the user cannot act on.
  if (!in.multi_profile) return ImplicitDefault(in, profiles_root);

  // 1. a named profile. Explicit selection outranks the build flavour: on a
  // portable build this opens <portable-root>/profiles/<id>, and that profile's
  // own policy decides whether it is self-contained — the flavour only chooses
  // the implicit default at the bottom of this ladder.
  const auto cli_named = OptionValue(in.args, "profile");
  if (!cli_named.isEmpty()) return ResolvePersist(cli_named, in, profiles_root);

  // 2. a package named on the command line. It has no root of its own until it
  // is extracted, which the loader does — and which may need a passphrase, so
  // it cannot happen in a pure function.
  const auto package = PositionalPackage(in.args);
  if (!package.isEmpty()) {
    ProfileSelectionResult r;
    r.selection.profiles_root = profiles_root;
    r.selection.kind = ProfileKind::kPACKAGED;
    r.selection.package_path = package;
    r.selection.id = QFileInfo(package).completeBaseName();
    return r;
  }

  // 3. the environment, for CI and headless runs
  if (!in.env_profile.isEmpty()) {
    return ResolvePersist(in.env_profile, in, profiles_root);
  }

  // 4. the implicit default: this build's root profile. Nothing is remembered
  // between runs on purpose — an instance always starts here, and another
  // profile or a package is opened from here into a new window.
  return ImplicitDefault(in, profiles_root);
}

auto MakeProfile(const ProfileSelection &selection) -> QSharedPointer<Profile> {
  switch (selection.kind) {
    case ProfileKind::kPORTABLE_ROOT:
      return QSharedPointer<PortableRootProfile>::create(selection.root);
    case ProfileKind::kPERSIST:
      return QSharedPointer<PersistProfile>::create(selection.id,
                                                    selection.root);
    case ProfileKind::kPACKAGED:
      return QSharedPointer<PackagedProfile>::create(selection.package_path,
                                                     selection.profiles_root);
    case ProfileKind::kINSTALLED_ROOT:
      break;
  }
  return QSharedPointer<InstalledRootProfile>::create(selection.root);
}

}  // namespace GpgFrontend
