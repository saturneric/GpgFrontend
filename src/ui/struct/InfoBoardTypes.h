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

// The Status Panel's value types. They live apart from InfoBoardWidget on
// purpose: lupdate mis-tracks the namespace across a typed enum followed by a
// Q_OBJECT class in one file, which used to force an awkward ordering there.

namespace GpgFrontend::UI {

/// The indicator's colour, stamp and status word.
enum InfoBoardStatus : uint8_t {
  kINFO_ERROR_OK = 0,
  kINFO_ERROR_WARN = 1,
  kINFO_ERROR_CRITICAL = 2,
  kINFO_ERROR_NEUTRAL = 3,
};

/**
 * @brief What happened to one object of a report.
 *
 * The first five describe an engine run as well as a task; the last two only
 * a task: no result arrived, or the engine finished but its output was
 * discarded because another item in the batch did not succeed.
 */
enum class InfoBoardOutcome : std::uint8_t {
  kSUCCEEDED,
  kWARNING,
  kFAILED,
  kCANCELLED,
  kNOT_SUPPORTED,
  kNOT_REPORTED,
  kNOT_SAVED,
};

/// A card's role; it decides the card's place within its object's group.
enum class InfoBoardCardKind : std::uint8_t {
  kOUTCOME,
  kCODEC,
  kSIGNATURE,
  kNEW_SIGNATURE,
  kINVALID_SIGNER,
  kMESSAGE_METADATA,
  kRECIPIENT,
  kFILE_HASH,
  kGENERIC,
};

/**
 * @brief What a card or object is about.
 *
 * @ref id is the identity (grouping and matching only), @ref label the short
 * text in the card header, @ref full the complete path or description for the
 * tooltip and every export. Two files with the same name differ by id.
 */
struct InfoBoardSource {
  enum Kind : std::uint8_t { kFILE, kDOCUMENT, kOPERATION, kGENERIC };

  Kind kind = kGENERIC;
  QString id;
  QString label;
  QString full;

  [[nodiscard]] auto IsEmpty() const -> bool { return id.isEmpty(); }
};

/// Which run of a card's fields a field belongs to; runs are spaced apart.
enum class InfoBoardFieldGroup : std::uint8_t {
  kPRIMARY,     ///< what happened: result, reason, message
  kDIAGNOSTIC,  ///< why, in the engine's or the system's own words
  kFOLLOW_UP,   ///< what became of the output, what to do next
  kDETAIL,      ///< everything else: hashes, keys, metadata
};

/// What a field is, where the presentation needs to know.
enum class InfoBoardFieldRole : std::uint8_t {
  kNONE,
  kREASON,  ///< the Outcome card's explanation
};

/// How one field is presented. Never what it says.
struct InfoBoardFieldMeta {
  /// Secondary text under the value, e.g. the stage and code of an error.
  QString note;
  /// A hash, code or identifier: shown in the monospaced face.
  bool technical = false;
  InfoBoardFieldGroup group = InfoBoardFieldGroup::kDETAIL;
  InfoBoardFieldRole role = InfoBoardFieldRole::kNONE;
  /// The report's summary or headline already says this; the screen may
  /// leave it out. Text exports always keep it.
  bool summarized = false;
};

struct InfoBoardCard {
  QString title;
  /// Styling only: never changes what the report says happened.
  InfoBoardStatus status{kINFO_ERROR_NEUTRAL};
  QList<QPair<QString, QString>> fields;
  /// Presentation of fields[i]; may be shorter than fields, the rest
  /// default.
  QList<InfoBoardFieldMeta> field_meta;
  InfoBoardCardKind kind = InfoBoardCardKind::kGENERIC;
  /// Empty: the posting's own source.
  InfoBoardSource source;
  /// A structured outcome; meaningful only on a kOUTCOME card, where it is
  /// what makes the card a valid Outcome card.
  std::optional<InfoBoardOutcome> outcome;

  void AddField(const QString& key, const QString& value,
                const InfoBoardFieldMeta& meta = {}) {
    field_meta.resize(fields.size());
    fields.append({key, value});
    field_meta.append(meta);
  }

  void PrependField(const QString& key, const QString& value,
                    const InfoBoardFieldMeta& meta = {}) {
    field_meta.resize(fields.size());
    fields.prepend({key, value});
    field_meta.prepend(meta);
  }

  [[nodiscard]] auto MetaAt(qsizetype i) const -> InfoBoardFieldMeta {
    return i < field_meta.size() ? field_meta.at(i) : InfoBoardFieldMeta{};
  }
};

}  // namespace GpgFrontend::UI
