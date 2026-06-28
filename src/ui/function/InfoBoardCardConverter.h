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

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "ui/widgets/InfoBoardWidget.h"

namespace GpgFrontend::UI {

auto GF_UI_EXPORT convert_op_info_to_cards(
    const GpgFrontend::GpgOpResultInfo& info) -> QContainer<InfoBoardCard>;

// Serialize cards to a JSON array string. Used by the SDK to hand structured
// results to modules, which can only pass plain string key/value params; the
// receiving UI slot decodes them back into cards so the Info Board never has to
// parse a human-readable report.
auto GF_UI_EXPORT
encode_info_board_cards(const QContainer<InfoBoardCard>& cards) -> QByteArray;

// Decoded form of a module's `result_cards` payload. `valid` is false when the
// JSON was absent or malformed, in which case the caller should fall back to
// the plain-text status path.
struct InfoBoardCardsPayload {
  bool valid = false;
  QString operation;
  QString description;
  QString details_title;
  QStringList details_items;
  QContainer<InfoBoardCard> cards;
};

// Decode a `result_cards` JSON object (see encode_info_board_cards for the card
// shape; the object also carries operation/description/details metadata).
auto decode_info_board_cards(const QByteArray& json) -> InfoBoardCardsPayload;

}  // namespace GpgFrontend::UI
