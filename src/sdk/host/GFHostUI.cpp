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

#include <core/utils/CommonUtils.h>

#include <QApplication>
#include <QMap>
#include <QObject>
#include <QString>

#include "GFHostImpl.h"
#include "private/GFHostContext.h"
#include "private/GFSDKPrivate.h"
#include "ui/UIModuleManager.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/TextEdit.h"

namespace gf_host {

auto GFUIDefaultUserFilePath() -> char* {
  return GFStrDup(GpgFrontend::UI::GetDefaultUserFilePath());
}

auto GFUIPaletteColor(int role) -> uint32_t {
  const auto p = QApplication::palette();
  switch (role) {
    case GF_UI_COLOR_MUTED_TEXT:
      return GpgFrontend::UI::MutedTextColor(p).rgba();
    case GF_UI_COLOR_BORDER:
      return GpgFrontend::UI::BorderColor(p).rgba();
    case GF_UI_COLOR_WARNING:
      return GpgFrontend::UI::WarningColor(p).rgba();
    case GF_UI_COLOR_DANGER:
      return GpgFrontend::UI::DangerColor(p).rgba();
    case GF_UI_COLOR_ACCENT_POSITIVE:
      return GpgFrontend::UI::AccentColor(p, true).rgba();
    case GF_UI_COLOR_ACCENT_NEGATIVE:
      return GpgFrontend::UI::AccentColor(p, false).rgba();
    default:
      return 0U;
  }
}

auto GFUITakeCurrentEditorContent() -> GFBufferRef {
  const auto bytes = GpgFrontend::UI::CurrentEditorContent();
  if (!bytes.has_value()) return nullptr;
  return GFBufferNewFromBytes(bytes->constData(),
                              static_cast<size_t>(bytes->size()));
}

}  // namespace gf_host