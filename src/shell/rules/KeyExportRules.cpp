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

#include "shell/rules/KeyExportRules.h"

namespace GpgFrontend::Shell {

namespace {

/**
 * @brief Strip what no filesystem will accept in a name.
 *
 * A user ID may legitimately contain a slash or a colon, and substituting one
 * straight into the template produced a default filename the save dialog would
 * reject — on every platform, not just Windows.
 */
auto SanitiseFileNameField(const QString& field) -> QString {
  QString out;
  out.reserve(field.size());

  for (const auto c : field) {
    if (c == '/' || c == '\\' || c == ':' || c == '"' || c == '|' || c == '?' ||
        c == '*' || c == '<' || c == '>' ||
        c.category() == QChar::Other_Control) {
      continue;
    }
    out.append(c == ' ' ? QChar('_') : c);
  }
  return out.trimmed();
}

}  // namespace

auto KeyExportOptionsFor(KeyExportKind kind) -> KeyExportOptions {
  switch (kind) {
    case KeyExportKind::kPublic:
      return {.secret = false, .shortest = false, .type = u"pub"_s};
    case KeyExportKind::kShortSecret:
      return {.secret = true, .shortest = true, .type = u"short_secret"_s};
    case KeyExportKind::kFullSecret:
      return {.secret = true, .shortest = false, .type = u"full_secret"_s};
  }
  return {};
}

auto ExportKeyFileName(const QString& name, const QString& email,
                       const QString& id, const QString& type) -> QString {
  // The angle brackets are part of the conventional "Name <email>" shape, so
  // they go in from the template rather than surviving inside a field; Windows
  // will not take them at all, hence the square brackets there. On Android a
  // document named with brackets or parentheses is created by the picker but
  // cannot be opened again through its content URI (Qt's content file engine
  // takes it for a missing file and fails to create it), so the export came
  // out empty: plain underscores there.
#if defined(Q_OS_ANDROID)
  auto file_name = QString("%1_%2_%3_%4.asc");
#elif defined(Q_OS_WINDOWS)
  auto file_name = QString("%1[%2](%3)_%4.asc");
#else
  auto file_name = QString("%1<%2>(%3)_%4.asc");
#endif

  return file_name.arg(SanitiseFileNameField(name),
                       SanitiseFileNameField(email), SanitiseFileNameField(id),
                       SanitiseFileNameField(type));
}

}  // namespace GpgFrontend::Shell
