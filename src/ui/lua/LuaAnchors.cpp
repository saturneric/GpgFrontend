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

#include "LuaAnchors.h"

#include <array>

namespace GpgFrontend::UI::Lua {

namespace {

constexpr std::array<AnchorSpec, 11> kAnchors{{
    {"main.menu.file.workspace", AnchorKind::kMENU, kCTX_DOCUMENT, true, 1, 0,
     "File > Workspace: things that open a new workspace or document"},
    {"main.menu.advanced", AnchorKind::kMENU, kCTX_DOCUMENT, true, 1, 0,
     "Advanced: tools that act on the current document or the application"},
    {"main.menu.help", AnchorKind::kMENU, kCTX_DOCUMENT, true, 1, 0,
     "Help: information about the application and its environment"},
    {"main.menu.import_key", AnchorKind::kMENU, kCTX_NONE, true, 1, 0,
     "Keys > Import Key: further sources to import keys from"},
    {"main.menu.operations", AnchorKind::kMENU, kCTX_DOCUMENT, true, 2, 0,
     "Operations: further ways to protect the current document"},
    {"editor.context", AnchorKind::kMENU, kCTX_DOCUMENT | kCTX_SELECTION, true,
     1, 0, "the text editor's context menu"},
    {"key.details.actions", AnchorKind::kBUTTONS, kCTX_KEY, true, 1, 0,
     "the key details dialog's operations; one button per command category"},
    {"key.list.context", AnchorKind::kMENU, kCTX_KEY | kCTX_KEYS, true, 2, 0,
     "a key list's context menu; ctx.keys holds the selected keys"},
    {"settings", AnchorKind::kSETTINGS, kCTX_NONE, true, 1, 0,
     "a page in the Settings dialog; ui.anchor.settings{section=...}"},
    {"editor", AnchorKind::kEDITOR, kCTX_DOCUMENT, false, 1, 0,
     "the view for one document type; ui.anchor.editor{document_type=...}"},
    {"dialog", AnchorKind::kDIALOG, kCTX_NONE, true, 1, 0,
     "a dialog the module opens with org.gpgfrontend.view.open"},
}};

auto KindName(AnchorKind k) -> const char* {
  switch (k) {
    case AnchorKind::kMENU:
      return "menu";
    case AnchorKind::kBUTTONS:
      return "buttons";
    case AnchorKind::kSETTINGS:
      return "settings";
    case AnchorKind::kEDITOR:
      return "editor";
    case AnchorKind::kDIALOG:
      return "dialog";
  }
  return "?";
}

}  // namespace

auto FindAnchor(const QString& id) -> const AnchorSpec* {
  for (const auto& a : kAnchors) {
    if (id == QLatin1String(a.id)) return &a;
  }
  return nullptr;
}

auto AllAnchors() -> QList<const AnchorSpec*> {
  QList<const AnchorSpec*> all;
  for (const auto& a : kAnchors) all.append(&a);
  return all;
}

auto AnchorCatalogReference() -> QString {
  QStringList lines;
  lines << QString("anchor catalog v%1").arg(kAnchorCatalogVersion);
  for (const auto& a : kAnchors) {
    QStringList ctx;
    if ((a.context & kCTX_DOCUMENT) != 0) ctx << "document";
    if ((a.context & kCTX_SELECTION) != 0) ctx << "selection";
    if ((a.context & kCTX_KEY) != 0) ctx << "key";
    if ((a.context & kCTX_KEYS) != 0) ctx << "keys";
    lines << QString("%1 | %2 | context: %3 | modules: %4 | since %5%6")
                 .arg(QLatin1String(a.id), QLatin1String(KindName(a.kind)),
                      ctx.isEmpty() ? QStringLiteral("none") : ctx.join(", "),
                      a.multiple_modules ? QStringLiteral("many")
                                         : QStringLiteral("one"))
                 .arg(a.since)
                 .arg(a.deprecated_since == 0
                          ? QString()
                          : QString(" | deprecated since %1")
                                .arg(a.deprecated_since));
  }
  return lines.join('\n');
}

}  // namespace GpgFrontend::UI::Lua
