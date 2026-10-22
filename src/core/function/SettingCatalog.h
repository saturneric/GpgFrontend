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

#pragma once

#include <array>

#include "core/function/SettingItem.h"

/**
 * @file SettingCatalog.h
 * @brief Every application setting, once.
 *
 * Read a setting through its entry here -- `SettingItems::kDefaultEngine
 * .Read(settings)` -- rather than by spelling its key. The key, the default,
 * and every policy that used to be a string list somewhere else live in one
 * row, so renaming a key, changing a default or deciding whether a setting
 * travels is one edit that every reader sees.
 *
 * Titles are marked for translation with a literal context on purpose: a
 * context held in a constant makes lupdate skip the call without a word.
 */

namespace GpgFrontend::SettingItems {

// Spelled out rather than pulled in with `using enum`: this is built as C++17.
constexpr auto kENGINE = SettingGroup::kENGINE;
constexpr auto kRPGP = SettingGroup::kRPGP;
constexpr auto kBEHAVIOUR = SettingGroup::kBEHAVIOUR;
constexpr auto kAPPEARANCE = SettingGroup::kAPPEARANCE;
constexpr auto kDIAGNOSTICS = SettingGroup::kDIAGNOSTICS;
constexpr auto kSECURITY = SettingGroup::kSECURITY;
constexpr auto kNETWORK = SettingGroup::kNETWORK;
constexpr auto kWORKSPACE = SettingGroup::kWORKSPACE;
constexpr auto kLAYOUT = SettingGroup::kLAYOUT;

constexpr auto kNoRestart = SettingRestart::kNONE;
constexpr auto kUIRestart = SettingRestart::kUI;
constexpr auto kCoreRestart = SettingRestart::kCORE;

constexpr auto kProfile = SettingLocality::kPROFILE;
constexpr auto kMachine = SettingLocality::kMACHINE;

// ------------------------------------------------- OpenPGP engine and GnuPG

inline constexpr StringSetting kDefaultEngine{
    {.key = "basic/default_engine",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Default OpenPGP engine"),
     .restart = kCoreRestart,
     .inheritable = true},
    "GNUPG"};

inline constexpr BoolSetting kUseCustomGnuPGInstallPath{
    {.key = "gnupg/use_custom_gnupg_install_path",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Use a custom GnuPG installation"),
     .restart = kCoreRestart,
     .locality = kMachine,
     .inheritable = true},
    false};

inline constexpr StringSetting kCustomGnuPGInstallPath{
    {.key = "gnupg/custom_gnupg_install_path",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Custom GnuPG installation"),
     .restart = kCoreRestart,
     .locality = kMachine,
     .inheritable = true},
    ""};

inline constexpr BoolSetting kEnableGpgmeDebugLog{
    {.key = "gnupg/enable_gpgme_debug_log",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "GPGME debug log"),
     .restart = kCoreRestart,
     .inheritable = true},
    false};

inline constexpr BoolSetting kKillAllGnuPGDaemonAtClose{
    {.key = "gnupg/kill_all_gnupg_daemon_at_close",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Stop GnuPG daemons on exit"),
     .inheritable = true},
    true};

inline constexpr BoolSetting kNonAsciiAtFileOperation{
    {.key = "gnupg/non_ascii_at_file_operation",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Binary output for file operations"),
     .inheritable = true},
    true};

inline constexpr BoolSetting kForbidAllGnuPGConnection{
    {.key = "network/forbid_all_gnupg_connection",
     .group = kENGINE,
     .title =
         QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Keep GnuPG offline"),
     .inheritable = true},
    false};

inline constexpr BoolSetting kClearGpgPasswordCache{
    {.key = "basic/clear_gpg_password_cache",
     .group = kENGINE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Forget cached passphrases on exit"),
     .inheritable = true},
    true};

// ----------------------------------------------------------------- rPGP

inline constexpr Int64Setting kPasswordCacheTtl{
    {.key = "engine/password_cache_ttl",
     .group = kRPGP,
     .title =
         QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Passphrase cache time"),
     .inheritable = true},
    600};

inline constexpr Int64Setting kPasswordCacheMaxTtl{
    {.key = "engine/password_cache_max_ttl",
     .group = kRPGP,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Longest passphrase cache time"),
     .inheritable = true},
    7200};

inline constexpr IntSetting kPassphrasePromptTimeout{
    {.key = "engine/passphrase_prompt_timeout",
     .group = kRPGP,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Passphrase prompt timeout"),
     .inheritable = true},
    60};

inline constexpr IntSetting kPassphraseRequesterGraceMs{
    {.key = "engine/passphrase_requester_grace_ms",
     .group = kRPGP,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Passphrase requester grace period"),
     .inheritable = true},
    5000};

inline constexpr StringSetting kArgon2S2kProfile{
    {.key = "engine/argon2_s2k_profile",
     .group = kRPGP,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Argon2 key derivation strength"),
     .inheritable = true},
    "high_memory"};

// ------------------------------------------------------- everyday behaviour

inline constexpr BoolSetting kConfirmImportKeys{
    {.key = "basic/confirm_import_keys",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Confirm before importing keys"),
     .inheritable = true},
    true};

inline constexpr BoolSetting kSingleRecipientMode{
    {.key = "basic/single_recipient_mode",
     .group = kBEHAVIOUR,
     .title =
         QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Single recipient mode"),
     .inheritable = true},
    false};

inline constexpr BoolSetting kConfirmMultipleRecipients{
    {.key = "basic/confirm_multiple_recipients",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Confirm encrypting to several recipients"),
     .inheritable = true},
    false};

inline constexpr BoolSetting kRememberCheckedKeys{
    {.key = "basic/remember_checked_keys",
     .group = kBEHAVIOUR,
     .title =
         QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Remember checked keys"),
     .inheritable = true},
    false};

inline constexpr BoolSetting kUndoableOperations{
    {.key = "basic/undoable_operations",
     .group = kBEHAVIOUR,
     .title =
         QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Undoable operations"),
     .inheritable = true},
    false};

inline constexpr BoolSetting kRestoreTextEditorPage{
    {.key = "basic/restore_text_editor_page",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Restore text editor pages"),
     .inheritable = true},
    true};

inline constexpr StringSetting kDefaultWorkspaceAs{
    {.key = "basic/default_workspace_as",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Default workspace view"),
     .inheritable = true},
    "file_panel"};

inline constexpr StringSetting kFilePanelDefaultPathMode{
    {.key = "basic/file_panel_default_path_mode",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "File panel starting folder"),
     .inheritable = true,
     .legacy_key = "basic/home_path_as_file_panel_default_path"},
    ""};

inline constexpr BoolSetting kShowTypeColumn{
    {.key = "file_panel/show_type_column",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Show the file type column"),
     .inheritable = true},
    false};

inline constexpr IntSetting kExpiringSoonDays{
    {.key = "keys/expiring_soon_days",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Days before a key counts as expiring soon"),
     .inheritable = true},
    30};

inline constexpr BoolSetting kAutoImportMissingKey{
    {.key = "network/auto_import_missing_key",
     .group = kBEHAVIOUR,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Fetch missing keys automatically"),
     .inheritable = true},
    false};

inline constexpr StringSetting kModuleLoadingPolicy{
    {.key = "basic/module_loading_policy",
     .group = kBEHAVIOUR,
     .title =
         QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Which modules to load"),
     .restart = kCoreRestart,
     .inheritable = true},
    "only_integrated"};

// -------------------------------------------------- language and appearance

inline constexpr StringSetting kLanguage{
    {.key = "basic/lang",
     .group = kAPPEARANCE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Language"),
     .restart = kUIRestart,
     .inheritable = true},
    ""};

inline constexpr StringSetting kTheme{
    {.key = "appearance/theme",
     .group = kAPPEARANCE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Theme"),
     .restart = kUIRestart,
     .inheritable = true},
    ""};

inline constexpr BoolSetting kShowWizard{
    {.key = "wizard/show_wizard",
     .group = kAPPEARANCE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Show the first-run wizard"),
     .inheritable = true},
    true};

// ------------------------------------------------------------- diagnostics

inline constexpr IntSetting kLogLevel{
    {.key = "advanced/log_level",
     .group = kDIAGNOSTICS,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Log level"),
     .restart = kCoreRestart,
     .inheritable = true,
     .deployment_key = "LogLevel"},
    3};  // GFLogLevel::kCRITICAL: an unset level must not mean debug

inline constexpr IntSetting kLogRingBufferCapacity{
    {.key = "advanced/log_ring_buffer_capacity",
     .group = kDIAGNOSTICS,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Log lines kept in memory"),
     .restart = kCoreRestart,
     .deployment_key = "LogRingBufferCapacity"},
    1024};

// ---------------------------------------------------------------- security

inline constexpr IntSetting kSecureLevel{
    {.key = "advanced/secure_level",
     .group = kSECURITY,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Secure level"),
     .restart = kCoreRestart,
     .deployment_key = "SecureLevel"},
    0};

inline constexpr StringSetting kAppKeyProtection{
    {.key = "advanced/app_key_protection",
     .group = kSECURITY,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Application key protection"),
     .restart = kCoreRestart,
     .locality = kMachine,
     .deployment_key = "AppKeyProtection"},
    ""};

/// Superseded by kAppKeyProtection and still read so an older profile
/// resolves the same way; never written.
inline constexpr StringSetting kOSSecretStore{
    {.key = "advanced/os_secret_store",
     .group = kSECURITY,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "System credential store (legacy)"),
     .restart = kCoreRestart,
     .locality = kMachine,
     .deployment_key = "OSSecretStore"},
    ""};

inline constexpr StringSetting kProfilePackageStorage{
    {.key = "advanced/profile_package_storage",
     .group = kSECURITY,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Where profile files are opened"),
     .locality = kMachine},
    ""};

// ----------------------------------------------------------------- network

inline constexpr BoolSetting kProhibitUpdateCheck{
    {.key = "network/prohibit_update_check",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem",
                                "Never check for updates")},
    false};

inline constexpr BoolSetting kProxyEnable{
    {.key = "proxy/enable",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Use a proxy")},
    false};

inline constexpr StringSetting kProxyType{
    {.key = "proxy/proxy_type",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Proxy type")},
    ""};

inline constexpr StringSetting kProxyHost{
    {.key = "proxy/proxy_host",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Proxy host")},
    ""};

inline constexpr IntSetting kProxyPort{
    {.key = "proxy/port",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Proxy port")},
    0};

inline constexpr StringSetting kProxyUsername{
    {.key = "proxy/username",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Proxy user name"),
     .secret = true},
    ""};

inline constexpr StringSetting kProxyPassword{
    {.key = "proxy/password",
     .group = kNETWORK,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Proxy password"),
     .secret = true},
    ""};

// --------------------------------------------------------------- workspace

inline constexpr StringSetting kWorkspacePath{
    {.key = "workspace/path",
     .group = kWORKSPACE,
     .title = QT_TRANSLATE_NOOP("GpgFrontend::SettingItem", "Workspace folder"),
     .locality = kMachine},
    ""};

// ------------------------------------------------------------------ layout

inline constexpr StateSetting kKeyTabOrder{.key = "keys/key_tab_order",
                                           .group = kLAYOUT};
inline constexpr StateSetting kKeyMgmtTabOrder{.key = "keys/keymgmt_tab_order",
                                               .group = kLAYOUT};
inline constexpr StateSetting kToolboxTabOrder{.key = "keys/toolbox_tab_order",
                                               .group = kLAYOUT};
inline constexpr StateSetting kKeyGroupManageTabOrder{
    .key = "keys/keygroup_manage_tab_order", .group = kLAYOUT};
inline constexpr StateSetting kGlobalColumnsFilter{
    .key = "keys/global_columns_filter", .group = kLAYOUT};
inline constexpr StateSetting kToolboxColumnsFilter{
    .key = "keys/toolbox_columns_filter", .group = kLAYOUT};
inline constexpr StateSetting kCustomCategoryOrder{
    .key = "keys/custom_category_order", .group = kLAYOUT};

/// Every entry above. A setting missing from here is invisible to every
/// policy that walks the catalog, so a new entry is added in both places;
/// SettingCatalogTest checks that each key appears once.
inline constexpr std::array<const SettingMeta *, 49> kAll = {
    &kDefaultEngine,
    &kUseCustomGnuPGInstallPath,
    &kCustomGnuPGInstallPath,
    &kEnableGpgmeDebugLog,
    &kKillAllGnuPGDaemonAtClose,
    &kNonAsciiAtFileOperation,
    &kForbidAllGnuPGConnection,
    &kClearGpgPasswordCache,
    &kPasswordCacheTtl,
    &kPasswordCacheMaxTtl,
    &kPassphrasePromptTimeout,
    &kPassphraseRequesterGraceMs,
    &kArgon2S2kProfile,
    &kConfirmImportKeys,
    &kSingleRecipientMode,
    &kConfirmMultipleRecipients,
    &kRememberCheckedKeys,
    &kUndoableOperations,
    &kRestoreTextEditorPage,
    &kDefaultWorkspaceAs,
    &kFilePanelDefaultPathMode,
    &kShowTypeColumn,
    &kExpiringSoonDays,
    &kAutoImportMissingKey,
    &kModuleLoadingPolicy,
    &kLanguage,
    &kTheme,
    &kShowWizard,
    &kLogLevel,
    &kLogRingBufferCapacity,
    &kSecureLevel,
    &kAppKeyProtection,
    &kOSSecretStore,
    &kProfilePackageStorage,
    &kProhibitUpdateCheck,
    &kProxyEnable,
    &kProxyType,
    &kProxyHost,
    &kProxyPort,
    &kProxyUsername,
    &kProxyPassword,
    &kWorkspacePath,
    &kKeyTabOrder,
    &kKeyMgmtTabOrder,
    &kToolboxTabOrder,
    &kKeyGroupManageTabOrder,
    &kGlobalColumnsFilter,
    &kToolboxColumnsFilter,
    &kCustomCategoryOrder,
};

}  // namespace GpgFrontend::SettingItems

namespace GpgFrontend {

/**
 * @brief The catalog entry for a key.
 *
 * @param key a settings key, group included
 * @return the entry, or null for a key the catalog does not describe -- a
 * module's own settings, window geometry, anything written under a computed
 * key
 */
inline auto FindSetting(const QString &key) -> const SettingMeta * {
  for (const auto *meta : SettingItems::kAll) {
    if (key == QLatin1String(meta->key)) return meta;
  }
  return nullptr;
}

/// Every catalog entry in @p group, in catalog order.
inline auto SettingsInGroup(SettingGroup group) -> QList<const SettingMeta *> {
  QList<const SettingMeta *> out;
  for (const auto *meta : SettingItems::kAll) {
    if (meta->group == group) out.append(meta);
  }
  return out;
}

/// The groups a new profile can start with, in catalog order: those with at
/// least one inheritable entry.
inline auto InheritableSettingGroups() -> QList<SettingGroup> {
  QList<SettingGroup> out;
  for (const auto *meta : SettingItems::kAll) {
    if (!meta->inheritable) continue;
    if (!out.contains(meta->group)) out.append(meta->group);
  }
  return out;
}

/// What to call a group, translated.
inline auto SettingGroupTitle(SettingGroup group) -> QString {
  switch (group) {
    case SettingGroup::kENGINE:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "OpenPGP engine and GnuPG");
    case SettingGroup::kRPGP:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "rPGP passphrase handling");
    case SettingGroup::kBEHAVIOUR:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Everyday behaviour");
    case SettingGroup::kAPPEARANCE:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Language and appearance");
    case SettingGroup::kDIAGNOSTICS:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Diagnostics");
    case SettingGroup::kSECURITY:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Key protection");
    case SettingGroup::kNETWORK:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Network and proxy");
    case SettingGroup::kWORKSPACE:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Workspace");
    case SettingGroup::kLAYOUT:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Remembered layout");
  }
  return {};
}

/// One line on what a group covers, translated: what a person reads before
/// deciding whether a new profile should start with it.
inline auto SettingGroupDescription(SettingGroup group) -> QString {
  switch (group) {
    case SettingGroup::kENGINE:
      return QCoreApplication::translate(
          "GpgFrontend::SettingItem",
          "Which engine runs, which GnuPG it uses, and how GnuPG behaves.");
    case SettingGroup::kRPGP:
      return QCoreApplication::translate(
          "GpgFrontend::SettingItem",
          "How long passphrases are kept and how keys are derived.");
    case SettingGroup::kBEHAVIOUR:
      return QCoreApplication::translate(
          "GpgFrontend::SettingItem",
          "Confirmations, recipients, the editor and the file panel.");
    case SettingGroup::kAPPEARANCE:
      return QCoreApplication::translate(
          "GpgFrontend::SettingItem",
          "Language, theme and the first-run wizard.");
    case SettingGroup::kDIAGNOSTICS:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "How much is written to the log.");
    case SettingGroup::kSECURITY:
      return QCoreApplication::translate(
          "GpgFrontend::SettingItem",
          "How this profile's own key is protected.");
    case SettingGroup::kNETWORK:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Update checks and the proxy.");
    case SettingGroup::kWORKSPACE:
      return QCoreApplication::translate("GpgFrontend::SettingItem",
                                         "Where files are kept.");
    case SettingGroup::kLAYOUT:
      return QCoreApplication::translate(
          "GpgFrontend::SettingItem",
          "Remembered tab orders and column choices.");
  }
  return {};
}

}  // namespace GpgFrontend
