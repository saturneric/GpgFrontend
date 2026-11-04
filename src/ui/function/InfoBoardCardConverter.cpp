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

#include "ui/function/InfoBoardCardConverter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "ui/function/InfoBoardReport.h"

namespace GpgFrontend::UI {

namespace {

auto status_from_string(const QString& s) -> InfoBoardStatus {
  if (s == "ok"_L1) return kINFO_ERROR_OK;
  if (s == "warn"_L1) return kINFO_ERROR_WARN;
  if (s == "critical"_L1) return kINFO_ERROR_CRITICAL;
  return kINFO_ERROR_NEUTRAL;
}

auto source_kind_from_string(const QString& s) -> InfoBoardSource::Kind {
  if (s == "file"_L1) return InfoBoardSource::kFILE;
  if (s == "document"_L1) return InfoBoardSource::kDOCUMENT;
  if (s == "generic"_L1) return InfoBoardSource::kGENERIC;
  return InfoBoardSource::kOPERATION;
}

auto card_to_json(const InfoBoardCard& card) -> QJsonObject {
  QJsonArray fields;
  for (const auto& f : card.fields) {
    fields.append(QJsonArray{f.first, f.second});
  }
  QJsonObject obj{{u"title"_s, card.title},
                  {u"status"_s, SeverityId(card.status)},
                  {u"fields"_s, fields},
                  {u"kind"_s, CardKindId(card.kind)}};
  // Presentation hints, only when a field has any.
  QJsonArray meta;
  bool any_meta = false;
  for (qsizetype i = 0; i < card.fields.size(); ++i) {
    const auto m = card.MetaAt(i);
    any_meta = any_meta || !m.note.isEmpty() || m.technical ||
               m.group != InfoBoardFieldGroup::kDETAIL;
    meta.append(QJsonObject{{u"note"_s, m.note},
                            {u"technical"_s, m.technical},
                            {u"group"_s, static_cast<int>(m.group)}});
  }
  if (any_meta) obj.insert(u"meta"_s, meta);
  if (!card.source.IsEmpty()) {
    obj.insert(u"source"_s,
               QJsonObject{{u"kind"_s, SourceKindId(card.source.kind)},
                           {u"id"_s, card.source.id},
                           {u"label"_s, card.source.label},
                           {u"full"_s, card.source.full}});
  }
  if (card.outcome.has_value()) {
    obj.insert(u"outcome"_s, OutcomeId(*card.outcome));
  }
  return obj;
}

auto source_from_json(const QJsonValue& value) -> InfoBoardSource {
  InfoBoardSource source;
  if (value.isString()) {
    const auto s = value.toString();
    if (s.isEmpty()) return {};
    source.kind = InfoBoardSource::kOPERATION;
    source.id = source.label = source.full = s;
    return source;
  }
  if (!value.isObject()) return {};

  const auto obj = value.toObject();
  source.kind = source_kind_from_string(obj.value(u"kind"_s).toString());
  source.id = obj.value(u"id"_s).toString();
  source.label = obj.value(u"label"_s).toString();
  source.full = obj.value(u"full"_s).toString();
  // Whichever is given stands in for what is missing.
  if (source.id.isEmpty())
    source.id = source.full.isEmpty() ? source.label : source.full;
  if (source.label.isEmpty()) source.label = source.id;
  if (source.full.isEmpty()) source.full = source.label;
  return source;
}

auto card_from_json(const QJsonObject& obj) -> InfoBoardCard {
  InfoBoardCard card;
  card.title = obj.value(u"title"_s).toString();
  card.status = status_from_string(obj.value(u"status"_s).toString());
  card.kind = CardKindFromId(obj.value(u"kind"_s).toString());
  card.source = source_from_json(obj.value(u"source"_s));
  // Only a recognized outcome counts; anything else is no outcome at all.
  card.outcome = OutcomeFromId(obj.value(u"outcome"_s).toString());
  for (const auto& fv : obj.value(u"fields"_s).toArray()) {
    const auto pair = fv.toArray();
    if (pair.size() != 2) continue;
    card.fields.append({pair.at(0).toString(), pair.at(1).toString()});
  }
  const auto meta = obj.value(u"meta"_s).toArray();
  for (qsizetype i = 0; i < meta.size() && i < card.fields.size(); ++i) {
    const auto m = meta.at(i).toObject();
    const auto group =
        m.value(u"group"_s)
            .toInt(static_cast<int>(InfoBoardFieldGroup::kDETAIL));
    card.field_meta.append(
        {m.value(u"note"_s).toString(), m.value(u"technical"_s).toBool(),
         group >= 0 && group <= static_cast<int>(InfoBoardFieldGroup::kDETAIL)
             ? static_cast<InfoBoardFieldGroup>(group)
             : InfoBoardFieldGroup::kDETAIL});
  }
  return card;
}

}  // namespace

auto encode_info_board_cards(const QList<InfoBoardCard>& cards) -> QByteArray {
  QJsonArray arr;
  for (const auto& card : cards) arr.append(card_to_json(card));
  return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

auto decode_info_board_cards(const QByteArray& json) -> InfoBoardCardsPayload {
  InfoBoardCardsPayload payload;

  QJsonParseError err{};
  const auto doc = QJsonDocument::fromJson(json, &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject()) return payload;

  const auto obj = doc.object();
  payload.operation = obj.value(u"operation"_s).toString();
  payload.description = obj.value(u"description"_s).toString();
  payload.details_title = obj.value(u"details_title"_s).toString();
  for (const auto& item : obj.value(u"details_items"_s).toArray()) {
    payload.details_items.append(item.toString());
  }
  for (const auto& cv : obj.value(u"cards"_s).toArray()) {
    if (cv.isObject()) payload.cards.append(card_from_json(cv.toObject()));
  }

  payload.valid = true;
  return payload;
}

}  // namespace GpgFrontend::UI
