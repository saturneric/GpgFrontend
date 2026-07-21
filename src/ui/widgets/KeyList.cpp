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

#include "ui/widgets/KeyList.h"

#include <cstddef>

#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/function/openpgp/helper/OpSupport.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/model/GpgImportInformation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/KeyGroupCreationDialog.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"

//
#include "ui_KeyList.h"

namespace GpgFrontend::UI {

namespace {

// Custom (user-defined) categories share one global order across every key
// list; integrated built-in tabs keep a per-window order instead.
constexpr auto kCustomCategoryOrderKey = "keys/custom_category_order";

auto IsCustomCategoryId(const QString& id) -> bool {
  return id.startsWith("cat:");
}

// Order the ids that are actually present to match `saved`, appending any
// present id that is not in `saved` at the end (preserving its current order).
auto OrderIdsBy(const QStringList& present, const QStringList& saved)
    -> QStringList {
  QStringList result;
  for (const auto& id : saved) {
    if (present.contains(id) && !result.contains(id)) result << id;
  }
  for (const auto& id : present) {
    if (!result.contains(id)) result << id;
  }
  return result;
}

auto IsOwnerTrustSupported(int channel) -> bool {
  return GpgFrontend::IsOpSupported<GpgFrontend::SetOwnerTrustLevelOpTag>(
      channel);
}

auto ApplyEngineColumnFilter(int channel, GpgKeyTableColumn columns)
    -> GpgKeyTableColumn {
  if (!IsOwnerTrustSupported(channel)) {
    columns = columns & ~GpgKeyTableColumn::kOWNER_TRUST;
  }

  return columns;
}

}  // namespace

KeyList::KeyList(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyList>()),
      model_(AbstractKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetGpgKeyTableModel()),
      global_column_filter_(static_cast<GpgKeyTableColumn>(
          GetSettings()
              .value("keys/global_columns_filter",
                     static_cast<unsigned int>(GpgKeyTableColumn::kALL))
              .toUInt())) {
  ui_->setupUi(this);
}

KeyList::KeyList(int channel, KeyMenuAbility menu_ability,
                 GpgKeyTableColumn fixed_columns_filter, QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyList>()),
      current_gpg_context_channel_(channel),
      menu_ability_(menu_ability),
      model_(AbstractKeyRepository::GetInstance(channel).GetGpgKeyTableModel()),
      fixed_columns_filter_(fixed_columns_filter),
      global_column_filter_(static_cast<GpgKeyTableColumn>(
          GetSettings()
              .value("keys/global_columns_filter",
                     static_cast<unsigned int>(GpgKeyTableColumn::kALL))
              .toUInt())) {
  ui_->setupUi(this);
  init();
}

auto KeyList::has_ability(KeyMenuAbility ability) const -> bool {
  using T = std::underlying_type_t<KeyMenuAbility>;

  return (static_cast<T>(menu_ability_) & static_cast<T>(ability)) !=
         static_cast<T>(KeyMenuAbility::kNONE);
}

void KeyList::init_ui_visibility() {
  ui_->menuWidget->setHidden(menu_ability_ == KeyMenuAbility::kNONE);

  ui_->refreshKeyListButton->setHidden(!has_ability(KeyMenuAbility::kREFRESH));
  ui_->syncButton->setHidden(!has_ability(KeyMenuAbility::kSYNC_PUBLIC_KEY));
  ui_->checkALLButton->setHidden(!has_ability(KeyMenuAbility::kCHECK_ALL));
  ui_->uncheckButton->setHidden(!has_ability(KeyMenuAbility::kUNCHECK_ALL));
  ui_->columnTypeButton->setHidden(
      !has_ability(KeyMenuAbility::kCOLUMN_FILTER));
  ui_->searchBarEdit->setHidden(!has_ability(KeyMenuAbility::kSEARCH_BAR));
  ui_->switchContextButton->setHidden(
      !has_ability(KeyMenuAbility::kKEY_DATABASE));
  ui_->keyGroupButton->setHidden(!has_ability(KeyMenuAbility::kKEY_GROUP));
}

void KeyList::init_ui_style() {
  setObjectName(QStringLiteral("KeyList"));

  ui_->menuWidget->setObjectName(QStringLiteral("KeyListMenu"));
  ui_->categoryList->setObjectName(QStringLiteral("KeyCategoryList"));

  ui_->searchBarEdit->setClearButtonEnabled(true);
  ui_->searchBarEdit->setMinimumHeight(30);

  const auto setup_tool_button = [](QToolButton* button) {
    button->setMinimumHeight(28);
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIconSize(QSize(16, 16));
    // Flat buttons with a native hover highlight keep the toolbar light.
    button->setAutoRaise(true);
  };

  setup_tool_button(ui_->refreshKeyListButton);
  setup_tool_button(ui_->syncButton);
  setup_tool_button(ui_->uncheckButton);
  setup_tool_button(ui_->checkALLButton);
  setup_tool_button(ui_->columnTypeButton);
  setup_tool_button(ui_->keyGroupButton);

  // The context button keeps its label (the current key database name) beside
  // the icon, replacing the old LCD channel indicator.
  ui_->switchContextButton->setMinimumHeight(28);
  ui_->switchContextButton->setFocusPolicy(Qt::NoFocus);
  ui_->switchContextButton->setSizePolicy(QSizePolicy::Maximum,
                                          QSizePolicy::Fixed);
  ui_->switchContextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  ui_->switchContextButton->setIconSize(QSize(16, 16));
  ui_->switchContextButton->setAutoRaise(true);

  ui_->columnTypeButton->setPopupMode(QToolButton::InstantPopup);
  ui_->switchContextButton->setPopupMode(QToolButton::InstantPopup);

  // Category source list: a clean, frameless sidebar that drives the stacked
  // key tables. Reordering is by drag; the order is persisted across restarts.
  ui_->categoryList->setFrameShape(QFrame::NoFrame);
  ui_->categoryList->setUniformItemSizes(true);
  ui_->categoryList->setAlternatingRowColors(false);
  ui_->categoryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui_->categoryList->setSelectionMode(QAbstractItemView::SingleSelection);
  ui_->categoryList->setDragDropMode(QAbstractItemView::InternalMove);
  ui_->categoryList->setContextMenuPolicy(Qt::CustomContextMenu);

  // Add-category (+) button; only shown when management is enabled.
  ui_->addCategoryButton->setAutoRaise(true);
  ui_->addCategoryButton->setFocusPolicy(Qt::NoFocus);
  ui_->addCategoryButton->setToolTip(tr("New Category..."));
  ui_->addCategoryButton->setVisible(category_management_enabled_);

  apply_category_strip_style();

  // Give the key table the lion's share of the panel width.
  ui_->keyListSplitter->setStretchFactor(0, 0);
  ui_->keyListSplitter->setStretchFactor(1, 1);

  // Keep the toolbar/search block at its natural height and let the splitter
  // absorb all the remaining vertical space; otherwise the layout spreads the
  // extra height evenly and leaves large gaps between the rows.
  ui_->menuWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  ui_->keyListSplitter->setSizePolicy(QSizePolicy::Expanding,
                                      QSizePolicy::Expanding);
}

void KeyList::SetCategoryRailCompact(bool compact) {
  if (compact_rail_ == compact) return;
  compact_rail_ = compact;

  if (ui_ == nullptr || ui_->categoryList == nullptr) return;

  apply_category_strip_style();

  // Re-render any existing rows so their icon/text matches the new mode.
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    auto* item = ui_->categoryList->item(i);
    const auto id = item->data(Qt::UserRole).toString();
    const auto name = item->toolTip();

    item->setIcon(make_category_icon(resolve_category_color(id, {}),
                                     IsCustomCategoryId(id)));
    item->setText(compact_rail_ ? QString() : name);
  }
}

void KeyList::SetCategoryManagementEnabled(bool enabled) {
  category_management_enabled_ = enabled;
  if (ui_ != nullptr && ui_->addCategoryButton != nullptr) {
    ui_->addCategoryButton->setVisible(enabled);
  }
}

void KeyList::apply_category_strip_style() {
  if (ui_ == nullptr || ui_->categoryList == nullptr) return;

  if (compact_rail_) {
    ui_->categoryList->setIconSize(QSize(20, 20));
    ui_->categoryList->setMinimumWidth(0);
    // Hard-cap the rail so the splitter cannot widen it past a thin strip.
    ui_->categoryPane->setMinimumWidth(28);
    ui_->categoryPane->setMaximumWidth(30);
    ui_->categoryList->setTextElideMode(Qt::ElideNone);
    ui_->keyListSplitter->setSizes({28, 560});
    // Neutral, theme-independent selection/hover so it never competes with the
    // colour swatches.
    ui_->categoryList->setStyleSheet(R"(
QListWidget#KeyCategoryList {
  outline: 0;
  background: transparent;
}

QListWidget#KeyCategoryList::item {
  padding: 3px 1px;
  border: none;
  border-radius: 4px;
}

QListWidget#KeyCategoryList::item:selected {
  background: rgba(128, 128, 128, 0.30);
}

QListWidget#KeyCategoryList::item:hover:!selected {
  background: rgba(128, 128, 128, 0.16);
}
)");
    apply_category_item_metrics();
    return;
  }

  ui_->categoryList->setIconSize(QSize(16, 16));
  ui_->categoryList->setMinimumWidth(120);
  ui_->categoryPane->setMinimumWidth(120);
  ui_->categoryPane->setMaximumWidth(QWIDGETSIZE_MAX);
  ui_->categoryList->setTextElideMode(Qt::ElideRight);
  ui_->keyListSplitter->setSizes({150, 470});
  ui_->categoryList->setStyleSheet(R"(
QListWidget#KeyCategoryList {
  outline: 0;
  background: transparent;
}

QListWidget#KeyCategoryList::item {
  padding: 6px 10px;
  border: none;
  border-radius: 4px;
}

QListWidget#KeyCategoryList::item:selected {
  background: palette(highlight);
  color: palette(highlighted-text);
}

QListWidget#KeyCategoryList::item:hover:!selected {
  background: palette(alternate-base);
}
)");
  apply_category_item_metrics();
}

auto KeyList::resolve_category_color(const QString& id,
                                     const QString& hint) const -> QColor {
  // A user-chosen colour (or a custom category's own colour) from the category
  // cache wins over the built-in defaults.
  const auto stored =
      KeyCategoryRepository::GetInstance(current_gpg_context_channel_)
          .GetTabColor(id);
  if (!stored.isEmpty()) {
    if (QColor c(stored); c.isValid()) return c;
  }

  if (!hint.isEmpty()) {
    if (QColor c(hint); c.isValid()) return c;
  }

  // Semantic default colours for the well-known built-in tabs.
  static const QHash<QString, QString> kBuiltinColors = {
      {"default", "#3B82F6"},          // blue
      {"mine", "#10B981"},             // emerald
      {"key_group", "#8B5CF6"},        // violet
      {"only_public_key", "#06B6D4"},  // cyan
      {"has_private_key", "#F43F5E"},  // rose
      {"all", "#64748B"},              // slate
      {"no_primary_key", "#9CA3AF"},   // gray
      {"revoked", "#EF4444"},          // red
      {"expired", "#F97316"},          // orange
      {"disabled", "#71717A"},         // zinc
  };

  if (const auto it = kBuiltinColors.constFind(id);
      it != kBuiltinColors.constEnd()) {
    return QColor(*it);
  }

  // Stable, well-spread colour derived from the id for custom categories with
  // no explicit colour.
  const auto hue = static_cast<int>(qHash(id) % 360U);
  return QColor::fromHsv(hue, 160, 205);
}

auto KeyList::make_category_icon(const QColor& color, bool custom) const
    -> QIcon {
  const qreal dpr = devicePixelRatioF() > 0 ? devicePixelRatioF() : 1.0;
  // Square canvas so the same icon scales cleanly at both strip icon sizes.
  constexpr int kSide = 28;
  constexpr int kSwatch = 16;

  QPixmap pm(QSize(kSide, kSide) * dpr);
  pm.setDevicePixelRatio(dpr);
  pm.fill(Qt::transparent);

  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QRectF r((kSide - kSwatch) / 2.0, (kSide - kSwatch) / 2.0, kSwatch,
                 kSwatch);
  QColor border = color.darker(125);
  border.setAlpha(190);
  p.setPen(QPen(border, 1));
  p.setBrush(color);

  // Shape encodes the category kind: built-in tabs are rounded squares, custom
  // (user-defined) categories are circles.
  if (custom) {
    p.drawEllipse(r);
  } else {
    p.drawRoundedRect(r, 5, 5);
  }
  p.end();

  return QIcon(pm);
}

auto KeyList::category_item_size_hint() const -> QSize {
  const auto icon = ui_->categoryList->iconSize();

  // Compact rail: swatch only, so the row is purely the icon plus the padding
  // from the stylesheet. Keeping it off the font/style metrics is what makes
  // the swatch spacing identical on every platform.
  if (compact_rail_) return {icon.width() + 2, icon.height() + 6};

  // Text list: the row must still grow with the UI font, but nothing else.
  const auto text_height = ui_->categoryList->fontMetrics().height();
  return {icon.width() + 20, qMax(icon.height(), text_height) + 12};
}

void KeyList::apply_category_item_metrics() {
  const auto hint = category_item_size_hint();
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    ui_->categoryList->item(i)->setSizeHint(hint);
  }
}

void KeyList::init_texts() {
  ui_->refreshKeyListButton->setText(tr("Refresh"));
  ui_->refreshKeyListButton->setToolTip(
      tr("Refresh the key list to synchronize changes."));

  ui_->syncButton->setText(tr("Sync Public Key"));
  ui_->syncButton->setToolTip(
      tr("Sync public keys with the default keyserver."));

  ui_->uncheckButton->setText(tr("Uncheck All"));
  ui_->uncheckButton->setToolTip(tr("Uncheck all keys in the current tab."));

  ui_->checkALLButton->setText(tr("Check All"));
  ui_->checkALLButton->setToolTip(tr("Check all keys in the current tab."));

  ui_->searchBarEdit->setPlaceholderText(
      tr("Search keys by user ID, key ID, fingerprint..."));

  ui_->columnTypeButton->setText(tr("Columns"));
  ui_->columnTypeButton->setToolTip(tr("Choose visible key table columns."));

  ui_->keyGroupButton->setText(tr("New Key Group"));
  ui_->keyGroupButton->setToolTip(
      tr("Create a key group from checked encryption-capable keys."));

  // The visible text is the active key database name (set by
  // init_context_menu / set_context_button_text); only the tooltip is fixed.
  ui_->switchContextButton->setToolTip(tr("Switch between key databases."));
}

void KeyList::init_context_menu() {
  auto* gpg_context_menu = new QMenu(this);
  auto* gpg_context_groups = new QActionGroup(this);
  gpg_context_groups->setExclusive(true);

  // Baseline label; overridden below with the current database name if any.
  set_context_button_text(QString());

  const auto key_db_infos = GetGpgKeyDatabaseInfos();

  for (const auto& key_db_info : key_db_infos) {
    const auto channel = key_db_info.channel;
    const auto key_db_name = key_db_info.name;

    const auto engine = ConvertOpenPGPEngine2String(
        OpenPGPContext::GetInstance(channel).Engine());

    auto* action = new QAction(QString("%1  ·  %2  ·  %3")
                                   .arg(key_db_name)
                                   .arg(engine.toUpper())
                                   .arg(tr("Channel %1").arg(channel)),
                               this);

    action->setCheckable(true);
    action->setChecked(channel == current_gpg_context_channel_);

    if (channel == current_gpg_context_channel_) {
      set_context_button_text(key_db_name);
    }

    connect(action, &QAction::toggled, this,
            [this, channel, key_db_name](bool checked) {
              if (!checked) return;

              current_gpg_context_channel_ = channel;
              set_context_button_text(key_db_name);

              init_column_menu();
              UpdateKeyTableColumnType(global_column_filter_);

              emit SignalRefreshDatabase();
            });

    gpg_context_groups->addAction(action);
    gpg_context_menu->addAction(action);
  }

  if (gpg_context_menu->isEmpty()) {
    auto* empty_action =
        gpg_context_menu->addAction(tr("No key database available"));
    empty_action->setEnabled(false);
  }

  ui_->switchContextButton->setMenu(gpg_context_menu);
}

void KeyList::set_context_button_text(const QString& db_name) {
  ui_->switchContextButton->setText(db_name.isEmpty() ? tr("Key Database")
                                                      : db_name);
}

void KeyList::init_column_menu() {
  auto* column_type_menu = new QMenu(this);

  auto add_column_action = [this, column_type_menu](QAction*& action,
                                                    const QString& title,
                                                    GpgKeyTableColumn column) {
    action = new QAction(title, this);
    action->setCheckable(true);
    action->setChecked((global_column_filter_ & column) !=
                       GpgKeyTableColumn::kNONE);

    connect(action, &QAction::toggled, this, [this, column](bool checked) {
      UpdateKeyTableColumnType(checked ? global_column_filter_ | column
                                       : global_column_filter_ & ~column);
    });

    auto effective_fixed_columns = ApplyEngineColumnFilter(
        current_gpg_context_channel_, fixed_columns_filter_);

    if ((effective_fixed_columns & column) != GpgKeyTableColumn::kNONE) {
      column_type_menu->addAction(action);
    }
  };

  add_column_action(key_id_column_action_, tr("Key ID"),
                    GpgKeyTableColumn::kKEY_ID);
  add_column_action(algo_column_action_, tr("Algorithm"),
                    GpgKeyTableColumn::kALGO);
  add_column_action(create_date_column_action_, tr("Create Date"),
                    GpgKeyTableColumn::kCREATE_DATE);
  add_column_action(owner_trust_column_action_, tr("Owner Trust"),
                    GpgKeyTableColumn::kOWNER_TRUST);
  add_column_action(subkeys_number_column_action_, tr("Subkeys"),
                    GpgKeyTableColumn::kSUBKEYS_NUMBER);
  add_column_action(comment_column_action_, tr("Comment"),
                    GpgKeyTableColumn::kCOMMENT);

  if (column_type_menu->isEmpty()) {
    auto* empty_action = column_type_menu->addAction(tr("No optional columns"));
    empty_action->setEnabled(false);
  }

  column_type_menu->addSeparator();
  connect(column_type_menu->addAction(tr("Reset Column Widths")),
          &QAction::triggered, this, [this]() {
            for (auto* page : pages_) page->ResetColumnWidths();
          });

  ui_->columnTypeButton->setMenu(column_type_menu);
}

void KeyList::init_signals() {
  connect(this, &KeyList::SignalRefreshDatabase, UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyList::SlotRefresh);
  connect(UISignalStation::GetInstance(), &UISignalStation::SignalUIRefresh,
          this, &KeyList::SlotRefreshUI);

  connect(CommonUtils::GetInstance(), &CommonUtils::SignalCategoriesChanged,
          this, &KeyList::RebuildCategoryTabs);

  // Switch the shown key table when the selected category row changes.
  connect(ui_->categoryList, &QListWidget::currentRowChanged, this,
          &KeyList::slot_current_category_changed);

  // Right-click a category row for the context menu.
  connect(ui_->categoryList, &QListWidget::customContextMenuRequested, this,
          &KeyList::slot_category_context_menu);

  // The add-category (+) button creates a new custom category.
  connect(ui_->addCategoryButton, &QToolButton::clicked, this,
          [this]() { new_category(); });

  // Keep colour swatches consistent across open key lists.
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyCategoryColorsChanged, this,
          &KeyList::refresh_category_icons);

  // Persist the order after a drag-reorder of the category rows (keeping the
  // primary category pinned to the top). Deferred so the model is no longer
  // mutating inside its own rowsMoved emission.
  connect(ui_->categoryList->model(), &QAbstractItemModel::rowsMoved, this,
          [this](const QModelIndex&, int, int, const QModelIndex&, int) {
            if (applying_tab_order_) return;
            QTimer::singleShot(0, this, [this]() {
              // Persist the new sub-orders, then re-apply to normalise the
              // display (integrated block, then custom block, primary pinned).
              save_tab_order();
              apply_saved_tab_order();
            });
          });

  // Re-apply the (shared) custom order live when another key list reorders its
  // custom categories.
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyCategoryTabOrderChanged, this,
          [this](const QString& key) {
            if (key == kCustomCategoryOrderKey) apply_saved_tab_order();
          });

  connect(ui_->refreshKeyListButton, &QPushButton::clicked, this,
          &KeyList::SignalRefreshDatabase);

  connect(ui_->uncheckButton, &QPushButton::clicked, this,
          &KeyList::uncheck_all);
  connect(ui_->checkALLButton, &QPushButton::clicked, this,
          &KeyList::check_all);
  connect(ui_->syncButton, &QPushButton::clicked, this,
          &KeyList::slot_sync_with_key_server);

  search_timer_ = new QTimer(this);
  search_timer_->setSingleShot(true);
  search_timer_->setInterval(180);

  connect(ui_->searchBarEdit, &QLineEdit::textChanged, this,
          [this]() { search_timer_->start(); });

  connect(search_timer_, &QTimer::timeout, this, &KeyList::filter_by_keyword);

  connect(this, &KeyList::SignalRefreshStatusBar,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshStatusBar);

  connect(this, &KeyList::SignalColumnTypeChange, this, [this]() {
    GetSettings().setValue(column_filter_settings_key_,
                           static_cast<unsigned int>(global_column_filter_));
  });

  connect(ui_->keyGroupButton, &QPushButton::clicked, this,
          &KeyList::slot_new_key_group);

  connect(this, &KeyList::SignalKeyChecked, this,
          [this]() { update_action_state(); });
}

void KeyList::update_action_state() {
  const bool has_key_table = current_page() != nullptr;

  if (!has_key_table) {
    ui_->keyGroupButton->setEnabled(false);
    ui_->checkALLButton->setEnabled(false);
    ui_->uncheckButton->setEnabled(false);

    if (!ui_->syncButton->isHidden()) {
      ui_->syncButton->setEnabled(false);
    }

    return;
  }

  ui_->checkALLButton->setEnabled(true);
  ui_->uncheckButton->setEnabled(true);

  const auto keys = GetCheckedKeys();

  bool can_create_group = !keys.empty();
  for (const auto& key : keys) {
    if (key == nullptr || !key->IsHasEncrCap()) {
      can_create_group = false;
      break;
    }
  }

  ui_->keyGroupButton->setEnabled(can_create_group);

  if (!ui_->syncButton->isHidden()) {
    ui_->syncButton->setEnabled(true);
  }
}

void KeyList::init() {
  init_ui_visibility();
  init_ui_style();
  init_context_menu();
  init_column_menu();

  pages_.clear();
  pinned_first_id_.clear();
  while (ui_->keyStack->count() > 0) {
    auto* w = ui_->keyStack->widget(0);
    ui_->keyStack->removeWidget(w);
    w->deleteLater();
  }
  ui_->categoryList->clear();

  ui_->syncButton->setHidden(
      !Module::IsEventListening("REQUEST_GET_PUBLIC_KEY_BY_KEY_ID"));

  init_signals();
  init_texts();
  update_action_state();

  setAcceptDrops(true);
}

auto KeyList::AddListGroupTab(const QString& name, const QString& id,
                              GpgKeyTableDisplayMode display_mode,
                              GpgKeyTableProxyModel::KeyFilter search_filter,
                              GpgKeyTableColumn custom_columns_filter,
                              const QString& category_id,
                              const QString& color_hint) -> KeyTable* {
  auto* key_table =
      new KeyTable(this, model_, display_mode, custom_columns_filter,
                   std::move(search_filter), category_id);

  key_table->setObjectName(id);
  key_table->SetColumnWidthsScope(column_widths_scope_);

  ui_->keyStack->addWidget(key_table);
  pages_.insert(id, key_table);

  // The first category added is the primary one, pinned to the top.
  if (pinned_first_id_.isEmpty()) pinned_first_id_ = id;

  auto* item = new QListWidgetItem(ui_->categoryList);
  item->setData(Qt::UserRole, id);
  // The name lives in the tooltip so it is available in both strip modes (the
  // compact rail hides the text) and for the delete-category prompt.
  item->setToolTip(name);

  // Both modes carry the colour swatch; the text list also shows the name.
  item->setIcon(make_category_icon(resolve_category_color(id, color_hint),
                                   IsCustomCategoryId(id)));
  if (!compact_rail_) item->setText(name);
  item->setSizeHint(category_item_size_hint());

  if (ui_->categoryList->currentRow() < 0) {
    ui_->categoryList->setCurrentRow(0);
  }

  connect(this, &KeyList::SignalColumnTypeChange, key_table,
          &KeyTable::SignalColumnTypeChange);
  connect(key_table, &KeyTable::SignalKeyChecked, this, [=]() {
    if (sender() != current_page()) return;
    emit SignalKeyChecked();
  });

  // Column widths are shared by every tab of this key list, so a resize in one
  // tab has to be picked up by all the others.
  connect(key_table, &KeyTable::SignalColumnWidthChanged, this, [this]() {
    auto* source = qobject_cast<KeyTable*>(sender());
    for (auto* page : pages_) {
      if (page == source) continue;
      page->ReloadColumnWidths();
    }
  });

  UpdateKeyTableColumnType(global_column_filter_);
  return key_table;
}

void KeyList::RebuildCategoryTabs() {
  auto categories =
      KeyCategoryRepository::GetInstance(current_gpg_context_channel_).Fetch();

  QSet<QString> desired_ids;
  for (const auto& c : categories) {
    if (c.builtin) continue;  // built-in tabs are added by the host window
    desired_ids.insert(c.id);
  }

  // Drop rows whose category no longer exists.
  for (int i = ui_->categoryList->count() - 1; i >= 0; --i) {
    const auto id = ui_->categoryList->item(i)->data(Qt::UserRole).toString();
    if (!id.startsWith("cat:") || desired_ids.contains(id)) continue;

    delete ui_->categoryList->takeItem(i);
    if (auto* page = pages_.take(id); page != nullptr) {
      ui_->keyStack->removeWidget(page);
      page->deleteLater();
    }
  }

  // Add a row for every category that does not have one yet, and update the
  // name/colour of those that already exist (handles rename and recolour).
  for (const auto& c : categories) {
    if (c.builtin) continue;

    if (!pages_.contains(c.id)) {
      AddListGroupTab(
          c.name, c.id,
          GpgKeyTableDisplayMode::kPUBLIC_KEY |
              GpgKeyTableDisplayMode::kPRIVATE_KEY,
          [](const GpgAbstractKey*) -> bool { return true; },
          GpgKeyTableColumn::kALL, c.id, c.color);
      continue;
    }

    if (auto* item = item_for_id(c.id); item != nullptr) {
      item->setToolTip(c.name);
      if (!compact_rail_) item->setText(c.name);
      item->setIcon(
          make_category_icon(resolve_category_color(c.id, c.color), true));
    }
  }

  apply_saved_tab_order();
}

void KeyList::SetTabOrderSettingsKey(const QString& settings_key) {
  if (!settings_key.isEmpty()) tab_order_settings_key_ = settings_key;
}

void KeyList::SetColumnFilterSettingsKey(const QString& settings_key,
                                         GpgKeyTableColumn default_columns) {
  if (!settings_key.isEmpty()) column_filter_settings_key_ = settings_key;

  global_column_filter_ = static_cast<GpgKeyTableColumn>(
      GetSettings()
          .value(column_filter_settings_key_,
                 static_cast<unsigned int>(default_columns))
          .toUInt());

  // Rebuild the chooser so its checkmarks reflect the (possibly new) state,
  // then apply the filter to every existing tab.
  init_column_menu();
  UpdateKeyTableColumnType(global_column_filter_);
}

void KeyList::SetColumnWidthsScope(const QString& scope) {
  if (scope.isEmpty()) return;

  column_widths_scope_ = scope;
  for (auto* page : pages_) page->SetColumnWidthsScope(column_widths_scope_);
}

void KeyList::save_tab_order() {
  // Persist the integrated (built-in) order per window and the custom-category
  // order globally, so the two never influence each other.
  QStringList integrated;
  QStringList custom;
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    const auto id = ui_->categoryList->item(i)->data(Qt::UserRole).toString();
    (IsCustomCategoryId(id) ? custom : integrated) << id;
  }

  auto& repo = KeyCategoryRepository::GetInstance(current_gpg_context_channel_);
  repo.SetTabOrder(tab_order_settings_key_, integrated);
  repo.SetTabOrder(kCustomCategoryOrderKey, custom);

  // The custom order is shared, so tell other open key lists to re-apply it.
  emit UISignalStation::GetInstance()
      -> SignalKeyCategoryTabOrderChanged(kCustomCategoryOrderKey);
}

void KeyList::apply_saved_tab_order() {
  applying_tab_order_ = true;

  const auto current_id =
      ui_->categoryList->currentItem() != nullptr
          ? ui_->categoryList->currentItem()->data(Qt::UserRole).toString()
          : QString{};

  // Split the present ids into integrated and custom groups.
  QStringList cur_integrated;
  QStringList cur_custom;
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    const auto id = ui_->categoryList->item(i)->data(Qt::UserRole).toString();
    (IsCustomCategoryId(id) ? cur_custom : cur_integrated) << id;
  }

  // Order each group by its own saved order (persisted in the category cache).
  auto& repo = KeyCategoryRepository::GetInstance(current_gpg_context_channel_);
  auto ordered_integrated =
      OrderIdsBy(cur_integrated, repo.GetTabOrder(tab_order_settings_key_));
  const auto ordered_custom =
      OrderIdsBy(cur_custom, repo.GetTabOrder(kCustomCategoryOrderKey));

  // Keep the primary category at the top of the integrated group.
  if (!pinned_first_id_.isEmpty() &&
      ordered_integrated.removeOne(pinned_first_id_)) {
    ordered_integrated.prepend(pinned_first_id_);
  }

  // Merge: integrated block first, then the custom block.
  const QStringList desired = ordered_integrated + ordered_custom;

  int target = 0;
  for (const auto& id : desired) {
    for (int i = target; i < ui_->categoryList->count(); ++i) {
      if (ui_->categoryList->item(i)->data(Qt::UserRole).toString() != id) {
        continue;
      }
      if (i != target) {
        ui_->categoryList->insertItem(target, ui_->categoryList->takeItem(i));
      }
      ++target;
      break;
    }
  }

  // Restore the selection, which item moves may have disturbed.
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    if (ui_->categoryList->item(i)->data(Qt::UserRole).toString() ==
        current_id) {
      ui_->categoryList->setCurrentRow(i);
      break;
    }
  }

  applying_tab_order_ = false;
}

void KeyList::delete_category(const QString& id, const QString& name) {
  if (!id.startsWith("cat:")) return;  // safety: only categories are deletable

  auto ret = QMessageBox::question(
      this, tr("Delete Category"),
      tr("Delete category \"%1\"? This removes the grouping only; the keys "
         "themselves are not affected.")
          .arg(name),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (ret != QMessageBox::Yes) return;

  KeyCategoryRepository::GetInstance(current_gpg_context_channel_).Remove(id);
  CommonUtils::GetInstance()->NotifyCategoriesChanged();
}

auto KeyList::current_page() const -> KeyTable* {
  if (ui_ == nullptr || ui_->categoryList == nullptr) return nullptr;

  auto* item = ui_->categoryList->currentItem();
  if (item == nullptr) return nullptr;

  return page_for_id(item->data(Qt::UserRole).toString());
}

auto KeyList::page_for_id(const QString& id) const -> KeyTable* {
  return pages_.value(id, nullptr);
}

void KeyList::slot_current_category_changed(int row) {
  auto* item = row >= 0 ? ui_->categoryList->item(row) : nullptr;
  auto* page = item != nullptr
                   ? page_for_id(item->data(Qt::UserRole).toString())
                   : nullptr;

  if (page != nullptr) ui_->keyStack->setCurrentWidget(page);

  update_action_state();
  emit SignalKeyChecked();
}

void KeyList::slot_category_context_menu(const QPoint& pos) {
  // The main-window rail is a simple switcher: no management menu there.
  if (!category_management_enabled_) return;

  auto* item = ui_->categoryList->itemAt(pos);
  const auto id =
      item != nullptr ? item->data(Qt::UserRole).toString() : QString{};
  const auto name = item != nullptr ? item->toolTip() : QString{};
  const bool custom = IsCustomCategoryId(id);

  QMenu menu(this);

  auto* new_action = menu.addAction(tr("New Category..."));
  connect(new_action, &QAction::triggered, this, [this]() { new_category(); });

  if (item != nullptr) {
    menu.addSeparator();

    auto* color_action = menu.addAction(tr("Set Colour..."));
    connect(color_action, &QAction::triggered, this,
            [this, id]() { choose_category_color(id); });

    auto* reset_action = menu.addAction(tr("Reset Colour"));
    connect(reset_action, &QAction::triggered, this, [this, id]() {
      KeyCategoryRepository::GetInstance(current_gpg_context_channel_)
          .SetTabColor(id, QString{});
      emit UISignalStation::GetInstance() -> SignalKeyCategoryColorsChanged();
    });

    // Rename / membership / deletion apply to user categories only.
    if (custom) {
      menu.addSeparator();

      auto* rename_action = menu.addAction(tr("Rename Category..."));
      connect(rename_action, &QAction::triggered, this,
              [this, id, name]() { rename_category(id, name); });

      auto* delete_action = menu.addAction(tr("Delete Category..."));
      connect(delete_action, &QAction::triggered, this,
              [this, id, name]() { delete_category(id, name); });
    }
  }

  menu.exec(ui_->categoryList->viewport()->mapToGlobal(pos));
}

void KeyList::choose_category_color(const QString& id) {
  if (id.isEmpty()) return;

  const QColor current = resolve_category_color(id, {});
  const QColor picked =
      QColorDialog::getColor(current, this, tr("Choose Category Colour"));
  if (!picked.isValid()) return;

  KeyCategoryRepository::GetInstance(current_gpg_context_channel_)
      .SetTabColor(id, picked.name());

  emit UISignalStation::GetInstance() -> SignalKeyCategoryColorsChanged();
}

auto KeyList::item_for_id(const QString& id) const -> QListWidgetItem* {
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    auto* item = ui_->categoryList->item(i);
    if (item->data(Qt::UserRole).toString() == id) return item;
  }
  return nullptr;
}

void KeyList::select_category(const QString& id) {
  if (auto* item = item_for_id(id); item != nullptr) {
    ui_->categoryList->setCurrentItem(item);
  }
}

void KeyList::refresh_category_icons() {
  for (int i = 0; i < ui_->categoryList->count(); ++i) {
    auto* item = ui_->categoryList->item(i);
    const auto id = item->data(Qt::UserRole).toString();
    item->setIcon(make_category_icon(resolve_category_color(id, {}),
                                     IsCustomCategoryId(id)));
  }
}

void KeyList::new_category() {
  bool ok = false;
  const auto name =
      QInputDialog::getText(this, tr("New Category"), tr("Category name:"),
                            QLineEdit::Normal, QString{}, &ok)
          .trimmed();
  if (!ok || name.isEmpty()) return;

  const QColor color = QColorDialog::getColor(
      resolve_category_color(name, {}), this, tr("Category Colour (optional)"));

  auto& repo = KeyCategoryRepository::GetInstance(current_gpg_context_channel_);
  const auto id =
      repo.AddCategory(name, color.isValid() ? color.name() : QString{});

  CommonUtils::GetInstance()->NotifyCategoriesChanged();
  select_category(id);
}

void KeyList::rename_category(const QString& id, const QString& current_name) {
  bool ok = false;
  const auto name =
      QInputDialog::getText(this, tr("Rename Category"), tr("Category name:"),
                            QLineEdit::Normal, current_name, &ok)
          .trimmed();
  if (!ok || name.isEmpty() || name == current_name) return;

  KeyCategoryRepository::GetInstance(current_gpg_context_channel_)
      .Rename(id, name);
  CommonUtils::GetInstance()->NotifyCategoriesChanged();
}

void KeyList::SlotRefresh() {
  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  LOG_D() << "request new key table module, current gpg context channel: "
          << current_gpg_context_channel_;
  model_ = AbstractKeyRepository::GetInstance(current_gpg_context_channel_)
               .GetGpgKeyTableModel();

  for (int i = 0; i < ui_->keyStack->count(); i++) {
    auto* key_table = qobject_cast<KeyTable*>(ui_->keyStack->widget(i));
    if (key_table == nullptr) continue;

    key_table->RefreshModel(model_);
  }

  emit SignalRefreshStatusBar(tr("Refreshing Key List..."), 3000);
  this->SlotRefreshUI();
}

void KeyList::SlotRefreshUI() {
  emit SignalRefreshStatusBar(tr("Key List Refreshed."), 1000);
  ui_->refreshKeyListButton->setDisabled(false);
  ui_->syncButton->setDisabled(false);
  emit SignalKeyChecked();
}

auto KeyList::GetCheckedKeys() -> GpgAbstractKeyPtrList {
  auto* key_table = current_page();
  if (key_table == nullptr) return {};

  return key_table->GetCheckedKeys();
}

auto KeyList::GetCheckedPrivateKey() -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};

  auto keys = GetCheckedKeys();
  for (const auto& key : keys) {
    if (key->IsPrivateKey()) ret.push_back(key);
  }

  return ret;
}

auto KeyList::GetCheckedPublicKey() -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};

  auto keys = GetCheckedKeys();
  for (const auto& key : keys) {
    if (!key->IsPrivateKey()) ret.push_back(key);
  }

  return ret;
}

void KeyList::SetChecked(const KeyIdArgsList& key_ids,
                         const KeyTable& key_table) {
  if (!key_ids.empty()) {
    for (int i = 0; i < key_table.GetRowCount(); i++) {
      if (std::find(
              key_ids.begin(), key_ids.end(),
              key_table.GetKeyByIndex(key_table.model()->index(i, 0))->ID()) !=
          key_ids.end()) {
        key_table.SetRowChecked(i);
      }
    }
  }
}

[[maybe_unused]] auto KeyList::ContainsPrivateKeys() -> bool {
  auto* key_table = current_page();
  if (key_table == nullptr) return false;

  for (int i = 0; i < key_table->GetRowCount(); i++) {
    if (key_table->GetKeyByIndex(key_table->model()->index(i, 0)) != nullptr) {
      return true;
    }
  }
  return false;
}

void KeyList::contextMenuEvent(QContextMenuEvent* event) {
  auto* key_table = current_page();

  if (key_table == nullptr) {
    FLOG_D("m_key_list_ is nullptr, key stack page count: %d",
           ui_->keyStack->count());
    return;
  }

  if (key_table->GetRowSelected() >= 0) {
    emit SignalRequestContextMenu(event, key_table);
  }
}

void KeyList::dropEvent(QDropEvent* event) {
  if (!event->mimeData()->hasUrls() && !event->mimeData()->hasText()) {
    event->ignore();
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Import Keys"));

  auto* label = new QLabel(tr("You've dropped something on the key list.\n"
                              "GpgFrontend will now try to import key(s)."),
                           &dialog);

  auto* check_box =
      new QCheckBox(tr("Ask before importing keys next time."), &dialog);

  auto confirm_import_keys =
      GetSettings().value("basic/confirm_import_keys", true).toBool();
  check_box->setChecked(confirm_import_keys);

  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

  connect(button_box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto* vbox = new QVBoxLayout(&dialog);
  vbox->addWidget(label);
  vbox->addWidget(check_box);
  vbox->addWidget(button_box);

  if (confirm_import_keys) {
    if (dialog.exec() != QDialog::Accepted) {
      event->ignore();
      return;
    }

    auto settings = GetSettings();
    settings.setValue("basic/confirm_import_keys", check_box->isChecked());
  }

  QByteArray all_import_data;

  if (event->mimeData()->hasUrls()) {
    for (const QUrl& url : event->mimeData()->urls()) {
      if (!url.isLocalFile()) continue;

      QFile file(url.toLocalFile());
      if (!file.open(QIODevice::ReadOnly)) {
        LOG_W() << "couldn't open file:" << url.toString();
        continue;
      }

      all_import_data += file.readAll();
      all_import_data += '\n';
    }
  } else {
    all_import_data = event->mimeData()->text().toUtf8();
  }

  if (!all_import_data.isEmpty()) {
    import_keys(all_import_data);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent* event) {
  const auto* mime = event->mimeData();

  if (mime->hasUrls() || mime->hasText()) {
    event->acceptProposedAction();
    return;
  }

  event->ignore();
}

void KeyList::import_keys(const QByteArray& in_buffer) {
  LOG_D() << "importing keys to channel:" << current_gpg_context_channel_;
  auto result =
      KeyImportExportOperation::GetInstance(current_gpg_context_channel_)
          .ImportKey(GFBuffer(in_buffer));

  auto connection = QSharedPointer<QMetaObject::Connection>::create();
  *connection = connect(UISignalStation::GetInstance(),
                        &UISignalStation::SignalKeyDatabaseRefreshDone, this,
                        [this, result, connection]() {
                          auto* dialog = new KeyImportDetailDialog(
                              current_gpg_context_channel_, result, this);
                          dialog->show();

                          QObject::disconnect(*connection);
                        });

  emit SignalRefreshDatabase();
}

auto KeyList::GetSelectedKey() -> GpgAbstractKeyPtr {
  auto k = GetSelectedKeys();
  if (k.empty()) return nullptr;
  return k.front();
}

auto KeyList::GetSelectedGpgKey() -> GpgKeyPtr {
  auto k = GetSelectedGpgKeys();
  if (k.empty()) return nullptr;
  return k.front();
}

auto KeyList::GetSelectedGpgKeys() -> GpgKeyPtrList {
  auto keys = GetSelectedKeys();
  auto g_keys = GpgKeyPtrList{};
  for (const auto& key : keys) {
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) continue;
    g_keys.push_back(qSharedPointerDynamicCast<GpgKey>(key));
  }
  return g_keys;
}

void KeyList::sync_keys_from_key_server(
    const KeyIdArgsList& key_ids,
    const std::function<void(const QString&, const QString&, size_t, size_t)>&
        callback) const {
  // LOOP
  decltype(key_ids.size()) current_index = 1;
  decltype(key_ids.size()) all_index = key_ids.size();

  auto channel = current_gpg_context_channel_;

  for (const auto& key_id : key_ids) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr&) -> int {
              // rate limit
              QThread::msleep(200);
              // call
              Module::TriggerEvent(
                  "REQUEST_GET_PUBLIC_KEY_BY_KEY_ID",
                  {
                      {"key_id", GFBuffer{key_id}},
                  },
                  [key_id, channel, callback, current_index, all_index](
                      const Module::EventIdentifier&,
                      const Module::Event::ListenerIdentifier&,
                      Module::Event::Params p) {
                    QString status;

                    if (p["ret"] != "0" || !p["error_msg"].Empty()) {
                      LOG_E()
                          << "An error occurred trying to get data from key:"
                          << key_id << "error message: "
                          << p["error_msg"].ConvertToQString() << "reply data: "
                          << p["reply_data"].ConvertToQString();
                      status = p["error_msg"].ConvertToQString() +
                               p["reply_data"].ConvertToQString();
                    } else if (p.contains("key_data")) {
                      const auto key_data = p["key_data"];
                      LOG_D() << "got key data of key " << key_id
                              << " from key server: "
                              << key_data.ConvertToQString();

                      auto result =
                          KeyImportExportOperation::GetInstance(channel)
                              .ImportKey(GFBuffer(key_data));
                      if (result->imported == 1) {
                        status = tr("The key has been updated");
                      } else {
                        status = tr("No need to update the key");
                      }
                    }

                    callback(key_id, status, current_index, all_index);
                  });

              return 0;
            },
            QString("key_%1_import_task").arg(key_id)));

    current_index++;
  }
}

void KeyList::slot_sync_with_key_server() {
  auto keys = GetCheckedPublicKey();
  if (keys.empty()) {
    QMessageBox::StandardButton const reply = QMessageBox::question(
        this, QCoreApplication::tr("Sync All Public Key"),
        QCoreApplication::tr("You have not checked any public keys that you "
                             "want to synchronize, do you want to synchronize "
                             "all local public keys from the key server?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    keys = model_->GetAllKeys();
  }

  auto key_ids = ConvertKey2GpgKeyIdList(current_gpg_context_channel_, keys);
  if (key_ids.empty()) return;

  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  emit SignalRefreshStatusBar(tr("Syncing Key List..."), 3000);

  sync_keys_from_key_server(
      key_ids, [=](const QString& key_id, const QString& status,
                   size_t current_index, size_t all_index) {
        auto status_str = tr("Sync [%1/%2] %3 %4")
                              .arg(current_index)
                              .arg(all_index)
                              .arg(key_id)
                              .arg(status);
        emit SignalRefreshStatusBar(status_str, 1500);

        if (current_index == all_index) {
          ui_->syncButton->setDisabled(false);
          ui_->refreshKeyListButton->setDisabled(false);
          emit SignalRefreshStatusBar(tr("Key List Sync Done."), 3000);
          emit this->SignalRefreshDatabase();
        }
      });
}

void KeyList::filter_by_keyword() {
  auto keyword = ui_->searchBarEdit->text().trimmed().toLower();

  for (int i = 0; i < ui_->keyStack->count(); i++) {
    auto* key_table = qobject_cast<KeyTable*>(ui_->keyStack->widget(i));
    if (key_table == nullptr) continue;

    key_table->SetFilterKeyword(keyword);
  }

  SlotRefreshUI();
}

void KeyList::uncheck_all() {
  auto* key_table = current_page();
  if (key_table == nullptr) return;
  key_table->UncheckAll();
}

void KeyList::check_all() {
  auto* key_table = current_page();
  if (key_table == nullptr) return;
  key_table->CheckAll();
}

void KeyList::UpdateKeyTableColumnType(GpgKeyTableColumn column_type) {
  global_column_filter_ = column_type;

  const auto effective_fixed_columns = ApplyEngineColumnFilter(
      current_gpg_context_channel_, fixed_columns_filter_);

  emit SignalColumnTypeChange(effective_fixed_columns & global_column_filter_);
}

auto KeyList::GetCurrentGpgContextChannel() const -> int {
  return current_gpg_context_channel_;
}

auto KeyList::GetSelectedKeys() -> GpgAbstractKeyPtrList {
  auto* key_table = current_page();
  if (key_table == nullptr) return {};

  return key_table->GetSelectedKeys();
}

void KeyList::slot_new_key_group() {
  auto keys = GetCheckedKeys();

  QStringList proper_key_ids;
  for (const auto& key : keys) {
    if (!key->IsHasEncrCap()) continue;
    proper_key_ids.append(key->ID());
  }

  if (proper_key_ids.isEmpty()) return;

  auto* dialog =
      new KeyGroupCreationDialog(current_gpg_context_channel_, proper_key_ids);
  dialog->exec();
}

void KeyList::UpdateKeyTableFilter(
    int index, const GpgKeyTableProxyModel::KeyFilter& filter) {
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyStack->widget(index));
  if (key_table == nullptr) return;

  key_table->SetFilter(filter);
}

void KeyList::RefreshKeyTable(int index) {
  auto* key_table = qobject_cast<KeyTable*>(ui_->keyStack->widget(index));
  if (key_table == nullptr) return;

  key_table->RefreshProxyModel();
}

void KeyList::InitAfter(int channel, KeyMenuAbility menu_ability,
                        GpgKeyTableColumn fixed_columns_filter) {
  current_gpg_context_channel_ = channel;
  menu_ability_ = menu_ability;
  fixed_columns_filter_ = fixed_columns_filter;
  model_ = AbstractKeyRepository::GetInstance(channel).GetGpgKeyTableModel();

  init();
}

}  // namespace GpgFrontend::UI
