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

#include "shell/present/ProfileStatus.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileSession.h"
#include "shell/present/SecurityDisplayNames.h"
#if GF_ENGINE_GNUPG
#include "core/function/gpg/GpgContext.h"
#endif

namespace GpgFrontend {

// Note the translation context spelled out at every call below rather than
// hoisted into a constant: lupdate silently skips every translate() call whose
// context is a named constant instead of a literal, which compiles cleanly and
// leaves the strings untranslated at run time.

auto DescribeSessionStorage(bool is_volatile, bool encrypted_at_rest)
    -> AboutStatusValue {
  if (is_volatile) {
    return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "Memory"),
            QCoreApplication::translate(
                "GpgFrontend::UI::AboutStatusInfo",
                "Not written to this disk in the normal course of things."),
            false};
  }

  if (encrypted_at_rest) {
    return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "An encrypted folder"),
            QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "Only this session can read it."),
            false};
  }

  return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                      "An ordinary folder on this disk"),
          QCoreApplication::translate(
              "GpgFrontend::UI::AboutStatusInfo",
              "Nothing here could hold it in memory or encrypt it."),
          true};
}

auto BuildProfileIdentityRows(ProfileKind kind, const QString& display_name,
                              bool transient) -> QList<StatusRow> {
  QList<StatusRow> rows;

  const auto name_caption = QCoreApplication::translate(
      "GpgFrontend::UI::AboutStatusInfo", "Profile:");
  const auto type_caption = QCoreApplication::translate(
      "GpgFrontend::UI::AboutStatusInfo", "Profile Type:");

  // A root profile is not named by its user, so CurrentProfileDisplayName()
  // answers with the kind -- and a type row under it then said the same word a
  // second time. What that row would have carried becomes the sentence here,
  // which is the part that was never on the page at all.
  if (kind == ProfileKind::kINSTALLED_ROOT) {
    rows.append({.caption = name_caption,
                 .value = display_name,
                 .detail = QCoreApplication::translate(
                     "GpgFrontend::UI::AboutStatusInfo",
                     "The profile this computer starts on, in your user data "
                     "folder."),
                 .emphasis = true});
    return rows;
  }

  if (kind == ProfileKind::kPORTABLE_ROOT) {
    rows.append({.caption = name_caption,
                 .value = display_name,
                 .detail = QCoreApplication::translate(
                     "GpgFrontend::UI::AboutStatusInfo",
                     "Kept beside the application, so it travels with it."),
                 .emphasis = true});
    return rows;
  }

  rows.append(
      {.caption = name_caption, .value = display_name, .emphasis = true});

  if (kind == ProfileKind::kPACKAGED) {
    rows.append(
        {.caption = type_caption,
         .value = QCoreApplication::translate(
             "GpgFrontend::UI::AboutStatusInfo", "Opened from a profile file"),
         // Asked of the profile rather than of the kind, so a shape
         // added later cannot inherit the sentence by being packaged.
         .detail = transient ? QCoreApplication::translate(
                                   "GpgFrontend::UI::AboutStatusInfo",
                                   "Temporary. It disappears when this "
                                   "window closes, and closing asks whether "
                                   "to save your changes back into the file.")
                             : QString()});
    return rows;
  }

  if (kind == ProfileKind::kEPHEMERAL) {
    rows.append({.caption = type_caption,
                 .value = QCoreApplication::translate(
                     "GpgFrontend::UI::AboutStatusInfo", "Temporary"),
                 .detail = QCoreApplication::translate(
                     "GpgFrontend::UI::AboutStatusInfo",
                     "Not kept on this computer. Everything in it is "
                     "destroyed when this window closes.")});
    return rows;
  }

  rows.append(
      {.caption = type_caption,
       .value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                            "Kept on this computer")});
  return rows;
}

auto DescribeAppKeyProtection(AppKeyProtection protection, bool allows_keychain)
    -> AboutStatusValue {
  QString value;
  switch (protection) {
    case AppKeyProtection::kKEYCHAIN:
      value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                          "System keychain");
      break;
    case AppKeyProtection::kPIN:
      value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                          "PIN at startup");
      break;
    case AppKeyProtection::kNONE:
      value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                          "No extra protection");
      break;
  }

  // Not a fallback, so not degraded: the rule is what keeps the profile
  // openable elsewhere. It is still the answer to a question the settings page
  // raises by greying the option out and then not explaining itself.
  if (!allows_keychain) {
    return {value,
            QCoreApplication::translate(
                "GpgFrontend::UI::AboutStatusInfo",
                "This profile can leave this computer, so the system keychain "
                "is not offered: a key sealed with one computer's keychain "
                "cannot be opened on another."),
            false};
  }

  return {value, {}, false};
}

auto CurrentProfileDisplayName() -> QString {
  const auto& session = ProfileSession::Instance();

  switch (session.Profile().Kind()) {
    case ProfileKind::kINSTALLED_ROOT:
      return QObject::tr("Default");
    case ProfileKind::kPORTABLE_ROOT:
      return QObject::tr("Portable");
    case ProfileKind::kEPHEMERAL:
      return QObject::tr("Temporary");
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

auto GatherProfileStatusFacts() -> ProfileStatusFacts {
  ProfileStatusFacts facts;

  facts.secure_level = qApp->property("GFSecureLevel").toInt();
  facts.portable_mode = qApp->property("GFPortableMode").toBool();
#ifdef Q_OS_ANDROID
  // A phone app is installed and nothing else; the row would only say so.
  facts.show_running_mode = false;
#endif
  facts.gnupg_offline_mode = qApp->property("GFGnuPGOfflineMode").toBool();
  facts.pinentry_program_path =
      qApp->property("GFPinentryProgramPath").toString();
  facts.profiles_root = qApp->property("GFProfilesRoot").toString();

  const auto& session = ProfileSession::Instance();
  const auto& profile = session.Profile();

  facts.kind = profile.Kind();
  facts.display_name = CurrentProfileDisplayName();
  facts.transient = profile.IsTransient();
  facts.profile_id = profile.Id();
  facts.root = profile.Root();
  facts.workspace = session.WorkspacePath();

  if (facts.transient) {
    const auto& storage = session.Accessor();
    facts.storage_volatile = storage.IsVolatile();
    facts.storage_encrypted = storage.IsEncryptedAtRest();
    facts.secure_area_resident = storage.IsAreaResident(ProfileArea::kSecure);
  }

  // Asked only once the key set is attached: Keys() aborts before that, and
  // the status is reachable from a window that opened without one.
  facts.rotating_keys = session.KeysLoaded() &&
                        session.Keys().Mode() == ProfileKeyMode::kROTATING;

  const auto& marker = session.Marker();
  facts.schema_version = marker.schema_version;
  facts.imported_from = marker.package_id;
  if (const auto* packaged = dynamic_cast<const PackagedProfile*>(&profile);
      packaged != nullptr) {
    facts.package_path = packaged->PackagePath();
  }

  facts.protection = ProfileLoader::AppKeyProtectionFromApp();
  facts.allows_keychain = profile.AllowsSystemKeychain();

  // Read from the registry rather than probed: opening the status must never
  // raise a keyring unlock prompt.
  if (auto* store = GetSystemSecretStore(); store != nullptr) {
    facts.secret_store_available = true;
    facts.secret_store_name = store->Name();
  }
  facts.secret_store_reason = SystemSecretStoreReason();

  facts.engines = GetGSS().AllSupportedEngines();
  facts.gnupg_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);
#if GF_ENGINE_GNUPG
  facts.gnupg_home_unusable_reason = GpgContext::FirstUnusableHomeReason();
#endif

  return facts;
}

auto BuildProfileStatus(const ProfileStatusFacts& facts)
    -> ProfileStatusReport {
  ProfileStatusReport report;
  auto& attention = report.attention;
  auto& glance = report.glance;

  // The strings below were the About dialog's Status tab's, and keep its
  // translation context, spelled out at each call for lupdate.

  // One row or two, decided by whether the profile has a name of its own.
  glance.append(BuildProfileIdentityRows(facts.kind, facts.display_name,
                                         facts.transient));

  const auto protection =
      DescribeAppKeyProtection(facts.protection, facts.allows_keychain);
  glance.append(
      {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                              "Application Key Protection:"),
       .value = protection.value,
       .detail = protection.detail});
  glance.append({.caption = QCoreApplication::translate(
                     "GpgFrontend::UI::StatusTab", "Secure Level:"),
                 .value = SecureLevelDisplayName(facts.secure_level)});

  // Reported whether or not it is in use: when "System keychain" is greyed out
  // in the settings, this is the only place that says why, and the loader's
  // reason rides along untranslated so it can be pasted into a bug report.
  if (!facts.secret_store_available) {
    attention.append(
        {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "System Credential Store:"),
         .value = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                              "Unavailable"),
         .detail = facts.secret_store_reason,
         .degraded = true});
  }

  // Only ever set when the home directory could not host the agent sockets.
  if (!facts.gnupg_home_unusable_reason.isEmpty()) {
    attention.append({.caption = QCoreApplication::translate(
                          "GpgFrontend::UI::StatusTab", "GnuPG Home:"),
                      .value = QCoreApplication::translate(
                          "GpgFrontend::UI::StatusTab", "Unusable"),
                      .detail = facts.gnupg_home_unusable_reason,
                      .degraded = true});
  }

  QList<StatusRow> application;
  if (facts.show_running_mode) {
    application.append(
        {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "Running Mode:"),
         .value = facts.portable_mode
                      ? QCoreApplication::translate(
                            "GpgFrontend::UI::StatusTab", "Portable Mode")
                      : QCoreApplication::translate(
                            "GpgFrontend::UI::StatusTab", "Installed Mode")});
  }
  if (facts.secret_store_available) {
    application.append(
        {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "System Credential Store:"),
         .value = facts.secret_store_name,
         .detail = facts.secret_store_reason});
  }
  report.details.append(
      {QCoreApplication::translate("GpgFrontend::UI::StatusTab", "Application"),
       application});

  QList<StatusRow> profile_rows;

  // The one value that identifies this profile in a bug report. A uuid is an
  // unbreakable token, so it gets the same treatment as a path.
  profile_rows.append({.caption = QCoreApplication::translate(
                           "GpgFrontend::UI::StatusTab", "Profile ID:"),
                       .value = facts.profile_id,
                       .path = true});

  // Only for a transient session, because only there was there a choice to
  // make.
  if (facts.transient) {
    const auto storage =
        DescribeSessionStorage(facts.storage_volatile, facts.storage_encrypted);

    auto& into = storage.degraded ? attention : profile_rows;
    into.append({.caption = QCoreApplication::translate(
                     "GpgFrontend::UI::StatusTab", "Session Storage:"),
                 .value = storage.value,
                 .detail = storage.detail,
                 .degraded = storage.degraded});

    // Stated separately from the row above, and deliberately narrow: the
    // storage line describes where the *profile* went; this describes one
    // thing inside it.
    if (facts.secure_area_resident) {
      profile_rows.append(
          {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                  "Profile Key:"),
           .value = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "Held in memory only"),
           .detail = QCoreApplication::translate(
               "GpgFrontend::UI::StatusTab",
               "The key that protects this profile's own saved data is never "
               "written here. Your OpenPGP keys are a separate thing and do "
               "live in the session storage above, because GnuPG needs real "
               "files for them.")});
    }
  }

  if (facts.rotating_keys) {
    profile_rows.append(
        {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "Profile Key Rotation:"),
         .value = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                              "On a schedule"),
         .detail = QCoreApplication::translate(
             "GpgFrontend::UI::StatusTab",
             "New saved data uses the current period's key, and the "
             "keys that open what earlier periods wrote are kept "
             "alongside it.")});
  }

  if (facts.schema_version > 0) {
    profile_rows.append(
        {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "Profile Format Version:"),
         .value = QString::number(facts.schema_version)});
  }

  // The file this session is running out of.
  if (!facts.package_path.isEmpty()) {
    profile_rows.append({.caption = QCoreApplication::translate(
                             "GpgFrontend::UI::StatusTab", "Profile File:"),
                         .value = QDir::toNativeSeparators(facts.package_path),
                         .path = true});
  }

  // A different fact from the row above: stamped by an *import*, which mints a
  // fresh identity and never touches the file again.
  if (!facts.imported_from.isEmpty()) {
    profile_rows.append({.caption = QCoreApplication::translate(
                             "GpgFrontend::UI::StatusTab", "Imported From:"),
                         .value = facts.imported_from,
                         .path = true});
  }
  report.details.append(
      {QCoreApplication::translate("GpgFrontend::UI::StatusTab", "Profile"),
       profile_rows});

  QList<StatusRow> folders;
  folders.append({.caption = QCoreApplication::translate(
                      "GpgFrontend::UI::StatusTab", "Profile Folder:"),
                  .value = QDir::toNativeSeparators(facts.root),
                  .path = true});

  if (facts.workspace.isEmpty()) {
    folders.append({.caption = QCoreApplication::translate(
                        "GpgFrontend::UI::StatusTab", "Workspace:"),
                    .value = QCoreApplication::translate(
                        "GpgFrontend::UI::StatusTab", "None"),
                    .dimmed = true});
  } else {
    folders.append({.caption = QCoreApplication::translate(
                        "GpgFrontend::UI::StatusTab", "Workspace:"),
                    .value = QDir::toNativeSeparators(facts.workspace),
                    .path = true});
  }

  // Where this machine keeps its persisted profiles is a property of the
  // installation, not of the one profile in front of us.
  if (!facts.profiles_root.isEmpty()) {
    folders.append({.caption = QCoreApplication::translate(
                        "GpgFrontend::UI::StatusTab", "Profiles Folder:"),
                    .value = QDir::toNativeSeparators(facts.profiles_root),
                    .path = true});
  }
  report.details.append(
      {QCoreApplication::translate("GpgFrontend::UI::StatusTab", "Folders"),
       folders});

  // The engines, and the settings that belong to one of them: a pinentry path
  // is how GnuPG asks for a passphrase, not a property of GpgFrontend.
  QList<StatusRow> engines;
  for (const auto& engine : facts.engines) {
    engines.append({.caption = engine,
                    .value = QCoreApplication::translate(
                        "GpgFrontend::UI::StatusTab", "Available")});
  }

  if (facts.gnupg_supported) {
    engines.append(
        {.caption = QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                                "GnuPG Offline Mode:"),
         .value = facts.gnupg_offline_mode
                      ? QCoreApplication::translate(
                            "GpgFrontend::UI::StatusTab", "Active")
                      : QCoreApplication::translate(
                            "GpgFrontend::UI::StatusTab", "Disabled"),
         .dimmed = !facts.gnupg_offline_mode});

    // The default is a statement, not a path.
    const auto& pinentry = facts.pinentry_program_path;
    engines.append({.caption = QCoreApplication::translate(
                        "GpgFrontend::UI::StatusTab", "Pinentry Program Path:"),
                    .value = pinentry.isEmpty()
                                 ? QCoreApplication::translate(
                                       "GpgFrontend::UI::StatusTab",
                                       "Default Pinentry Program")
                                 : QDir::toNativeSeparators(pinentry),
                    .dimmed = pinentry.isEmpty(),
                    .path = !pinentry.isEmpty()});
  }
  report.details.append({QCoreApplication::translate(
                             "GpgFrontend::UI::StatusTab", "OpenPGP Engines"),
                         engines});

  return report;
}

auto ProfileStatusSections(const ProfileStatusReport& report)
    -> QList<StatusSection> {
  QList<StatusSection> sections;
  sections.append({QCoreApplication::translate("GpgFrontend::UI::StatusTab",
                                               "Needs Attention"),
                   report.attention});
  sections.append(
      {QCoreApplication::translate("GpgFrontend::UI::StatusTab", "At a Glance"),
       report.glance});
  sections.append(report.details);

  sections.removeIf(
      [](const StatusSection& section) { return section.rows.isEmpty(); });
  return sections;
}

auto ProfileStatusText(const QList<StatusSection>& sections) -> QString {
  QStringList lines;
  for (const auto& section : sections) {
    if (!lines.isEmpty()) lines << QString();
    lines << QStringLiteral("[%1]").arg(section.title);
    for (const auto& row : section.rows) {
      lines << QStringLiteral("%1 %2").arg(row.caption, row.value).trimmed();
      if (!row.detail.isEmpty())
        lines << QStringLiteral("    %1").arg(row.detail);
    }
  }
  return lines.join(u'\n');
}

auto ShowsDetailInline(const QString& detail, bool degraded) -> bool {
  return !detail.isEmpty() && degraded;
}

}  // namespace GpgFrontend
