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

#include "shell/rules/RecipientConfirm.h"

#include "core/model/GpgKeyGroup.h"
#include "shell/rules/RecipientRules.h"

namespace GpgFrontend::Shell {

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

auto EncryptQuestions(qsizetype recipient_count,
                      const GpgAbstractKeyPtrList& keys, bool confirm_many)
    -> QList<EncryptQuestion> {
  // The desktop's main window asked these first; its context keeps their
  // translations. Spelled out at each call, which is what lupdate reads.
  QList<EncryptQuestion> questions;

  if (NeedsRecipientConfirmation(recipient_count, confirm_many)) {
    // Only reached with two or more recipients, so the English source can
    // say "recipients" outright; %n still lets other languages pick their
    // form.
    const int count = static_cast<int>(recipient_count);
    questions.append({
        .kind = EncryptQuestionKind::kManyRecipients,
        .title = QCoreApplication::translate("GpgFrontend::UI::MainWindow",
                                             "Confirm Recipients"),
        .text = QCoreApplication::translate("GpgFrontend::UI::MainWindow",
                                            "Encrypt to %n recipients?",
                                            nullptr, count),
        .detail = QCoreApplication::translate(
            "GpgFrontend::UI::MainWindow",
            "Each recipient will be able to see the key IDs of all the "
            "others."),
        .accept = QCoreApplication::translate("GpgFrontend::UI::MainWindow",
                                              "Encrypt"),
    });
  }

  // Allowed, never silently: every recipient can unwrap the same session
  // key, so the message is only as strong as its weakest recipient.
  if (RecipientsMixPostQuantumAndClassical(keys)) {
    questions.append({
        .kind = EncryptQuestionKind::kMixedRecipients,
        .title = QCoreApplication::translate("GpgFrontend::UI::MainWindow",
                                             "Mixed Recipient Types"),
        .text = QCoreApplication::translate(
            "GpgFrontend::UI::MainWindow",
            "Some recipients use post-quantum encryption and others do "
            "not."),
        .detail = QCoreApplication::translate(
            "GpgFrontend::UI::MainWindow",
            "The message key is encrypted to every recipient, so anyone able "
            "to break the classical recipients' encryption can read the "
            "whole message: the post-quantum protection does not hold for "
            "it. To keep that protection, encrypt separately for the "
            "post-quantum recipients."),
        .accept = QCoreApplication::translate("GpgFrontend::UI::MainWindow",
                                              "Encrypt Anyway"),
    });
  }
  return questions;
}

}  // namespace GpgFrontend::Shell
