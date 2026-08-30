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

#include "ui/dialog/MetaListDialog.h"

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

MetaListDialog::MetaListDialog(const QString& title, const QString& subtitle,
                               QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(title);
  setModal(true);
  // Word-wrapped labels ask for almost no width, so the dialog would open as a
  // tall ribbon; stating the width once makes everything wrap against it.
  setMinimumWidth(500);

  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(24, 22, 24, 18);
  layout_->setSpacing(16);

  layout_->addLayout(CreateDialogHeader(QStringLiteral(":/icons/lock.png"),
                                        title, subtitle, this));

  // Added now and kept last: everything else is inserted above it, so the
  // buttons stay at the bottom however many sections a caller adds.
  buttons_ = new QDialogButtonBox(this);
  layout_->addWidget(buttons_);

  connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void MetaListDialog::AddSection(const QString& card_title,
                                const QVector<MetaListRow>& rows) {
  if (rows.isEmpty()) return;

  auto* panel = new MetaListPanel(this);
  panel->SetRows(rows);

  QWidget* content = panel;
  if (!card_title.isEmpty()) content = CreateCard(card_title, panel, this);

  layout_->insertWidget(layout_->count() - 1, content);
}

void MetaListDialog::AddNote(const QString& text, bool danger) {
  if (text.isEmpty()) return;

  auto* note = new QLabel(text, this);
  note->setWordWrap(true);
  SetLabelTextColor(
      note, danger ? DangerColor(palette()) : MutedTextColor(note->palette()));

  layout_->insertWidget(layout_->count() - 1, note);
}

auto MetaListDialog::AddButton(const QString& text,
                               QDialogButtonBox::ButtonRole role) -> int {
  const auto index = static_cast<int>(buttons_->buttons().size());

  auto* button = buttons_->addButton(text, role);
  if (index == 0) button->setDefault(true);

  connect(button, &QPushButton::clicked, this, [this, index, role]() {
    choice_ = index;
    // A rejecting button still answers the question; it just answers it with
    // "no", and a caller reading Choice() gets to tell the two apart.
    if (role == QDialogButtonBox::RejectRole) {
      reject();
    } else {
      accept();
    }
  });

  return index;
}

auto MetaListDialog::Choice() const -> int { return choice_; }

}  // namespace GpgFrontend::UI
