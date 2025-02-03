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

auto SearchAlgoByName(const QString& name,
                      const QContainer<KeyAlgo>& algos) -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> res;

  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    res.append(algo);
  }

  return res;
}

auto GetAlgoByNameAndKeyLength(const QString& name, int key_length,
                               const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.KeyLength() != key_length) continue;
    return {true, algo};
  }

  return {};
}

auto GetAlgoByName(const QString& name, const QContainer<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    return {true, algo};
  }

  return {};
}

void SetKeyLengthComboxBoxByAlgo(QComboBox* combo,
                                 const QContainer<KeyAlgo>& algos) {
  combo->clear();

  QContainer<KeyAlgo> sorted_algos(algos.begin(), algos.end());
  std::sort(sorted_algos.begin(), sorted_algos.end(),
            [](const KeyAlgo& a, const KeyAlgo& b) {
              return a.KeyLength() < b.KeyLength();
            });

  QStringList key_lengths;
  for (const auto& algo : sorted_algos) {
    key_lengths.append(QString::number(algo.KeyLength()));
  }

  combo->addItems(key_lengths);
}
}  // namespace GpgFrontend::UI