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

#include "core/model/GpgKeyGenerateInfo.h"

namespace GpgFrontend::UI {

/**
 * @brief Get the Algo By Id object
 *
 * @param id
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoById(const QString& id, const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief Search for algorithms by name and type
 *
 * @param name
 * @param type
 * @param algos
 * @return QContainer<KeyAlgo>
 */
auto SearchAlgoByNameType(const QString& name, const QString& type,
                          const QContainer<KeyAlgo>& algos)
    -> QContainer<KeyAlgo>;

/**
 * @brief Get the Algo By Name And Key Length object
 *
 * @param name
 * @param key_length
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoByNameTypeAndKeyLength(const QString& name, const QString& type,
                                   int key_length,
                                   const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief Get the Algo By Name object
 *
 * @param name
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoByNameType(const QString& name, const QString& type,
                       const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief Set the Key Length Combox Box By Algo object
 *
 * @param combo
 * @param algos
 */
void SetKeyLengthComboxBoxByAlgo(QComboBox* combo,
                                 const QContainer<KeyAlgo>& algos);

/**
 * @brief
 *
 * @param combo_box
 * @param algos
 */
void PopulateAlgoComboBox(QComboBox* combo_box,
                          const QContainer<KeyAlgo>& algos);

/**
 * @brief Append a non-selectable, visually distinct section header to a combo
 * box, used to group related entries (e.g. post-quantum algorithms/profiles).
 *
 * @param combo_box
 * @param text the (already translated) header label
 */
void AddComboSectionHeader(QComboBox* combo_box, const QString& text);

/**
 * @brief Get the Algo By Id And Type object
 *
 * If type is empty, this function falls back to id-only matching for backward
 * compatibility.
 *
 * @param id
 * @param type
 * @param algos
 * @return std::tuple<bool, KeyAlgo>
 */
auto GetAlgoByIdType(const QString& id, const QString& type,
                     const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

}  // namespace GpgFrontend::UI
