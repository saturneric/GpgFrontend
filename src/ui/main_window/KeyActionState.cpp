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

#include "ui/main_window/KeyActionState.h"

namespace GpgFrontend::UI {

namespace {

auto Ok() -> KeyActionState { return {true, true, {}}; }

auto Blocked(const QString& reason) -> KeyActionState {
  return {true, false, reason};
}

auto Unsupported() -> KeyActionState { return {false, false, {}}; }

auto NoSelection() -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::KeyActionState",
                                     "Select a key first.");
}

auto SingleKeyOnly() -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::KeyActionState",
                                     "Select exactly one key for this.");
}

auto GpgKeysOnly() -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::KeyActionState",
                                     "Only available for a key, not a key "
                                     "group.");
}

auto OwnKeysOnly() -> QString {
  return QCoreApplication::translate(
      "GpgFrontend::UI::KeyActionState",
      "Only available for a key you own the private half of.");
}

auto NoTargets() -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::KeyActionState",
                                     "Check or select at least one key first.");
}

/**
 * @brief Whether exactly one real key — not a key group — is selected.
 *
 * The state shared by every operation that opens a dialog about one key.
 */
auto SingleGpgKey(const KeyActionContext& ctx) -> KeyActionState {
  if (ctx.selected_count == 0) return Blocked(NoSelection());
  if (ctx.selected_count > 1) return Blocked(SingleKeyOnly());
  if (!ctx.selection_is_gpg_key) return Blocked(GpgKeysOnly());
  return Ok();
}

/**
 * @brief SingleGpgKey(), plus the key having a private half.
 */
auto SingleOwnKey(const KeyActionContext& ctx) -> KeyActionState {
  const auto base = SingleGpgKey(ctx);
  if (!base.enabled) return base;
  if (!ctx.selection_is_private) return Blocked(OwnKeysOnly());
  return Ok();
}

/**
 * @brief Enabled when at least one key is checked or selected.
 */
auto AnyTarget(const KeyActionContext& ctx) -> KeyActionState {
  return EffectiveTargetCount(ctx) > 0 ? Ok() : Blocked(NoTargets());
}

}  // namespace

auto AllKeyActions() -> const QVector<KeyAction>& {
  static const QVector<KeyAction> kActions = {
      KeyAction::kShowDetails,       KeyAction::kCopyFingerprint,
      KeyAction::kCopyKeyId,         KeyAction::kCopyEmail,
      KeyAction::kCopyPublicKey,     KeyAction::kCertify,
      KeyAction::kSetExpiry,         KeyAction::kGenerateSubkey,
      KeyAction::kSetOwnerTrust,     KeyAction::kGenerateRevokeCert,
      KeyAction::kDeleteSelected,    KeyAction::kDeleteChecked,
      KeyAction::kExportPackage,     KeyAction::kExportClipboard,
      KeyAction::kExportOpenSsh,     KeyAction::kExportPublicKey,
      KeyAction::kExportPrivateKey,  KeyAction::kKeyserverSearch,
      KeyAction::kKeyserverPublish,  KeyAction::kKeyserverRefresh,
      KeyAction::kBulkSetOwnerTrust, KeyAction::kBulkExtendExpiry,
      KeyAction::kBackupAllPrivate,  KeyAction::kCategory,
  };
  return kActions;
}

auto EffectiveTargetCount(const KeyActionContext& ctx) -> int {
  return ctx.checked_count > 0 ? ctx.checked_count : ctx.selected_count;
}

auto EvaluateKeyAction(KeyAction action, const KeyActionContext& ctx)
    -> KeyActionState {
  switch (action) {
    // --- one key, any kind, including a key group -------------------------
    case KeyAction::kShowDetails:
    case KeyAction::kCopyFingerprint:
    case KeyAction::kCopyKeyId:
      if (ctx.selected_count == 0) return Blocked(NoSelection());
      if (ctx.selected_count > 1) return Blocked(SingleKeyOnly());
      return Ok();

    case KeyAction::kCopyEmail: {
      if (ctx.selected_count == 0) return Blocked(NoSelection());
      if (ctx.selected_count > 1) return Blocked(SingleKeyOnly());
      if (!ctx.selection_has_email) {
        return Blocked(
            QCoreApplication::translate("GpgFrontend::UI::KeyActionState",
                                        "This key carries no email address."));
      }
      return Ok();
    }

    // --- any number of selected keys --------------------------------------
    case KeyAction::kCopyPublicKey:
    case KeyAction::kDeleteSelected:
      // Armored blocks concatenate and deletion takes a list, so several
      // selected keys are as workable as one.
      return ctx.selected_count > 0 ? Ok() : Blocked(NoSelection());

    // --- one key we own ----------------------------------------------------
    case KeyAction::kSetExpiry:
    case KeyAction::kGenerateRevokeCert:
    case KeyAction::kExportPrivateKey:
      return SingleOwnKey(ctx);

    case KeyAction::kExportPublicKey:
      // One armored file per key, so one key at a time — the key package
      // export next to it is the way to save several at once.
      return SingleGpgKey(ctx);

    case KeyAction::kGenerateSubkey:
      if (!ctx.subkey_generation_supported) return Unsupported();
      return SingleOwnKey(ctx);

    // --- one key belonging to somebody else -------------------------------
    case KeyAction::kCertify: {
      if (!ctx.sign_supported) return Unsupported();

      const auto base = SingleGpgKey(ctx);
      if (!base.enabled) return base;
      if (ctx.selection_is_private) {
        // Certifying your own key says nothing: gpg already trusts it
        // ultimately by virtue of you holding the private half.
        return Blocked(QCoreApplication::translate(
            "GpgFrontend::UI::KeyActionState",
            "Certifying is for vouching for someone else's key."));
      }
      return Ok();
    }

    case KeyAction::kSetOwnerTrust:
      if (!ctx.owner_trust_supported) return Unsupported();
      return SingleGpgKey(ctx);

    // --- checked-else-selected --------------------------------------------
    case KeyAction::kExportPackage:
    case KeyAction::kExportClipboard:
    case KeyAction::kCategory:
      return AnyTarget(ctx);

    case KeyAction::kDeleteChecked:
      // The one action that means *checked* and nothing else; its counterpart
      // for the selection sits right next to it in the menu.
      return ctx.checked_count > 0
                 ? Ok()
                 : Blocked(QCoreApplication::translate(
                       "GpgFrontend::UI::KeyActionState",
                       "Tick the box beside at least one key first."));

    case KeyAction::kExportOpenSsh: {
      if (!ctx.ssh_export_supported) return Unsupported();
      // OpenSSH holds one key per file.
      const auto targets = EffectiveTargetCount(ctx);
      if (targets == 0) return Blocked(NoTargets());
      if (targets > 1) return Blocked(SingleKeyOnly());
      return Ok();
    }

    // --- key server --------------------------------------------------------
    case KeyAction::kKeyserverSearch:
      // Searching needs no selection: it is how you find a key you do not have
      // yet. A selected key only seeds the search box.
      return ctx.keyserver_search_available ? Ok() : Unsupported();

    case KeyAction::kKeyserverPublish:
      if (!ctx.keyserver_upload_available) return Unsupported();
      return AnyTarget(ctx);

    case KeyAction::kKeyserverRefresh:
      if (!ctx.keyserver_fetch_available) return Unsupported();
      return AnyTarget(ctx);

    // --- bulk --------------------------------------------------------------
    case KeyAction::kBulkSetOwnerTrust:
      if (!ctx.owner_trust_supported) return Unsupported();
      return AnyTarget(ctx);

    case KeyAction::kBulkExtendExpiry: {
      const auto base = AnyTarget(ctx);
      if (!base.enabled) return base;
      if (!ctx.any_target_private_key) {
        return Blocked(QCoreApplication::translate(
            "GpgFrontend::UI::KeyActionState",
            "Expiry can only be changed on keys you own."));
      }
      return Ok();
    }

    case KeyAction::kBackupAllPrivate:
      // Keyring-wide, so it ignores the selection entirely.
      return ctx.any_private_key_in_keyring
                 ? Ok()
                 : Blocked(QCoreApplication::translate(
                       "GpgFrontend::UI::KeyActionState",
                       "This keyring holds no private keys."));
  }

  return Ok();
}

}  // namespace GpgFrontend::UI
