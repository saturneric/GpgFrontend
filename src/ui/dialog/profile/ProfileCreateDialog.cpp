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
#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief One answer to a choice, as a tile the whole of which is clickable.
 *
 * A radio button with a grey paragraph under it asks the reader to aim at a
 * 14px circle and leaves them unsure which paragraph belongs to which answer.
 * A tile makes the answer and its explanation one object, and the selected one
 * says so with its border rather than with a dot.
 */
class ChoiceTile : public QFrame {
 public:
  ChoiceTile(const QString& title, const QString& detail, const QString& chip,
             QWidget* parent)
      : QFrame(parent) {
    setObjectName(u"GFChoiceTile"_s);
    setCursor(Qt::PointingHandCursor);

    radio_ = new QRadioButton(this);
    radio_->setFocusPolicy(Qt::StrongFocus);

    auto* title_label = new QLabel(title, this);
    auto title_font = title_label->font();
    title_font.setBold(true);
    title_label->setFont(title_font);

    auto* title_row = new QHBoxLayout();
    title_row->setSpacing(8);
    title_row->addWidget(title_label);
    if (!chip.isEmpty()) {
      auto* chip_label = new QLabel(this);
      SetChip(chip_label, chip, AccentColor(palette(), true));
      title_row->addWidget(chip_label);
    }
    title_row->addStretch(1);

    auto* detail_label = CreateDetailLabel(detail, this);

    // The labels would otherwise take the click for themselves, and a tile
    // that only answers to its radio button is a radio button with a border.
    for (auto* label : findChildren<QLabel*>()) {
      label->setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    auto* text = new QVBoxLayout();
    text->setSpacing(2);
    text->addLayout(title_row);
    text->addWidget(detail_label);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);
    layout->addWidget(radio_, 0, Qt::AlignTop);
    layout->addLayout(text, 1);

    connect(radio_, &QRadioButton::toggled, this, [this]() { restyle(); });
    restyle();
  }

  [[nodiscard]] auto Radio() const -> QRadioButton* { return radio_; }

 protected:
  void mousePressEvent(QMouseEvent* event) override {
    radio_->setChecked(true);
    radio_->setFocus(Qt::MouseFocusReason);
    QFrame::mousePressEvent(event);
  }

 private:
  QRadioButton* radio_;

  void restyle() {
    // Palette-derived, like every card in the application, so it follows a
    // dark theme without a second set of colours.
    const auto selected = radio_->isChecked();
    auto accent = palette().color(QPalette::Highlight);
    auto tint = accent;
    tint.setAlpha(24);

    setStyleSheet(
        u"QFrame#GFChoiceTile {"
        "  border: %1px solid %2;"
        "  border-radius: 8px;"
        "  background: %3;"
        "}"_s.arg(selected ? 2 : 1)
            .arg(selected ? accent.name() : BorderColor(palette()).name())
            .arg(selected ? tint.name(QColor::HexArgb) : u"transparent"_s));
  }
};

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
  layout->setSpacing(12);

  // Never smaller than its content. The dialog restores a remembered size,
  // and one remembered from an earlier, shorter layout would otherwise shave
  // the bottom line off every description.
  layout->setSizeConstraint(QLayout::SetMinimumSize);

  // The same width as the export dialog, its sibling: word-wrapped labels ask
  // for almost no width, and a dialog left to itself opens as a tall ribbon.
  setMinimumWidth(560);

  layout->addLayout(CreateDialogHeader(
      u":/icons/keypairs.png"_s, tr("New Profile"),
      tr("Its own keys and settings, opened in a window of its own."), this));

  build_name(layout);
  build_keyring(layout);
  build_settings(layout);

  // Whatever height the remembered geometry gives beyond what is needed goes
  // here, below everything, rather than between a label and its control.
  layout->addStretch(1);

  buttons_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  auto* create = buttons_->button(QDialogButtonBox::Ok);
  create->setText(tr("Create Profile"));
  create->setDefault(true);
  layout->addWidget(buttons_);

  connect(name_edit_, &QLineEdit::textChanged, this,
          &ProfileCreateDialog::slot_name_changed);
  connect(buttons_, &QDialogButtonBox::accepted, this,
          &ProfileCreateDialog::slot_accept);
  connect(buttons_, &QDialogButtonBox::rejected, this,
          &ProfileCreateDialog::reject);

  slot_name_changed();
  name_edit_->setFocus();
}

void ProfileCreateDialog::build_name(QVBoxLayout* layout) {
  auto* content = new QWidget(this);
  auto* content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->setSpacing(6);

  name_edit_ = new QLineEdit(content);
  name_edit_->setPlaceholderText(tr("for example: Work"));
  name_edit_->setClearButtonEnabled(true);
  content_layout->addWidget(name_edit_);
  content_layout->addWidget(CreateDetailLabel(
      tr("Shown in the profile list and the status bar. You can change it "
         "later."),
      content));

  layout->addWidget(CreateCard(tr("Name"), content, this));
}

void ProfileCreateDialog::build_keyring(QVBoxLayout* layout) {
  auto* content = new QWidget(this);
  auto* content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->setSpacing(8);

  // Asked rather than defaulted because both answers are wrong for somebody;
  // see the class comment. The isolated one is the recommendation because it
  // is the only one that is actually a separate profile.
  auto* own = new ChoiceTile(
      tr("Start with an empty keyring"),
      tr("Keys live inside this profile and can be exported with it as one "
         "file."),
      tr("Recommended"), content);
  auto* system = new ChoiceTile(
      tr("Use the system GnuPG keyring"),
      tr("Your existing keys, shared with the system. Not carried in a "
         "profile file."),
      {}, content);

  own_keyring_ = own->Radio();
  system_keyring_ = system->Radio();

  // Two radios in two tiles are two exclusive groups unless told otherwise.
  auto* group = new QButtonGroup(this);
  group->addButton(own_keyring_);
  group->addButton(system_keyring_);
  own_keyring_->setChecked(true);

  content_layout->addWidget(own);
  content_layout->addWidget(system);

  layout->addWidget(CreateCard(tr("Keys"), content, this));
}

void ProfileCreateDialog::build_settings(QVBoxLayout* layout) {
  auto* content = new QWidget(this);
  auto* content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->setSpacing(10);

  // A select-all, not a gate: its state is what the groups below add up to,
  // so it can never say "on" over a set of boxes that are all off.
  inherit_settings_ =
      new QCheckBox(tr("Start with this window's settings"), content);
  auto master_font = inherit_settings_->font();
  master_font.setBold(true);
  inherit_settings_->setFont(master_font);
  content_layout->addWidget(inherit_settings_);

  // One entry per group the catalog says a new profile may start with, so a
  // group added there appears here without anyone remembering to.
  //
  // One column rather than two: each description fits on one line at this
  // width, and a wrapped label in a grid is given the height of one line by a
  // top-level dialog, which clips the second.
  auto* groups = new QVBoxLayout();
  groups->setContentsMargins(24, 0, 0, 0);
  groups->setSpacing(8);

  for (const auto group : InheritableSettingGroups()) {
    auto* cell = new QWidget(content);
    auto* cell_layout = new QVBoxLayout(cell);
    cell_layout->setContentsMargins(0, 0, 0, 0);
    cell_layout->setSpacing(1);

    auto* box = new QCheckBox(SettingGroupTitle(group), cell);
    box->setChecked(true);
    cell_layout->addWidget(box);

    // Indented to sit under the label rather than under the box.
    auto* detail = CreateDetailLabel(SettingGroupDescription(group), cell);
    detail->setWordWrap(false);
    detail->setContentsMargins(
        box->style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, box) +
            box->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr,
                                      box),
        0, 0, 0);
    cell_layout->addWidget(detail);
    cell->setToolTip(DescribeGroup(group));

    // A title and its sentence are one object; spare height belongs to the
    // stretch below the cards, not between the two.
    cell->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    groups->addWidget(cell);
    group_boxes_.append({group, box});

    connect(box, &QCheckBox::toggled, this,
            [this]() { sync_inherit_master(); });
  }
  content_layout->addLayout(groups);

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
  auto* note_icon = new QLabel(content);
  note_icon->setPixmap(
      QPixmap(u":/icons/info.png"_s)
          .scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  note_icon->setFixedSize(16, 16);
  note->addWidget(note_icon, 0, Qt::AlignTop);
  note->addWidget(
      CreateDetailLabel(tr("Never copied: keys, key databases, the workspace "
                           "folder, key protection and proxy credentials."),
                        content),
      1);
  content_layout->addLayout(note);

  sync_inherit_master();
  layout->addWidget(CreateCard(tr("Settings"), content, this));
}

void ProfileCreateDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);

  // A top-level window's automatic size is capped at two thirds of the screen,
  // and wrapped text is measured at one line until it has a width. Asked once
  // it has one, the layout knows how tall this really is -- still kept on the
  // screen, where a scroll bar would be better than a cut-off button row.
  if (layout() == nullptr || !layout()->hasHeightForWidth()) return;

  auto wanted = layout()->totalHeightForWidth(width());
  if (const auto* current = screen(); current != nullptr) {
    wanted = qMin(wanted, current->availableGeometry().height() -
                              (frameGeometry().height() - height()));
  }
  if (wanted > height()) resize(width(), wanted);
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
  for (const auto& [group, box] : group_boxes_) {
    if (box->isChecked()) groups.append(group);
  }
  return groups;
}

}  // namespace GpgFrontend::UI
