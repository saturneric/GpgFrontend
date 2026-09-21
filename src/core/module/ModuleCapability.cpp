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

#include "ModuleCapability.h"

#include <QMap>

namespace GpgFrontend::Module {

namespace {

/**
 * @brief The whole vocabulary, in one place.
 *
 * One table rather than a switch per question, so that adding a capability is
 * one line and cannot be half-added -- the name, its kind and its bit are
 * decided together or not at all.
 */
struct CapabilityEntry {
  const char* name;
  ModuleCapabilityKind kind;
  uint32_t bit;  ///< 0 for an advisory name, which has no bit by definition
};

auto Vocabulary() -> const QList<CapabilityEntry>& {
  static const QList<CapabilityEntry> kVocabulary = {
      {"gpg", ModuleCapabilityKind::kENFORCEABLE,
       static_cast<uint32_t>(ModuleCapability::kGPG)},
      {"pgp", ModuleCapabilityKind::kENFORCEABLE,
       static_cast<uint32_t>(ModuleCapability::kPGP)},
      {"ui", ModuleCapabilityKind::kENFORCEABLE,
       static_cast<uint32_t>(ModuleCapability::kUI)},
      {"editor", ModuleCapabilityKind::kENFORCEABLE,
       static_cast<uint32_t>(ModuleCapability::kEDITOR)},
      {"storage", ModuleCapabilityKind::kENFORCEABLE,
       static_cast<uint32_t>(ModuleCapability::kSTORAGE)},
      {"process", ModuleCapabilityKind::kENFORCEABLE,
       static_cast<uint32_t>(ModuleCapability::kPROCESS)},

      // Advisory. A module reaches the network through Qt, not through the
      // Host, so there is nothing here for the Host to withhold. Recorded and
      // shown because a person deciding whether to trust an external module
      // wants to know -- not because it is a gate.
      {"network", ModuleCapabilityKind::kADVISORY, 0},
  };
  return kVocabulary;
}

auto Find(const QString& name) -> const CapabilityEntry* {
  for (const auto& entry : Vocabulary()) {
    if (name == QLatin1String(entry.name)) return &entry;
  }
  return nullptr;
}

auto NamesOfKind(ModuleCapabilityKind kind) -> QStringList {
  QStringList names;
  for (const auto& entry : Vocabulary()) {
    if (entry.kind == kind) names.append(QLatin1String(entry.name));
  }
  names.sort();
  return names;
}

}  // namespace

auto ModuleCapabilityKindOf(const QString& name) -> ModuleCapabilityKind {
  const auto* entry = Find(name);
  return entry == nullptr ? ModuleCapabilityKind::kUNKNOWN : entry->kind;
}

auto ModuleCapabilityMask(const QStringList& declared) -> uint32_t {
  uint32_t mask = 0;
  for (const auto& name : declared) {
    const auto* entry = Find(name);
    if (entry == nullptr) continue;
    mask |= entry->bit;
  }
  return mask;
}

auto AdvisoryDeclarationsOf(const QStringList& declared) -> QStringList {
  QStringList advisory;
  for (const auto& name : declared) {
    const auto* entry = Find(name);
    if (entry != nullptr && entry->kind == ModuleCapabilityKind::kADVISORY) {
      advisory.append(name);
    }
  }
  return advisory;
}

auto EnforceableCapabilityNames() -> QStringList {
  return NamesOfKind(ModuleCapabilityKind::kENFORCEABLE);
}

auto AdvisoryCapabilityNames() -> QStringList {
  return NamesOfKind(ModuleCapabilityKind::kADVISORY);
}

auto ModuleCapabilityMaskToString(uint32_t mask) -> QString {
  QStringList granted;
  for (const auto& entry : Vocabulary()) {
    if (entry.bit != 0 && (mask & entry.bit) != 0) {
      granted.append(QLatin1String(entry.name));
    }
  }
  if (granted.isEmpty()) return QStringLiteral("none");
  granted.sort();
  return granted.join(QStringLiteral(", "));
}

}  // namespace GpgFrontend::Module
