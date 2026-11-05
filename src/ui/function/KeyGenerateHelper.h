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

#include "core/model/DataObject.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "shell/rules/KeyGenProfile.h"

namespace GpgFrontend::UI {

/**
 * @brief Get the Algo By Id object
 *
 * @param id
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoById(const QString& id, const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief Search for algorithms by name and type
 *
 * @param name
 * @param type
 * @param algos
 * @return QList<KeyAlgo>
 */
auto SearchAlgoByNameType(const QString& name, const QString& type,
                          const QList<KeyAlgo>& algos) -> QList<KeyAlgo>;

/**
 * @brief Get the Algo By Name And Key Length object
 *
 * @param name
 * @param key_length
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoByNameTypeAndKeyLength(const QString& name, const QString& type,
                                   int key_length, const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief Get the Algo By Name object
 *
 * @param name
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoByNameType(const QString& name, const QString& type,
                       const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief Set the Key Length Combox Box By Algo object
 *
 * @param combo
 * @param algos
 */
void SetKeyLengthComboxBoxByAlgo(QComboBox* combo, const QList<KeyAlgo>& algos);

/**
 * @brief
 *
 * @param combo_box
 * @param algos
 */
void GF_UI_EXPORT PopulateAlgoComboBox(QComboBox* combo_box,
                                       const QList<KeyAlgo>& algos);

/**
 * @brief Append a non-selectable, visually distinct section header to a combo
 * box, used to group related entries (e.g. post-quantum algorithms/profiles).
 *
 * @param combo_box
 * @param text the (already translated) header label
 */
void AddComboSectionHeader(QComboBox* combo_box, const QString& text);

/**
 * @brief Return the index of the first selectable item in a combo box.
 *
 * A combo box populated by PopulateAlgoComboBox may begin with a non-selectable
 * section header (e.g. "ECC" when every algorithm is an elliptic curve), so
 * index 0 is not necessarily a valid default. Use this to pick a real default
 * instead of blindly selecting index 0.
 *
 * @param combo_box
 * @return the first index whose item is selectable and enabled, or -1 if none
 */
auto FirstSelectableComboIndex(const QComboBox* combo_box) -> int;

/**
 * @brief Ask the user to confirm an unusually short user id name.
 *
 * A short name is perfectly legal in an OpenPGP user id, but it is often a
 * typo, so warn instead of refusing. Names of five characters or more are
 * accepted silently.
 *
 * @param parent
 * @param name the already trimmed name
 * @return true if generation should continue
 */
auto ConfirmShortUserIdName(QWidget* parent, const QString& name) -> bool;

/**
 * @brief The engine's reason for a failed key or subkey generation, from the
 * operation's result data. Empty when the engine gave none (GnuPG).
 *
 * @param data_obj the data object the generation callback received
 * @return QString
 */
auto KeyGenerationErrorDetail(const DataObjectPtr& data_obj) -> QString;

/**
 * @brief The fingerprint of the key a generation made, from its result data.
 * Empty when the engine gave none.
 */
auto KeyGenerationFingerprint(const DataObjectPtr& data_obj) -> QString;

/**
 * @brief Generate a key, and its subkey if there is one, waiting on @p parent.
 *
 * Reports a failure itself, over @p parent, and asks the key lists to reload on
 * success.
 *
 * @param parent what the waiting dialog and any error belong to
 * @param channel the key database
 * @param key the primary key, with its user id set
 * @param subkey the subkey, or null
 * @return the new key's fingerprint (possibly empty), or nothing on failure
 */
auto RunKeyGeneration(QWidget* parent, int channel,
                      const QSharedPointer<KeyGenerateInfo>& key,
                      const QSharedPointer<KeyGenerateInfo>& subkey)
    -> std::optional<QString>;

/**
 * @brief Say that a key was made, naming it.
 */
void ShowKeyGenerated(QWidget* parent, const QString& uid,
                      const QString& fingerprint);

/**
 * @brief Make a key in one go, from the default profile and the user id
 * pattern in Settings.
 *
 * When the default profile cannot be made here, or the pattern does not make a
 * user id, offers the wizard instead.
 *
 * @param channel the key database
 * @param parent the window it is done from
 */
void QuickGenerateKey(int channel, QWidget* parent);

// What a new subkey is for, and the algorithm to suggest for it: in the core
// (core/function/KeyGenProfile.h), which the phone application shares.
using GpgFrontend::NormalizeKeyAlgoId;
using GpgFrontend::SubkeyPurpose;
using GpgFrontend::SuggestSubkeyAlgo;

}  // namespace GpgFrontend::UI
