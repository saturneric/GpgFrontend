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

#include "KeyGroupManageDialog.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyGroupEditDialog.h"
#include "ui/dialog/KeyGroupMetadataRules.h"
#include "ui/function/ShowKeyDetails.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/KeyTreeView.h"

namespace GpgFrontend::UI {

namespace {

// The available pane holds exactly one tab.
constexpr int kAvailableTabIndex = 0;

// Columns of GpgKeyTreeModel, as laid out by its column headers.
constexpr int kSelectColumn = 0;
constexpr int kTypeColumn = 1;
constexpr int kIdentityColumn = 2;
constexpr int kKeyIdColumn = 3;
constexpr int kUsageColumn = 4;
constexpr int kAlgorithmColumn = 5;
constexpr int kCreateDateColumn = 6;

// Every candidate is encryption-capable by construction, so a usage column
// would read the same on every row, and owner trust is not what membership
// turns on. Both stay available through the Columns chooser.
const auto kAvailableColumns =
    GpgKeyTableColumn::kTYPE | GpgKeyTableColumn::kNAME |
    GpgKeyTableColumn::kEMAIL_ADDRESS | GpgKeyTableColumn::kKEY_ID;

auto MakeHintLabel(QWidget* parent) -> QLabel* {
  auto* label = new QLabel(parent);
  label->setWordWrap(true);

  auto hint_palette = label->palette();
  hint_palette.setColor(
      QPalette::WindowText,
      parent->palette().color(QPalette::Disabled, QPalette::WindowText));
  label->setPalette(hint_palette);

  return label;
}

auto MakeSectionLabel(const QString& text, QWidget* parent) -> QLabel* {
  auto* label = new QLabel(text, parent);
  auto font = label->font();
  font.setBold(true);
  label->setFont(font);
  return label;
}

}  // namespace

KeyGroupManageDialog::KeyGroupManageDialog(
    int channel, const QSharedPointer<GpgKeyGroup>& key_group, QWidget* parent)
    : GeneralDialog("KeyGroupManageDialog", parent),
      channel_(channel),
      group_id_(key_group != nullptr ? key_group->ID() : QString{}) {
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#endif

  setWindowTitle(tr("Key Group Management"));
  setModal(true);
  setMinimumSize(880, 520);
  resize(1040, 640);

  init_ui();

  refresh_header();
  slot_update_action_state();

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyGroupManageDialog::slot_reload, Qt::QueuedConnection);

  setAttribute(Qt::WA_DeleteOnClose, true);

  this->show();
  this->raise();
  this->activateWindow();
}

void KeyGroupManageDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  init_header_card(layout);

  auto* desc_label = MakeHintLabel(this);
  desc_label->setText(
      tr("Encrypting to this group encrypts to every key it contains, "
         "including the members of any group nested inside it."));
  layout->addWidget(desc_label);

  init_panes(layout);

  footer_label_ = MakeHintLabel(this);
  layout->addWidget(footer_label_);

  auto* button_box = new QDialogButtonBox(QDialogButtonBox::Close, this);
  button_box->button(QDialogButtonBox::Close)->setText(tr("Close"));
  button_box->button(QDialogButtonBox::Close)->setDefault(true);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::close);
  connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::close);
  layout->addWidget(button_box);
}

void KeyGroupManageDialog::init_header_card(QVBoxLayout* layout) {
  auto* card = new QFrame(this);
  card->setObjectName("KeyGroupHeaderCard");
  card->setFrameShape(QFrame::StyledPanel);
  card->setFrameShadow(QFrame::Plain);
  card->setAutoFillBackground(true);

  auto card_palette = card->palette();
  card_palette.setColor(QPalette::Window, palette().color(QPalette::Base));
  card->setPalette(card_palette);

  icon_label_ = new QLabel(card);
  icon_label_->setPixmap(QIcon(":/icons/key-group.png").pixmap(32, 32));
  icon_label_->setFixedSize(32, 32);

  name_label_ = new QLabel(card);
  auto name_font = name_label_->font();
  name_font.setBold(true);
  name_font.setPointSizeF(name_font.pointSizeF() + 1.0);
  name_label_->setFont(name_font);
  name_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  identity_label_ = MakeHintLabel(card);
  identity_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  auto* text_layout = new QVBoxLayout();
  text_layout->setContentsMargins(0, 0, 0, 0);
  text_layout->setSpacing(2);
  text_layout->addWidget(name_label_);
  text_layout->addWidget(identity_label_);

  auto* edit_button =
      new QPushButton(QIcon(":/icons/edit.png"), tr("Edit…"), card);
  edit_button->setToolTip(
      tr("Change the name, email and comment of this key "
         "group."));
  connect(edit_button, &QPushButton::clicked, this,
          &KeyGroupManageDialog::slot_edit_metadata);

  auto* delete_button =
      new QPushButton(QIcon(":/icons/trash.png"), tr("Delete Group…"), card);
  delete_button->setToolTip(
      tr("Delete this key group. The keys in it are not touched."));
  connect(delete_button, &QPushButton::clicked, this,
          &KeyGroupManageDialog::slot_delete_group);

  auto* card_layout = new QHBoxLayout(card);
  card_layout->setContentsMargins(10, 10, 10, 10);
  card_layout->setSpacing(10);
  card_layout->addWidget(icon_label_);
  card_layout->addLayout(text_layout, 1);
  card_layout->addWidget(edit_button);
  card_layout->addWidget(delete_button);

  layout->addWidget(card);
}

void KeyGroupManageDialog::init_panes(QVBoxLayout* layout) {
  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);

  // --- members pane ---
  auto* members_pane = new QWidget(splitter);

  member_filter_ = new QLineEdit(members_pane);
  member_filter_->setClearButtonEnabled(true);
  member_filter_->setPlaceholderText(tr("Filter members"));
  member_filter_->addAction(QIcon(":/icons/search.png"),
                            QLineEdit::LeadingPosition);
  member_filter_->setMaximumWidth(220);

  filter_timer_ = new QTimer(this);
  filter_timer_->setSingleShot(true);
  filter_timer_->setInterval(200);

  members_view_ = new KeyTreeView(
      channel_,
      // Any direct member may be removed. The model forces deeper rows
      // uncheckable, so this never sees one.
      [](GpgAbstractKey* key) -> bool { return key != nullptr; },
      [](const GpgAbstractKey*) -> bool { return true; }, members_pane);
  members_view_->SetBuildMode(GpgKeyTreeBuildMode::kKEY_GROUP_MEMBERS);
  members_view_->SetKeyProvider([this]() -> GpgAbstractKeyPtrList {
    auto key_group = group();
    if (key_group == nullptr) return {};
    return AbstractKeyRepository::GetInstance(channel_).GetKeys(
        key_group->KeyIds());
  });
  members_view_->SetRecursiveFiltering(true);
  // This dialog is itself what "show key details" opens for a key group, so a
  // double click on a nested group would stack another copy of it.
  members_view_->SetOpenDetailsOnDoubleClick(false);
  members_view_->SetEmptyStateEnabled(true);
  members_view_->SetEmptyStateText(
      tr("This group has no members yet.\n\nCheck keys on the right and press "
         "Add."));
  members_view_->setContextMenuPolicy(Qt::CustomContextMenu);

  // Select / Type / Identity / Key ID is enough to recognise a member; the
  // rest only forces a horizontal scrollbar in a pane this narrow.
  for (const auto column :
       {kUsageColumn, kAlgorithmColumn, kCreateDateColumn}) {
    members_view_->setColumnHidden(column, true);
  }

  // Identity absorbs the leftover width and elides. Sizing it to its contents
  // instead, as the shared view does, lets a long user ID push the key ID off
  // the pane and leaves a horizontal scrollbar behind.
  members_view_->setTextElideMode(Qt::ElideRight);

  auto* member_columns = members_view_->header();
  member_columns->setStretchLastSection(false);
  member_columns->setSectionResizeMode(kSelectColumn,
                                       QHeaderView::ResizeToContents);
  member_columns->setSectionResizeMode(kTypeColumn,
                                       QHeaderView::ResizeToContents);
  member_columns->setSectionResizeMode(kIdentityColumn, QHeaderView::Stretch);
  member_columns->setSectionResizeMode(kKeyIdColumn,
                                       QHeaderView::ResizeToContents);

  auto* members_header = new QHBoxLayout();
  members_header->setContentsMargins(0, 0, 0, 0);
  members_header->addWidget(MakeSectionLabel(tr("Members"), members_pane));
  members_header->addStretch(1);
  members_header->addWidget(member_filter_);

  auto* members_layout = new QVBoxLayout(members_pane);
  members_layout->setContentsMargins(0, 0, 0, 0);
  members_layout->setSpacing(6);
  members_layout->addLayout(members_header);
  members_layout->addWidget(members_view_, 1);

  // --- transfer column ---
  auto* transfer_column = new QWidget(splitter);

  add_button_ = new QToolButton(transfer_column);
  add_button_->setIcon(QIcon(":/icons/button_previous.png"));
  add_button_->setText(tr("Add"));
  add_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  add_button_->setToolTip(
      tr("Add the keys checked on the right to this group."));
  add_button_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));

  remove_button_ = new QToolButton(transfer_column);
  remove_button_->setIcon(QIcon(":/icons/button_next.png"));
  remove_button_->setText(tr("Remove"));
  remove_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  remove_button_->setToolTip(
      tr("Remove the members checked on the left from this group."));
  remove_button_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));

  const auto button_width =
      qMax(add_button_->sizeHint().width(), remove_button_->sizeHint().width());
  add_button_->setMinimumWidth(button_width);
  remove_button_->setMinimumWidth(button_width);

  auto* transfer_layout = new QVBoxLayout(transfer_column);
  transfer_layout->setContentsMargins(6, 0, 6, 0);
  transfer_layout->setSpacing(6);
  transfer_layout->addStretch(1);
  transfer_layout->addWidget(add_button_);
  transfer_layout->addWidget(remove_button_);
  transfer_layout->addStretch(1);
  transfer_column->setFixedWidth(transfer_column->sizeHint().width());

  // --- available pane ---
  auto* available_pane = new QWidget(splitter);

  available_list_ = new KeyList(
      channel_, KeyMenuAbility::kCOLUMN_FILTER | KeyMenuAbility::kSEARCH_BAR,
      kAvailableColumns, available_pane);

  // Its own scope: this pane is narrow and single-purpose, and sharing the
  // default one would drag every other key list's column widths around.
  available_list_->SetPersistenceScope("keygroup_manage", kAvailableColumns);

  available_list_->AddListGroupTab(
      tr("Available"), "keygroup-available",
      GpgKeyTableDisplayMode::kPRIVATE_KEY |
          GpgKeyTableDisplayMode::kPUBLIC_KEY,
      [this](const GpgAbstractKey* key) -> bool {
        if (key == nullptr || !key->IsHasEncrCap()) return false;
        // One call covers the group itself, its current members, and any group
        // that would nest back into this one.
        return KeyGroupRepository::GetInstance(channel_).CanAddKeyToKeyGroup(
            group_id_, key->ID());
      });
  available_list_->SetTabOrderSettingsKey("keys/keygroup_manage_tab_order");

  // One fixed tab: the category rail would have nothing to switch between.
  available_list_->SetCategoryRailVisible(false);

  // The key list carries its own chrome margins, sized for a main window. Left
  // as-is they open a gap between the section label and the search box that the
  // members pane opposite does not have.
  available_list_->SetChromeInset({0, 0, 0, 0}, {0, 0, 0, 0});

  // The shared wording talks about tabs and keyrings, which explains nothing
  // here. Running out of candidates is the normal end state of this pane.
  available_list_->SetFilteredOutText(
      tr("Every key that can be added is already in this group."));

  available_list_->SlotRefresh();

  auto* available_layout = new QVBoxLayout(available_pane);
  available_layout->setContentsMargins(0, 0, 0, 0);
  available_layout->setSpacing(6);
  available_layout->addWidget(
      MakeSectionLabel(tr("Available Keys"), available_pane));
  available_layout->addWidget(available_list_, 1);

  splitter->addWidget(members_pane);
  splitter->addWidget(transfer_column);
  splitter->addWidget(available_pane);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 0);
  splitter->setStretchFactor(2, 1);

  // Without explicit sizes the panes inherit their size hints, and the key
  // table's is far wider than the tree's, which squeezes the members list
  // until its identity column truncates.
  const auto transfer_width = transfer_column->sizeHint().width();
  splitter->setSizes({460, transfer_width, 460});

  layout->addWidget(splitter, 1);

  connect(add_button_, &QToolButton::clicked, this,
          &KeyGroupManageDialog::slot_add_to_key_group);
  connect(remove_button_, &QToolButton::clicked, this,
          &KeyGroupManageDialog::slot_remove_from_key_group);

  connect(available_list_, &KeyList::SignalKeyChecked, this,
          &KeyGroupManageDialog::slot_update_action_state);
  connect(members_view_, &KeyTreeView::SignalKeysChecked, this,
          &KeyGroupManageDialog::slot_update_action_state);

  connect(available_list_, &KeyList::SignalRequestShowDetails, this, [this]() {
    auto keys = available_list_->GetSelectedKeys();
    if (keys.isEmpty()) return;
    ShowKeyDetails(this, channel_, keys.front());
  });

  connect(members_view_, &QWidget::customContextMenuRequested, this,
          &KeyGroupManageDialog::slot_members_context_menu);

  connect(member_filter_, &QLineEdit::textChanged, this,
          [this]() { filter_timer_->start(); });
  connect(filter_timer_, &QTimer::timeout, this, [this]() {
    members_view_->SetSearchKeywords(member_filter_->text().trimmed());
  });

  auto* find_shortcut = new QShortcut(QKeySequence::Find, this);
  connect(find_shortcut, &QShortcut::activated, this,
          [this]() { available_list_->FocusSearchBar(); });

  auto* remove_shortcut = new QShortcut(QKeySequence::Delete, members_view_);
  remove_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
  connect(remove_shortcut, &QShortcut::activated, this,
          &KeyGroupManageDialog::slot_remove_from_key_group);
}

auto KeyGroupManageDialog::group() const -> QSharedPointer<GpgKeyGroup> {
  if (group_id_.isEmpty()) return nullptr;
  return KeyGroupRepository::GetInstance(channel_).KeyGroup(group_id_);
}

void KeyGroupManageDialog::refresh_header() {
  auto key_group = group();
  if (key_group == nullptr) return;

  name_label_->setText(key_group->Name());

  QStringList identity;
  if (!key_group->Email().isEmpty()) identity << key_group->Email();
  if (!key_group->Comment().isEmpty()) identity << key_group->Comment();
  identity << tr("created %1")
                  .arg(QLocale().toString(key_group->CreationTime(),
                                          QLocale::ShortFormat));
  identity_label_->setText(identity.join(" · "));

  int direct = 0;
  int nested = 0;
  int missing = 0;
  auto& getter = AbstractKeyRepository::GetInstance(channel_);

  for (const auto& key_id : key_group->KeyIds()) {
    if (getter.GetKey(key_id) == nullptr) {
      missing++;
    } else if (IsKeyGroupID(key_id)) {
      nested++;
    } else {
      direct++;
    }
  }

  footer_label_->setText(DescribeKeyGroupMembership(direct, nested, missing));
}

void KeyGroupManageDialog::showEvent(QShowEvent* event) {
  GeneralDialog::showEvent(event);

  // If the window state has not been restored, move the dialog to the center of
  // the parent window (if has parent) or the screen.
  if (!isRectRestored()) {
    movePosition2CenterOfParent();
  }

  if (!invalid_prompt_shown_) {
    invalid_prompt_shown_ = true;
    QTimer::singleShot(0, this,
                       &KeyGroupManageDialog::slot_notify_invalid_key_ids);
  }
}

void KeyGroupManageDialog::slot_add_to_key_group() {
  if (group() == nullptr) {
    close();
    return;
  }

  auto keys = available_list_->GetCheckedKeys();
  QStringList failed_ids;
  auto& getter = KeyGroupRepository::GetInstance(channel_);

  for (const auto& key : keys) {
    if (key == nullptr) continue;
    if (!getter.AddKey2KeyGroup(group_id_, key)) failed_ids << key->ID();
  }

  refresh_after_mutation(failed_ids, tr("Some Keys Failed"),
                         tr("Some keys could not be added to the group:\n%1"));
}

void KeyGroupManageDialog::slot_remove_from_key_group() {
  if (group() == nullptr) {
    close();
    return;
  }

  // Ids, not keys: the tree hands back non-owning pointers that die with the
  // model, and the model is rebuilt right below.
  const auto key_ids = members_view_->GetAllCheckedKeyIds();
  QStringList failed_ids;
  auto& getter = KeyGroupRepository::GetInstance(channel_);

  for (const auto& key_id : key_ids) {
    if (!getter.RemoveKeyFromKeyGroup(group_id_, key_id)) failed_ids << key_id;
  }

  refresh_after_mutation(
      failed_ids, tr("Some Keys Failed"),
      tr("Some keys could not be removed from the group:\n%1"));
}

void KeyGroupManageDialog::refresh_after_mutation(const QStringList& failed_ids,
                                                  const QString& failure_title,
                                                  const QString& failure_text) {
  members_view_->Refresh();
  available_list_->RefreshKeyTable(kAvailableTabIndex);

  refresh_header();
  slot_update_action_state();

  if (!failed_ids.isEmpty()) {
    QMessageBox::warning(this, failure_title,
                         failure_text.arg(failed_ids.join(", ")));
  }

  // Every other view of the keyring shows key groups too, so one signal per
  // batch keeps them from going stale.
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
}

void KeyGroupManageDialog::slot_edit_metadata() {
  auto key_group = group();
  if (key_group == nullptr) {
    close();
    return;
  }

  KeyGroupEditDialog dialog(key_group->Name(), key_group->Email(),
                            key_group->Comment(), this);
  if (dialog.exec() != QDialog::Accepted) return;

  if (!KeyGroupRepository::GetInstance(channel_).UpdateMetadata(
          group_id_, dialog.Name(), dialog.Email(), dialog.Comment())) {
    QMessageBox::warning(this, tr("Update Failed"),
                         tr("This key group could not be updated."));
    return;
  }

  refresh_header();
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
}

void KeyGroupManageDialog::slot_delete_group() {
  auto key_group = group();
  if (key_group == nullptr) {
    close();
    return;
  }

  auto& getter = KeyGroupRepository::GetInstance(channel_);

  QStringList parent_names;
  for (const auto& parent_id : getter.ParentsOf(group_id_)) {
    auto parent = getter.KeyGroup(parent_id);
    parent_names << (parent != nullptr ? parent->Name() : parent_id);
  }

  QMessageBox box(this);
  box.setIcon(QMessageBox::Question);
  box.setWindowTitle(tr("Delete Key Group"));
  // Plain text: a group name may contain characters Qt would read as markup.
  box.setTextFormat(Qt::PlainText);
  box.setText(DescribeKeyGroupDeletion(key_group->Name(), parent_names));
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::No);

  if (box.exec() != QMessageBox::Yes) return;

  if (!getter.Remove(group_id_)) {
    QMessageBox::warning(this, tr("Delete Failed"),
                         tr("This key group could not be deleted."));
    return;
  }

  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
  close();
}

void KeyGroupManageDialog::slot_reload() {
  if (group() == nullptr) {
    close();
    return;
  }

  refresh_header();
  members_view_->Refresh();
  available_list_->RefreshKeyTable(kAvailableTabIndex);
  slot_update_action_state();
}

void KeyGroupManageDialog::slot_update_action_state() {
  const auto has_group = group() != nullptr;

  add_button_->setEnabled(has_group &&
                          !available_list_->GetCheckedKeys().isEmpty());
  remove_button_->setEnabled(has_group &&
                             !members_view_->GetAllCheckedKeyIds().isEmpty());
}

void KeyGroupManageDialog::slot_members_context_menu(const QPoint& pos) {
  const auto index = members_view_->indexAt(pos);
  if (!index.isValid()) return;

  auto key = members_view_->GetKeyByIndex(index);
  if (key == nullptr) return;

  // Only a direct member is this group's to remove; anything deeper belongs to
  // the nested group it came from.
  const auto is_direct_member = !index.parent().isValid();

  QMenu menu(this);

  if (is_direct_member) {
    auto* remove_action =
        menu.addAction(QIcon(":/icons/minus.png"), tr("Remove from Group"));
    connect(remove_action, &QAction::triggered, this, [this, id = key->ID()]() {
      QStringList failed_ids;
      if (!KeyGroupRepository::GetInstance(channel_).RemoveKeyFromKeyGroup(
              group_id_, id)) {
        failed_ids << id;
      }
      refresh_after_mutation(
          failed_ids, tr("Some Keys Failed"),
          tr("Some keys could not be removed from the group:\n%1"));
    });
  }

  if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
    auto* open_action =
        menu.addAction(QIcon(":/icons/key-group.png"), tr("Manage Group…"));
    connect(open_action, &QAction::triggered, this,
            [this, key]() { ShowKeyDetails(this, channel_, key); });
  } else {
    auto* details_action =
        menu.addAction(QIcon(":/icons/detail.png"), tr("Key Details…"));
    connect(details_action, &QAction::triggered, this,
            [this, key]() { ShowKeyDetails(this, channel_, key); });
  }

  if (menu.isEmpty()) return;
  menu.exec(members_view_->viewport()->mapToGlobal(pos));
}

void KeyGroupManageDialog::slot_notify_invalid_key_ids() {
  auto key_group = group();
  if (key_group == nullptr) return;

  auto& getter = AbstractKeyRepository::GetInstance(channel_);

  QStringList invalid_key_ids;
  for (const auto& key_id : key_group->KeyIds()) {
    if (getter.GetKey(key_id) == nullptr) invalid_key_ids << key_id;
  }

  if (invalid_key_ids.isEmpty()) return;

  const auto message =
      tr("This Key Group contains some invalid keys:\n\n%1\n\n"
         "These keys are no longer available. Do you want to remove them from "
         "the group?")
          .arg(invalid_key_ids.join(", "));

  const auto reply = QMessageBox::question(
      this, tr("Invalid Keys in Group"), message,
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

  if (reply != QMessageBox::Yes) return;

  auto& key_group_getter = KeyGroupRepository::GetInstance(channel_);
  for (const auto& key_id : invalid_key_ids) {
    key_group_getter.RemoveKeyFromKeyGroup(group_id_, key_id);
  }

  refresh_after_mutation({}, {}, {});
}

}  // namespace GpgFrontend::UI
