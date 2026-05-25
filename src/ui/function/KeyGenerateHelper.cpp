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

#include "KeyGenerateHelper.h"

namespace GpgFrontend::UI {

auto GetAlgoById(const QString& id, const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Id() != id) continue;
    return {true, algo};
  }

  return {};
}

auto SearchAlgoByNameType(const QString& name, const QString& type,
                          const QContainer<KeyAlgo>& algos)
    -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> res;

  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.Type() != type) continue;
    res.append(algo);
  }

  return res;
}

auto GetAlgoByNameTypeAndKeyLength(const QString& name, const QString& type,
                                   int key_length,
                                   const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.Type() != type) continue;
    if (algo.KeyLength() != key_length) continue;
    return {true, algo};
  }

  return {};
}

auto GetAlgoByNameType(const QString& name, const QString& type,
                       const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.Type() != type) continue;
    return {true, algo};
  }

  return {};
}

void SetKeyLengthComboxBoxByAlgo(QComboBox* combo,
                                 const QContainer<KeyAlgo>& algos) {
  if (combo == nullptr) return;

  combo->clear();

  QSet<int> added_lengths;

  for (const auto& algo : algos) {
    const auto key_length = algo.KeyLength();
    if (key_length <= 0) continue;

    if (added_lengths.contains(key_length)) continue;
    added_lengths.insert(key_length);

    combo->addItem(QString::number(key_length), key_length);
  }
}

void PopulateAlgoComboBox(QComboBox* combo_box,
                          const QContainer<KeyAlgo>& algos) {
  if (combo_box == nullptr) return;

  auto sorted_algos = algos;

  std::sort(sorted_algos.begin(), sorted_algos.end(),
            [](const KeyAlgo& a, const KeyAlgo& b) -> bool {
              if (a.Id() == "none" && b.Id() != "none") return true;
              if (b.Id() == "none" && a.Id() != "none") return false;

              if (a.Name() != b.Name()) return a.Name() < b.Name();
              if (a.Type() != b.Type()) return a.Type() < b.Type();
              if (a.KeyLength() != b.KeyLength()) {
                return a.KeyLength() < b.KeyLength();
              }
              return a.Id() < b.Id();
            });

  combo_box->clear();

  QString last_label;
  QString last_type;

  for (const auto& algo : sorted_algos) {
    QString label;

    if (algo.Id() == "none") {
      label = algo.Name();
    } else {
      label = QString("%1 (%2)").arg(algo.Name(), algo.Type());
    }

    if (label == last_label && algo.Type() == last_type) continue;
    last_label = label;
    last_type = algo.Type();

    QVariantMap data;
    data["id"] = algo.Id();
    data["name"] = algo.Name();
    data["type"] = algo.Type();
    data["length"] = algo.KeyLength();

    combo_box->addItem(label, data);
  }
}

auto GetAlgoByIdType(const QString& id, const QString& type,
                     const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Id() != id) continue;
    if (!type.isEmpty() && algo.Type() != type) continue;
    return {true, algo};
  }

  return {};
}

}  // namespace GpgFrontend::UI
