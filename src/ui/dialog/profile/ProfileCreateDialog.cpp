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

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/profile/Profile.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/ChoiceTile.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief What a group covers, for its tooltip.
 *
 * Straight from the catalog, so the list cannot drift from what is actually
 * copied.
 */
auto DescribeGroup(SettingGroup group) -> QString {
  QStringList titles;
  for (const auto* meta : SettingsInGroup(group)) {
    if (SettingInheritedByNewProfile(QLatin1StringView(meta->key))) {
      titles << u"\u2022 "_s + meta->Title();
    }
  }
  return titles.join("\n");
}

auto StepIndex(ProfileCreateStep step) -> int { return static_cast<int>(step); }

}  // namespace

ProfileCreateDialog::ProfileCreateDialog(QWidget* parent)
    : GeneralWizard(typeid(ProfileCreateDialog).name(), tr("Create Profile"),
                    parent) {
  setWindowTitle(tr("New Profile"));

  build_name_page();
  build_keys_page();
  build_settings_page();

  connect(name_edit_, &QLineEdit::textChanged, this, [this]() {
    for (auto* page : pages_) emit page->completeChanged();
  });
  connect(this, &QWizard::currentIdChanged, this,
          &ProfileCreateDialog::slot_page_changed);

  name_edit_->setFocus();
}

auto ProfileCreateDialog::add_page(ProfileCreateStep step)
    -> GeneralWizardPage* {
  auto* page = AddStep(StepIndex(step), kProfileCreateStepCount,
                       ProfileCreateStepTitle(step));
  page->complete = [this, step]() {
    return CanAdvanceProfileCreate(step, name_edit_->text());
  };
  pages_.at(StepIndex(step)) = page;
  return page;
}

void ProfileCreateDialog::build_name_page() {
  auto* page = add_page(ProfileCreateStep::kName);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Give the new profile a name. It is shown in the profile list and "
             "the status bar, and you can change it later."),
          page);

  name_edit_ = new QLineEdit(page);
  name_edit_->setPlaceholderText(tr("for example: Work"));
  name_edit_->setClearButtonEnabled(true);
  layout->addWidget(name_edit_);

  layout->addStretch(1);
}

void ProfileCreateDialog::build_keys_page() {
  auto* page = add_page(ProfileCreateStep::kKeys);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Choose where this profile's built-in key database lives."), page);

  // Asked rather than defaulted because both answers are wrong for somebody;
  // see the class comment. The isolated one is the recommendation because it
  // is the only one that is actually a separate profile.
  //
  // Worded from what this computer actually has: without GnuPG, "this
  // computer's" database is a shared rPGP one, and a tile promising the GnuPG
  // keyring would describe something the profile would never open.
  const auto gnupg = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);
  const auto own_text = DescribeProfileKeyringChoice(true, gnupg);
  const auto shared_text = DescribeProfileKeyringChoice(false, gnupg);

  auto* own =
      new ChoiceTile(own_text.title, own_text.detail, tr("Recommended"), page);
  auto* system =
      new ChoiceTile(shared_text.title, shared_text.detail, {}, page);

  own_keyring_ = own->Radio();
  system_keyring_ = system->Radio();

  // Two radios in two tiles are two exclusive groups unless told otherwise.
  auto* group = new QButtonGroup(page);
  group->addButton(own_keyring_);
  group->addButton(system_keyring_);
  own_keyring_->setChecked(true);

  layout->addWidget(own);
  layout->addWidget(system);

  // The built-in one is only where a profile starts; the settings are where
  // the rest are added, and saying so keeps this from reading as final.
  layout->addSpacing(4);
  layout->addWidget(CreateDetailLabel(
      tr("You can add more key databases later in Settings, under Key "
         "Databases."),
      page));

  layout->addStretch(1);
}

void ProfileCreateDialog::build_settings_page() {
  auto* page = add_page(ProfileCreateStep::kSettings);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Choose which of this window's settings the new profile starts "
             "with."),
          page);

  // A select-all, not a gate: its state is what the groups below add up to,
  // so it can never say "on" over a set of boxes that are all off.
  inherit_settings_ =
      new QCheckBox(tr("Start with this window's settings"), page);
  auto master_font = inherit_settings_->font();
  master_font.setBold(true);
  inherit_settings_->setFont(master_font);
  layout->addWidget(inherit_settings_);

  // One entry per group the catalog says a new profile may start with, so a
  // group added there appears here without anyone remembering to.
  auto* groups = new QVBoxLayout();
  groups->setContentsMargins(24, 0, 0, 0);
  groups->setSpacing(8);

  for (const auto group : InheritableSettingGroups()) {
    auto* cell = new QWidget(page);
    auto* cell_layout = new QVBoxLayout(cell);
    cell_layout->setContentsMargins(0, 0, 0, 0);
    cell_layout->setSpacing(1);

    auto* box = new QCheckBox(SettingGroupTitle(group), cell);
    box->setChecked(true);
    cell_layout->addWidget(box);

    // Indented to sit under the label rather than under the box.
    auto* detail = CreateDetailLabel(SettingGroupDescription(group), cell);
    detail->setContentsMargins(
        box->style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, box) +
            box->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr,
                                      box),
        0, 0, 0);
    cell_layout->addWidget(detail);
    cell->setToolTip(DescribeGroup(group));

    // A title and its sentence are one object; spare height belongs to the
    // stretch below, not between the two.
    cell->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    groups->addWidget(cell);
    group_boxes_.append({group, box});

    connect(box, &QCheckBox::toggled, this,
            [this]() { sync_inherit_master(); });
  }
  layout->addLayout(groups);

  connect(inherit_settings_, &QCheckBox::clicked, this, [this](bool checked) {
    for (const auto& entry : group_boxes_) {
      const QSignalBlocker blocker(entry.second);
      entry.second->setChecked(checked);
    }
    sync_inherit_master();
  });

  // What never follows, said once and plainly, beside the choice it limits.
  auto* note = new QHBoxLayout();
  note->setSpacing(8);
  auto* note_icon = new QLabel(page);
  note_icon->setPixmap(
      QPixmap(u":/icons/info.png"_s)
          .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  note_icon->setFixedSize(16, 16);
  note->addWidget(note_icon, 0, Qt::AlignTop);
  note->addWidget(
      CreateDetailLabel(tr("Never copied: keys, key databases, the workspace "
                           "folder, key protection and proxy credentials."),
                        page),
      1);
  layout->addSpacing(4);
  layout->addLayout(note);

  layout->addStretch(1);

  sync_inherit_master();
}

void ProfileCreateDialog::slot_page_changed(int id) {
  // Queued: QWizard settles focus on its own buttons after announcing the new
  // page, and this has to land after that.
  QTimer::singleShot(0, this, [this, id]() {
    if (currentId() != id) return;

    switch (static_cast<ProfileCreateStep>(id)) {
      case ProfileCreateStep::kName:
        name_edit_->setFocus();
        break;
      case ProfileCreateStep::kKeys:
        (own_keyring_->isChecked() ? own_keyring_ : system_keyring_)
            ->setFocus();
        break;
      case ProfileCreateStep::kSettings:
        inherit_settings_->setFocus();
        break;
    }
  });
}

void ProfileCreateDialog::sync_inherit_master() {
  int checked = 0;
  for (const auto& entry : group_boxes_) {
    if (entry.second->isChecked()) ++checked;
  }

  const QSignalBlocker blocker(inherit_settings_);
  if (checked == 0) {
    inherit_settings_->setCheckState(Qt::Unchecked);
  } else if (checked == group_boxes_.size()) {
    inherit_settings_->setCheckState(Qt::Checked);
  } else {
    inherit_settings_->setCheckState(Qt::PartiallyChecked);
  }
}

auto ProfileCreateDialog::DisplayName() const -> QString {
  return name_edit_->text().trimmed();
}

auto ProfileCreateDialog::SelfContained() const -> bool {
  return own_keyring_->isChecked();
}

auto ProfileCreateDialog::InheritedGroups() const -> QList<SettingGroup> {
  QList<SettingGroup> groups;
  for (const auto& [group, box] : group_boxes_) {
    if (box->isChecked()) groups.append(group);
  }
  return groups;
}

}  // namespace GpgFrontend::UI
