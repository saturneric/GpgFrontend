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

#include "SettingsIM.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/PassphraseGenerator.h"

namespace GpgFrontend::UI {

InstantMessagingTab::InstantMessagingTab(QWidget* parent) : QWidget(parent) {
  // The Message Book phrase whitens every instant-messaging token: its bytes
  // derive the shuffle + XOR that hide the message, so without the same phrase
  // a token is indistinguishable from random text.
  auto* book_box = new QGroupBox(tr("Message Book"), this);
  auto* book_form = new QFormLayout(book_box);

  book_phrase_edit_ = new QLineEdit(book_box);
  book_phrase_edit_->setPlaceholderText(
      tr("Leave empty to use the shared default book"));

  // One-click strong phrase: 48 CSPRNG alphanumeric chars. The user
  // still has to share it privately with the peer — both sides must match.
  auto* generate_button = new QPushButton(tr("Generate"), book_box);
  generate_button->setToolTip(
      tr("Create a strong random phrase. Share it with your friend so you both "
         "use the same one."));
  connect(generate_button, &QPushButton::clicked, this, [this]() {
    const auto phrase = PassphraseGenerator::GetInstance().Generate(48);
    if (phrase) book_phrase_edit_->setText(phrase->ConvertToQString());
  });

  auto* phrase_row = new QHBoxLayout();
  phrase_row->setContentsMargins(0, 0, 0, 0);
  phrase_row->addWidget(book_phrase_edit_, 1);
  phrase_row->addWidget(generate_button);
  book_form->addRow(tr("Message Book Phrase:"), phrase_row);

  auto* note = new QLabel(
      tr("Both sides must set the same phrase. A shared secret phrase makes "
         "your instant messages indistinguishable from random text. Therefore, "
         "an observer cannot even tell they are GpgFrontend or PGP messages. "
         "An empty phrase uses the built-in default book, which only hides the "
         "format from simple scanners and offers no protection against an "
         "observer who knows GpgFrontend."),
      book_box);
  note->setWordWrap(true);
  book_form->addRow(note);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(book_box);
  layout->addStretch(1);
  setLayout(layout);

  SetSettings();
}

void InstantMessagingTab::SetSettings() {
  auto settings = GetSettings();
  book_phrase_edit_->setText(
      settings.value("im/password_book_phrase").toString());
}

void InstantMessagingTab::ApplySettings() {
  auto settings = GetSettings();
  settings.setValue("im/password_book_phrase",
                    book_phrase_edit_->text().trimmed());
}

}  // namespace GpgFrontend::UI
