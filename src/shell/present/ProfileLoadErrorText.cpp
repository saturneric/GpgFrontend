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

#include "shell/present/ProfileLoadErrorText.h"

namespace GpgFrontend {

auto DescribeProfileLoadError(const ProfileLoadError& error)
    -> std::optional<ProfileLoadErrorText> {
  const auto subject = QDir::toNativeSeparators(error.subject);

  switch (error.failure) {
    case ProfileLoadFailure::kCANCELLED:
      // The user backed out on purpose; saying so again would be nagging.
      return std::nullopt;

    case ProfileLoadFailure::kSELECTION_INVALID:
      return ProfileLoadErrorText{QObject::tr("Cannot Open Profile"),
                                  error.detail};

    case ProfileLoadFailure::kALREADY_OPEN:
      return ProfileLoadErrorText{
          QObject::tr("Profile Is Already Open"),
          QObject::tr("This profile is already open in another window.") +
              "\n\n" + subject};

    case ProfileLoadFailure::kLOCK_UNAVAILABLE:
      return ProfileLoadErrorText{
          QObject::tr("Cannot Lock Profile"),
          QObject::tr("The lock file at %1 could not be created.")
                  .arg(subject) +
              "\n" + QObject::tr("Please check your storage and permissions.")};

    case ProfileLoadFailure::kNOT_A_PACKAGE:
    case ProfileLoadFailure::kPACKAGE_MALFORMED:
    case ProfileLoadFailure::kMOUNT_FAILED:
      return ProfileLoadErrorText{QObject::tr("Cannot Open Profile"),
                                  error.detail + "\n\n" + subject};

    case ProfileLoadFailure::kSTORAGE_UNAVAILABLE: {
      // Every reason verbatim. "This machine cannot" is not something anyone
      // can act on; "/dev/shm: not a RAM-backed filesystem" is, and it is the
      // difference between a bug report and a settings change.
      auto text = QObject::tr(
          "This profile is set to open only in storage that is not "
          "left readable on this disk, and none is available "
          "here.");
      if (!error.detail.isEmpty()) text += "\n\n" + error.detail;
      text += "\n\n" + QObject::tr(
                           "Change 'Profile package storage' in Settings to "
                           "allow an ordinary temporary folder.");

      return ProfileLoadErrorText{QObject::tr("Cannot Open Profile Safely"),
                                  text};
    }

    case ProfileLoadFailure::kSTORAGE_FULL:
      return ProfileLoadErrorText{
          QObject::tr("Not Enough Room"),
          QObject::tr("There was not enough room to unpack this profile.") +
              (error.detail.isEmpty()
                   ? QString()
                   : "\n\n" + QObject::tr("Storage: %1").arg(error.detail)) +
              "\n\n" +
              QObject::tr("The file itself is fine — free some space and try "
                          "again.")};

    case ProfileLoadFailure::kPACKAGE_TOO_LARGE:
      return ProfileLoadErrorText{
          QObject::tr("This Profile File Is Too Large"),
          QObject::tr("Opening a profile file needs to hold all of it in "
                      "memory at once, and this one is bigger than this "
                      "version can manage.") +
              (error.detail.isEmpty()
                   ? QString()
                   : "\n\n" +
                         QObject::tr("Size / limit: %1").arg(error.detail)) +
              "\n\n" +
              QObject::tr("Nothing has been unpacked. This version cannot "
                          "create a file this large either, so it was made "
                          "somewhere else or has been damaged.")};

    case ProfileLoadFailure::kPACKAGE_TAMPERED:
      return ProfileLoadErrorText{QObject::tr("This File Has Been Altered"),
                                  error.detail + "\n\n" + subject};

    case ProfileLoadFailure::kTOO_NEW:
    case ProfileLoadFailure::kUPGRADE_REFUSED: {
      const auto writer =
          error.actor.isEmpty() ? QObject::tr("a newer version") : error.actor;
      return ProfileLoadErrorText{
          error.failure == ProfileLoadFailure::kTOO_NEW
              ? QObject::tr("Profile Is Too New")
              : QObject::tr("Profile Cannot Be Opened"),
          QObject::tr("This application data was last used by %1, which stores "
                      "it in a format this version does not understand.")
                  .arg(writer) +
              "\n\n" + error.detail + "\n\n" +
              QObject::tr("Continuing would damage it. Please use %1 or later, "
                          "or start this version with a different profile.")
                  .arg(writer) +
              "\n\n" + QObject::tr("Data folder: %1").arg(subject)};
    }

    case ProfileLoadFailure::kUPGRADE_FAILED:
      return ProfileLoadErrorText{
          QObject::tr("Profile Upgrade Failed"),
          QObject::tr("Upgrading this profile stopped at step '%1'.")
                  .arg(error.actor) +
              "\n\n" + error.detail + "\n\n" +
              QObject::tr("The profile is intact. Please report this.")};

    case ProfileLoadFailure::kKEY_WRITE_FAILED:
      // Continuing would be silent data loss: everything encrypted during this
      // session gets a key ID that no longer exists on the next start, so it
      // would come back as unreadable rather than merely unsaved.
      return ProfileLoadErrorText{
          QObject::tr("Save Key Failed"),
          QObject::tr("The application secure key could not be saved: %1")
                  .arg(error.detail) +
              "\n" +
              QObject::tr("Anything saved now would be unreadable after a "
                          "restart, so the application will not continue. "
                          "Please check your storage and permissions.")};

    case ProfileLoadFailure::kKEY_DECRYPT_FAILED:
      // A wrong PIN is caught and retried before the key loader ever runs, so
      // reaching here means the key file itself will not decrypt.
      return ProfileLoadErrorText{
          QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. The key "
                      "file may be corrupted.") +
              "\n" + QObject::tr("Please clear the secure key and try again.")};

    case ProfileLoadFailure::kKEY_READ_FAILED:
      return ProfileLoadErrorText{
          QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1")
                  .arg(error.detail) +
              "\n" +
              QObject::tr(
                  "Please ensure the key file exists and is "
                  "accessible, or try re-initializing the secure key.")};

    case ProfileLoadFailure::kKEY_GENERATE_FAILED:
      return ProfileLoadErrorText{
          QObject::tr("Secure Key Generation Failed"),
          QObject::tr("Failed to generate an application secure key.") + "\n" +
              QObject::tr("Please check your system's cryptography support.")};

    case ProfileLoadFailure::kKEY_IO_FAILED:
      return ProfileLoadErrorText{
          QObject::tr("App Secure Key Error"),
          QObject::tr("The application secure key at %1 could not be read or "
                      "rewritten.")
                  .arg(error.detail) +
              "\n" + QObject::tr("Please check your storage and permissions.")};

    case ProfileLoadFailure::kKEY_LOCKED_OUT:
      // The offer to reset was already made and declined; quitting leaves the
      // key intact so the keychain can be unlocked and the start retried.
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace GpgFrontend
