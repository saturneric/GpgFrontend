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

namespace GpgFrontend::UI {

/**
 * @brief Everything the gating rules are allowed to look at.
 *
 * No widgets, no QAction, no gpgme: the rules deciding what a user can do to a
 * key are worth being able to test, and they cannot be while they are tangled
 * up with the things they act on.
 */
struct KeyActionContext {
  int selected_count = 0;  ///< rows highlighted in the current tab
  int checked_count = 0;   ///< boxes ticked, across every tab

  // Properties of the single selected key. Only meaningful when
  // selected_count == 1; the rules never consult them otherwise.
  bool selection_is_gpg_key = false;  ///< false for a key group
  bool selection_is_private = false;  ///< the user owns this key
  bool selection_has_email = false;   ///< the key carries an email address

  // What the engine behind the current key database can do.
  bool sign_supported = false;
  bool owner_trust_supported = false;
  bool subkey_generation_supported = false;
  bool ssh_export_supported = false;

  // Whether the key-server module is loaded and listening.
  bool keyserver_search_available = false;
  bool keyserver_upload_available = false;
  bool keyserver_fetch_available = false;

  bool any_target_private_key = false;      ///< any target key is one we own
  bool any_private_key_in_keyring = false;  ///< the keyring holds any at all
};

/**
 * @brief Every action the Key Management window gates on the selection.
 */
enum class KeyAction {
  kShowDetails,
  kCopyFingerprint,
  kCopyKeyId,
  kCopyEmail,
  kCopyPublicKey,
  kCertify,
  kSetExpiry,
  kGenerateSubkey,
  kSetOwnerTrust,
  kGenerateRevokeCert,
  kDeleteSelected,
  kDeleteChecked,
  kExportPackage,
  kExportClipboard,
  kExportOpenSsh,
  kExportPublicKey,
  kExportPrivateKey,
  kKeyserverSearch,
  kKeyserverPublish,
  kKeyserverRefresh,
  kBulkSetOwnerTrust,
  kBulkExtendExpiry,
  kBackupAllPrivate,
  kCategory,
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
/**
 * @brief Hash a KeyAction, so it can key a QHash.
 *
 * Qt 6 hashes any enum through a generic overload; Qt 5 has none, and a scoped
 * enum does not convert to int on its own to reach the integral ones.
 */
inline auto qHash(KeyAction action, uint seed = 0) -> uint {
  return ::qHash(static_cast<int>(action), seed);
}
#endif

/**
 * @brief Every KeyAction enumerator, for callers that need to walk them all.
 */
auto GF_UI_EXPORT AllKeyActions() -> const QVector<KeyAction>&;

/**
 * @brief What one action should look like right now.
 */
struct KeyActionState {
  /// Whether this build and this key database can perform the action at all.
  /// False hides the entry outright — offering something the engine cannot do
  /// is worse than not mentioning it.
  bool supported = true;

  /// Whether it applies to what is currently selected. A false here greys the
  /// entry rather than hiding it, so the menu bar keeps a stable shape and the
  /// user can learn what exists.
  bool enabled = false;

  /// Why it is unavailable, appended to the tooltip. Empty when enabled.
  QString reason;
};

/**
 * @brief Decide the state of one action from the current context.
 *
 * The single source of truth for the menu bar, the tool bar and the context
 * menu alike, so the three can never disagree about whether an operation makes
 * sense.
 *
 * @param action the action to evaluate
 * @param ctx what is selected, checked and supported
 * @return whether it is supported, whether it is enabled, and why not
 */
auto GF_UI_EXPORT EvaluateKeyAction(KeyAction action,
                                    const KeyActionContext& ctx)
    -> KeyActionState;

/**
 * @brief How many keys a multi-key operation would act on.
 *
 * The checked set when anything is checked, otherwise the selection. Written
 * once here so the gating rules and the slots that do the work cannot drift
 * into disagreeing about what "the target keys" means.
 *
 * @param ctx current context
 * @return number of keys the operation would touch
 */
auto GF_UI_EXPORT EffectiveTargetCount(const KeyActionContext& ctx) -> int;

}  // namespace GpgFrontend::UI
