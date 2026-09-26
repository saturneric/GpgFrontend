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

#include "ui/main_window/RecipientConfirm.h"

#include "core/model/GpgKeyGroup.h"

namespace GpgFrontend::UI {

auto NeedsRecipientConfirmation(qsizetype checked_count, bool setting_on)
    -> bool {
  return setting_on && checked_count >= 2;
}

auto FormatKeyIdForDisplay(const QString& id) -> QString {
  QStringList groups;
  for (qsizetype i = 0; i < id.size(); i += 4) groups.append(id.mid(i, 4));
  return groups.join(' ');
}

auto BuildRecipientConfirmHtml(const GpgAbstractKeyPtrList& keys,
                               const QColor& muted) -> QString {
  const auto muted_style = QString("color:%1;").arg(muted.name());

  QString rows;
  for (const auto& key : keys) {
    if (key == nullptr) continue;

    QString detail;
    QString id;
    if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
      auto group = qSharedPointerDynamicCast<GpgKeyGroup>(key);
      const int members = group != nullptr ? group->KeyIds().size() : 0;
      detail =
          QCoreApplication::translate("GpgFrontend::UI::RecipientConfirm",
                                      "Key group, %n key(s)", nullptr, members);
    } else {
      detail = key->Email().toHtmlEscaped();
      id = FormatKeyIdForDisplay(key->ID());
    }

    rows += QString(
                "<tr>"
                "<td style='padding:3px 16px 3px 0;'><b>%1</b><br/>"
                "<span style='%2'>%3</span></td>"
                "<td style='padding:3px 0; white-space:nowrap;' "
                "valign='middle'><code>%4</code></td>"
                "</tr>")
                .arg(key->Name().toHtmlEscaped(), muted_style, detail, id);
  }

  return QString("<table cellspacing='0' cellpadding='0'>%1</table>").arg(rows);
}

}  // namespace GpgFrontend::UI
