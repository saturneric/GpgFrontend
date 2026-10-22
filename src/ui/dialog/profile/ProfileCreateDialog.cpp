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

#include "ui/dialog/profile/ProfileCreateDialog.h"

#include "core/function/SettingCatalog.h"
#include "core/profile/Profile.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief A grey explanatory label that is allowed to be more than one line.
 *
 * A word-wrapped QLabel reports the height of a single line unless it is asked
 * to declare heightForWidth, so a dialog sized from its layout comes out too
 * short and the text is clipped into whatever sits below it.
 *
 * @param text the explanation
 * @param parent parent widget
 * @return the label
 */
auto MakeHintLabel(const QString& text, QWidget* parent) -> QLabel* {
  auto* label = new QLabel(text, parent);
  label->setWordWrap(true);
  label->setStyleSheet("color: gray;");

  // Minimum, not MinimumExpanding: the dialog restores a remembered size, and
  // an expanding label soaks up every pixel of it as blank space between the
  // text and the control it explains.
  auto policy = label->sizePolicy();
  policy.setHeightForWidth(true);
  policy.setVerticalPolicy(QSizePolicy::Minimum);
  label->setSizePolicy(policy);
  return label;
}

/**
 * @brief What a group covers, for its tooltip.
 *
 * Straight from the catalog, so the list cannot drift from what is actually
 * copied.
 */
auto DescribeGroup(SettingGroup group) -> QString {
  QStringList titles;
  for (const auto* meta : SettingsInGroup(group)) {
    if (SettingInheritedByNewProfile(QLatin1String(meta->key))) {
      titles << meta->Title();
    }
  }
  return titles.join("\n");
}

}  // namespace

ProfileCreateDialog::ProfileCreateDialog(QWidget* parent)
    : GeneralDialog("profile_create_dialog", parent) {
  init_ui();
  setWindowTitle(tr("New Profile"));
  setModal(true);

  // GeneralDialog deletes itself on close. This dialog is read *after*
  // exec() returns — it exists to hand back a name and a keyring choice —
  // so that would free it while the caller is still using it.
  setAttribute(Qt::WA_DeleteOnClose, false);

  movePosition2CenterOfParent();
}

void ProfileCreateDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);

  // The explanations below are word-wrapped, and a word-wrapped label asks for
  // almost no width and however much height that implies. Left to itself the
  // dialog opens as a tall ribbon with the text running into the controls, so
  // the width is stated once here and everything wraps against it.
  setMinimumWidth(460);

  auto* form = new QFormLayout();
  name_edit_ = new QLineEdit(this);
  name_edit_->setPlaceholderText(tr("for example: Work"));
  form->addRow(tr("Name"), name_edit_);
  layout->addLayout(form);

  auto* keyring = new QGroupBox(tr("Keys"), this);
  auto* keyring_layout = new QVBoxLayout(keyring);

  own_keyring_ = new QRadioButton(tr("Start with an empty keyring"), keyring);
  own_keyring_->setChecked(true);
  auto* own_hint = MakeHintLabel(
      tr("Keys live inside this profile. It stays separate from everything "
         "else and can be exported as a single file. It opens with no keys, "
         "so you will import or create them here."),
      keyring);

  system_keyring_ =
      new QRadioButton(tr("Use the system GnuPG keyring"), keyring);
  auto* system_hint = MakeHintLabel(
      tr("The same keys you already have. They are shared with the rest of "
         "the system, so they are not really separate and cannot be carried "
         "inside a profile file."),
      keyring);

  keyring_layout->addWidget(own_keyring_);
  keyring_layout->addWidget(own_hint);
  keyring_layout->addSpacing(8);
  keyring_layout->addWidget(system_keyring_);
  keyring_layout->addWidget(system_hint);
  layout->addWidget(keyring);

  auto* settings = new QGroupBox(tr("Settings"), this);
  auto* settings_layout = new QVBoxLayout(settings);

  inherit_settings_ =
      new QCheckBox(tr("Start with this window's settings"), settings);
  inherit_settings_->setChecked(true);
  settings_layout->addWidget(inherit_settings_);

  // One box per group the catalog says a new profile may start with, so a
  // group added there appears here without anyone remembering to.
  auto* groups_layout = new QVBoxLayout();
  groups_layout->setContentsMargins(24, 0, 0, 0);
  groups_layout->setSpacing(2);
  for (const auto group : InheritableSettingGroups()) {
    auto* box = new QCheckBox(SettingGroupTitle(group), settings);
    box->setChecked(true);
    box->setToolTip(DescribeGroup(group));
    groups_layout->addWidget(box);
    group_boxes_.append({group, box});
  }
  settings_layout->addLayout(groups_layout);

  settings_layout->addWidget(MakeHintLabel(
      tr("Keys, key databases, the workspace folder, key protection and proxy "
         "credentials are never copied. Hover over a group to see what it "
         "covers."),
      settings));
  layout->addWidget(settings);

  connect(inherit_settings_, &QCheckBox::toggled, this, [this](bool on) {
    for (const auto& entry : group_boxes_) entry.second->setEnabled(on);
  });

  // Whatever height the remembered geometry gives beyond what is needed goes
  // here, below everything, rather than between a label and its control.
  layout->addStretch(1);

  buttons_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons_);

  connect(name_edit_, &QLineEdit::textChanged, this,
          &ProfileCreateDialog::slot_name_changed);
  connect(buttons_, &QDialogButtonBox::accepted, this,
          &ProfileCreateDialog::slot_accept);
  connect(buttons_, &QDialogButtonBox::rejected, this,
          &ProfileCreateDialog::reject);

  slot_name_changed();
}

void ProfileCreateDialog::slot_name_changed() {
  // A name is the only thing that can be wrong here, and only by being absent:
  // it is shown in the profile list and nothing else is derived from it.
  buttons_->button(QDialogButtonBox::Ok)
      ->setEnabled(!name_edit_->text().trimmed().isEmpty());
}

void ProfileCreateDialog::slot_accept() {
  if (name_edit_->text().trimmed().isEmpty()) return;
  accept();
}

auto ProfileCreateDialog::DisplayName() const -> QString {
  return name_edit_->text().trimmed();
}

auto ProfileCreateDialog::SelfContained() const -> bool {
  return own_keyring_->isChecked();
}

auto ProfileCreateDialog::InheritedGroups() const -> QList<SettingGroup> {
  QList<SettingGroup> groups;
  if (!inherit_settings_->isChecked()) return groups;

  for (const auto& [group, box] : group_boxes_) {
    if (box->isChecked()) groups.append(group);
  }
  return groups;
}

}  // namespace GpgFrontend::UI
