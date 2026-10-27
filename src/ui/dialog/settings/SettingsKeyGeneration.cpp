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

#include "ui/dialog/settings/SettingsKeyGeneration.h"

#include <random>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/dialog/key_generate/KeyGenProfileEditDialog.h"
#include "ui/dialog/key_generate/KeyGenerateSummary.h"
#include "ui/function/KeyGenSettings.h"
#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

enum ProfileColumn { kNameColumn, kStatusColumn };

/// The margins both tabs keep around their content, so switching between
/// them moves nothing.
constexpr int kTabMarginH = 8;
constexpr int kTabMarginV = 10;

auto MakeProblemLabel(QWidget* parent) -> QLabel* {
  auto* label = CreateDetailLabel({}, parent);
  SetLabelTextColor(label, WarningColor(label->palette()));
  label->hide();
  return label;
}

/// A value in the details area: wraps, and can be selected to copy.
auto MakeDetailValue(QWidget* parent) -> QLabel* {
  auto* label = new QLabel(parent);
  label->setWordWrap(true);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

auto MakeTabPage(QWidget* parent) -> std::pair<QWidget*, QVBoxLayout*> {
  auto* page = new QWidget(parent);
  auto* layout = new QVBoxLayout(page);
  layout->setContentsMargins(kTabMarginH, kTabMarginV, kTabMarginH,
                             kTabMarginV);
  layout->setSpacing(10);
  return {page, layout};
}

/// The placeholder reference: one line per placeholder, the placeholder
/// itself in a fixed-width font, so it reads as a table and not a paragraph.
auto MakePlaceholderReference(QWidget* parent) -> QWidget* {
  auto* reference = new QWidget(parent);
  auto* grid = new QGridLayout(reference);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setHorizontalSpacing(16);
  grid->setVerticalSpacing(3);
  grid->setColumnStretch(1, 1);

  const std::array<std::pair<QString, QString>, 10> rows{{
      {u"{token}"_s,
       QObject::tr("8 random letters and digits, the same in every field")},
      {u"{token:N}"_s, QObject::tr("the first N of them, from 4 to 32")},
      {u"{domain}"_s, QObject::tr("a random domain label")},
      {u"{tld}"_s, QObject::tr("test, example or invalid")},
      {u"{date}"_s, QObject::tr("today, as yyyyMMdd")},
      {u"{date:FORMAT}"_s,
       QObject::tr("the date and time in any Qt format, e.g. yyyy-MM-dd or "
                   "yyMMdd-HHmm")},
      {u"{time}"_s, QObject::tr("the time of day, as HHmmss")},
      {u"{timestamp}"_s, QObject::tr("seconds since 1970 (Unix time)")},
      {u"{timestamp:ms}"_s, QObject::tr("the same, in milliseconds")},
      {u"{{  }}"_s, QObject::tr("a literal brace")},
  }};
  const auto mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  int row = 0;
  for (const auto& [placeholder, meaning] : rows) {
    auto* code = new QLabel(placeholder, reference);
    code->setFont(mono);
    code->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* text = CreateDetailLabel(meaning, reference);
    grid->addWidget(code, row, 0, Qt::AlignLeft | Qt::AlignTop);
    grid->addWidget(text, row, 1);
    ++row;
  }
  return reference;
}

}  // namespace

KeyGenerationTab::KeyGenerationTab(QWidget* parent)
    : QWidget(parent),
      channel_(UsableDefaultChannelHandle()),
      settings_(QJsonObject{}),
      preview_seed_(QRandomGenerator::global()->generate64()),
      default_combo_(new QComboBox(this)),
      default_note_(CreateDetailLabel({}, this)),
      name_pattern_(new QLineEdit(this)),
      email_pattern_(new QLineEdit(this)),
      comment_pattern_(new QLineEdit(this)),
      name_problem_(MakeProblemLabel(this)),
      email_problem_(MakeProblemLabel(this)),
      comment_problem_(MakeProblemLabel(this)),
      preview_(new QLabel(this)),
      preview_note_(CreateDetailLabel({}, this)),
      profile_table_(new QTableWidget(this)),
      detail_title_(new QLabel(this)),
      detail_signing_(MakeDetailValue(this)),
      detail_encryption_(MakeDetailValue(this)),
      detail_validity_(MakeDetailValue(this)),
      detail_availability_(MakeDetailValue(this)),
      new_action_(new QAction(tr("&New…"), this)),
      edit_action_(new QAction(tr("&Edit…"), this)),
      duplicate_action_(new QAction(tr("D&uplicate"), this)),
      default_action_(new QAction(tr("Set as &Default"), this)),
      hide_action_(new QAction(tr("&Hide"), this)),
      delete_action_(new QAction(tr("De&lete"), this)),
      up_action_(new QAction(tr("Move Up"), this)),
      down_action_(new QAction(tr("Move Down"), this)) {
  // Two tabs, one per question: what new keys start from and what Quick
  // Generate calls them, and which key types there are.
  auto* tabs = new QTabWidget(this);
  tabs->setDocumentMode(true);
  tabs->addTab(build_quick_tab(), tr("Quick Generate"));
  tabs->addTab(build_profiles_tab(), tr("Profiles"));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(tabs);

  SetSettings();
}

auto KeyGenerationTab::build_quick_tab() -> QWidget* {
  // The tab scrolls on its own, so nothing shown or hidden in it -- help,
  // problems, the note -- ever asks the dialog for more room.
  auto* scroll = new QScrollArea(this);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto [page, layout] = MakeTabPage(scroll);
  scroll->setWidget(page);

  // One column for both sections, as wide as the tab, so they share both
  // edges with the tab's margins and grow and shrink with the dialog.
  auto* column = new QWidget(page);
  auto* column_layout = new QVBoxLayout(column);
  column_layout->setContentsMargins(0, 0, 0, 0);
  column_layout->setSpacing(12);
  layout->addWidget(column);
  layout->addStretch(1);

  // --- Default key profile ---
  auto* profile_box = new QGroupBox(tr("Default key profile"), column);
  auto* profile_layout = new QVBoxLayout(profile_box);
  default_combo_->setMinimumContentsLength(12);
  default_combo_->setSizeAdjustPolicy(
      QComboBox::AdjustToMinimumContentsLengthWithIcon);
  default_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  default_combo_->setAccessibleName(tr("Default key profile"));
  profile_layout->addWidget(default_combo_);
  profile_layout->addWidget(CreateDetailLabel(
      tr("Preselected in the key generation wizard and used by Quick "
         "Generate."),
      profile_box));
  profile_layout->addWidget(default_note_);
  column_layout->addWidget(profile_box);

  // --- Quick Generate identity ---
  // A grid rather than a QFormLayout: a form reserves the height a wrapped
  // line would need at a narrower width than it gets. A row whose only widget
  // is hidden takes no room, so a problem is shown by showing its label.
  auto* identity_box = new QGroupBox(tr("Quick Generate identity"), column);
  auto* grid = new QGridLayout(identity_box);
  grid->setHorizontalSpacing(12);
  grid->setVerticalSpacing(6);
  grid->setColumnStretch(1, 1);
  int line = 0;
  const auto add_row = [grid, identity_box, &line](
                           const QString& label, QWidget* field,
                           Qt::Alignment align = Qt::AlignVCenter) {
    if (!label.isEmpty()) {
      auto* caption = new QLabel(label, identity_box);
      caption->setBuddy(field);
      grid->addWidget(caption, line, 0, Qt::AlignLeft | align);
    }
    grid->addWidget(field, line, 1);
    ++line;
  };

  name_pattern_->setPlaceholderText(u"anon-{token}"_s);
  email_pattern_->setPlaceholderText(tr("optional"));
  comment_pattern_->setPlaceholderText(tr("optional"));
  for (auto* edit : {name_pattern_, email_pattern_, comment_pattern_}) {
    edit->setClearButtonEnabled(true);
  }
  add_row(tr("&Name pattern:"), name_pattern_);
  add_row({}, name_problem_);
  add_row(tr("&Email pattern:"), email_pattern_);
  add_row({}, email_problem_);
  add_row(tr("C&omment pattern:"), comment_pattern_);
  add_row({}, comment_problem_);

  // The preview right under the patterns it shows, the refresh beside it.
  auto* preview_cell = new QWidget(identity_box);
  auto* preview_row = new QHBoxLayout(preview_cell);
  preview_row->setContentsMargins(0, 0, 0, 0);
  preview_row->setSpacing(6);
  preview_->setWordWrap(true);
  preview_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  // An address cannot wrap, and must not make the page wider than the
  // window: past the edge it is cut, and the tooltip holds it whole.
  preview_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  auto* again = new QToolButton(preview_cell);
  again->setIcon(QIcon(u":/icons/refresh.png"_s));
  again->setAutoRaise(true);
  again->setToolTip(tr("Generate another example"));
  again->setAccessibleName(tr("Generate another example"));
  preview_row->addWidget(preview_, 1);
  preview_row->addWidget(again, 0, Qt::AlignTop);
  add_row(tr("Preview:"), preview_cell, Qt::AlignTop);
  add_row({}, preview_note_);

  // One secondary row: the reference toggle and the reset. The reference
  // opens beneath them, inside this tab's scroll area.
  auto* secondary = new QWidget(identity_box);
  auto* secondary_layout = new QVBoxLayout(secondary);
  secondary_layout->setContentsMargins(0, 4, 0, 0);
  secondary_layout->setSpacing(6);
  auto* help_toggle = new QToolButton(secondary);
  help_toggle->setText(tr("Placeholders"));
  help_toggle->setCheckable(true);
  help_toggle->setChecked(false);
  help_toggle->setAutoRaise(true);
  help_toggle->setArrowType(Qt::RightArrow);
  help_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  auto* reset_patterns =
      new QPushButton(tr("Restore Default Patterns"), secondary);
  reset_patterns->setAutoDefault(false);
  reset_patterns->setToolTip(
      tr("Puts the name, email and comment patterns back as they were. The "
         "default key profile and the profiles are not changed."));
  auto* secondary_row = new QHBoxLayout();
  secondary_row->setContentsMargins(0, 0, 0, 0);
  secondary_row->addWidget(help_toggle);
  secondary_row->addStretch(1);
  secondary_row->addWidget(reset_patterns);
  secondary_layout->addLayout(secondary_row);
  auto* reference = MakePlaceholderReference(secondary);
  reference->hide();
  secondary_layout->addWidget(reference);
  connect(help_toggle, &QToolButton::toggled, reference,
          [help_toggle, reference](bool open) {
            help_toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
            reference->setVisible(open);
          });
  add_row({}, secondary);

  column_layout->addWidget(identity_box);

  for (auto* edit : {name_pattern_, email_pattern_, comment_pattern_}) {
    connect(edit, &QLineEdit::textChanged, this,
            &KeyGenerationTab::refresh_pattern);
  }
  connect(again, &QToolButton::clicked, this, [this]() {
    preview_seed_ = QRandomGenerator::global()->generate64();
    refresh_pattern();
  });
  connect(reset_patterns, &QPushButton::clicked, this, [this]() {
    const UidPattern defaults;
    name_pattern_->setText(defaults.name);
    email_pattern_->setText(defaults.email);
    comment_pattern_->setText(defaults.comment);
  });
  connect(default_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    const auto name = default_combo_->currentData().toString();
    if (name.isEmpty()) return;
    settings_.default_profile = name;
    refresh_profiles(current_row());
  });
  return scroll;
}

auto KeyGenerationTab::build_profiles_tab() -> QWidget* {
  auto [page, layout] = MakeTabPage(this);

  // One set of actions for the toolbar, the menu, the keys and the list's
  // context menu, so what can be done is decided once.
  up_action_->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  down_action_->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
  up_action_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
  down_action_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
  edit_action_->setShortcut(QKeySequence(Qt::Key_F2));
  delete_action_->setShortcut(QKeySequence::Delete);
  for (auto* action :
       {new_action_, edit_action_, duplicate_action_, default_action_,
        hide_action_, delete_action_, up_action_, down_action_}) {
    // The keys work while the list has focus, and nowhere else in Settings.
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    profile_table_->addAction(action);
  }

  auto* more_menu = new QMenu(page);
  more_menu->setToolTipsVisible(true);
  more_menu->addAction(duplicate_action_);
  more_menu->addAction(default_action_);
  more_menu->addAction(hide_action_);
  more_menu->addSeparator();
  more_menu->addAction(delete_action_);

  // Compact buttons, each as wide as its text, on one line of steady height.
  const auto tool_button = [page](QAction* action) {
    auto* button = new QToolButton(page);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    return button;
  };
  auto* more_button = new QToolButton(page);
  more_button->setText(tr("More"));
  more_button->setToolTip(tr("More actions for the selected profile"));
  more_button->setMenu(more_menu);
  more_button->setPopupMode(QToolButton::InstantPopup);
  auto* up_button = new QToolButton(page);
  up_button->setDefaultAction(up_action_);
  up_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
  up_button->setAccessibleName(tr("Move Up"));
  auto* down_button = new QToolButton(page);
  down_button->setDefaultAction(down_action_);
  down_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
  down_button->setAccessibleName(tr("Move Down"));

  auto* toolbar = new QHBoxLayout();
  toolbar->setContentsMargins(0, 0, 0, 0);
  toolbar->setSpacing(6);
  toolbar->addWidget(tool_button(new_action_));
  toolbar->addWidget(tool_button(edit_action_));
  toolbar->addWidget(more_button);
  toolbar->addStretch(1);
  toolbar->addWidget(up_button);
  toolbar->addWidget(down_button);
  layout->addLayout(toolbar);

  // The list, in the order the wizard uses; never sorted by a header click,
  // which would show an order that is not the one set here. It takes the
  // tab's spare height.
  profile_table_->setColumnCount(2);
  profile_table_->setHorizontalHeaderLabels({tr("Profile"), tr("Status")});
  profile_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  profile_table_->setSelectionMode(QAbstractItemView::SingleSelection);
  profile_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  profile_table_->setSortingEnabled(false);
  profile_table_->setShowGrid(false);
  profile_table_->setAlternatingRowColors(true);
  profile_table_->setWordWrap(false);
  profile_table_->setTextElideMode(Qt::ElideRight);
  profile_table_->setTabKeyNavigation(false);
  profile_table_->verticalHeader()->hide();
  profile_table_->verticalHeader()->setDefaultSectionSize(
      profile_table_->fontMetrics().height() + 8);
  auto* header = profile_table_->horizontalHeader();
  header->setSectionsClickable(false);
  header->setHighlightSections(false);
  header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  header->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
  header->setSectionResizeMode(kStatusColumn, QHeaderView::ResizeToContents);
  profile_table_->setMinimumHeight(profile_table_->fontMetrics().height() * 6);
  // Dragged to reorder. The table only draws where the row would go; the drop
  // itself moves the profile in the list (see eventFilter()), because a
  // table's own internal move overwrites the row it lands on.
  profile_table_->setDragEnabled(true);
  profile_table_->setAcceptDrops(true);
  profile_table_->setDragDropMode(QAbstractItemView::InternalMove);
  profile_table_->setDefaultDropAction(Qt::MoveAction);
  profile_table_->setDragDropOverwriteMode(false);
  profile_table_->setDropIndicatorShown(true);
  profile_table_->viewport()->installEventFilter(this);
  profile_table_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget(profile_table_, 1);

  // The selected profile: a pane of a steady height that scrolls inside
  // itself, so neither a long name nor a long reason moves the list.
  auto title_font = detail_title_->font();
  title_font.setBold(true);
  detail_title_->setFont(title_font);
  detail_title_->setWordWrap(true);
  detail_title_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  auto* details = new QWidget();
  auto* details_layout = new QVBoxLayout(details);
  details_layout->setContentsMargins(10, 8, 10, 8);
  details_layout->setSpacing(6);
  details_layout->addWidget(detail_title_);
  auto* details_grid = new QGridLayout();
  details_grid->setHorizontalSpacing(12);
  details_grid->setVerticalSpacing(4);
  details_grid->setColumnStretch(1, 1);
  int line = 0;
  for (const auto& [caption, value] :
       std::array<std::pair<QString, QLabel*>, 4>{{
           {tr("Signing:"), detail_signing_},
           {tr("Encryption:"), detail_encryption_},
           {tr("Valid for:"), detail_validity_},
           {tr("Availability:"), detail_availability_},
       }}) {
    auto* label = new QLabel(caption, details);
    details_grid->addWidget(label, line, 0, Qt::AlignLeft | Qt::AlignTop);
    details_grid->addWidget(value, line, 1);
    ++line;
  }
  details_layout->addLayout(details_grid);
  details_layout->addStretch(1);

  auto* details_scroll = new QScrollArea(page);
  details_scroll->setWidgetResizable(true);
  details_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  details_scroll->setWidget(details);
  details_scroll->setAccessibleName(tr("Selected profile"));
  // Seven lines of the current font: enough for every field of a typical
  // profile, measured so it grows with the font, and fixed so selecting
  // another profile never changes it.
  details_scroll->setFixedHeight(details_scroll->fontMetrics().lineSpacing() *
                                     7 +
                                 details_layout->contentsMargins().top() +
                                 details_layout->contentsMargins().bottom());
  layout->addWidget(details_scroll);

  // Last and quiet: a repair, not an everyday action.
  auto* restore_button =
      new QPushButton(tr("Restore Built-in Profiles…"), page);
  restore_button->setAutoDefault(false);
  restore_button->setToolTip(
      tr("Adds back the profiles GpgFrontend comes with that are missing. "
         "Your own profiles, and changes to the built-in ones, are kept."));
  auto* restore_row = new QHBoxLayout();
  restore_row->setContentsMargins(0, 0, 0, 0);
  restore_row->addWidget(restore_button);
  restore_row->addStretch(1);
  layout->addLayout(restore_row);

  // The same actions on a right click.
  profile_table_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(profile_table_, &QWidget::customContextMenuRequested, this,
          [this](const QPoint& pos) {
            QMenu menu(profile_table_);
            menu.setToolTipsVisible(true);
            menu.addActions({new_action_, edit_action_, duplicate_action_,
                             default_action_, hide_action_});
            menu.addSeparator();
            menu.addActions({up_action_, down_action_});
            menu.addSeparator();
            menu.addAction(delete_action_);
            menu.exec(profile_table_->viewport()->mapToGlobal(pos));
          });

  connect(profile_table_, &QTableWidget::currentCellChanged, this,
          [this]() { refresh_profile_detail(); });
  connect(profile_table_, &QTableWidget::cellDoubleClicked, this, [this]() {
    if (edit_action_->isEnabled()) edit_action_->trigger();
  });
  connect(new_action_, &QAction::triggered, this, &KeyGenerationTab::slot_new);
  connect(edit_action_, &QAction::triggered, this,
          &KeyGenerationTab::slot_edit);
  connect(duplicate_action_, &QAction::triggered, this,
          &KeyGenerationTab::slot_duplicate);
  connect(delete_action_, &QAction::triggered, this,
          &KeyGenerationTab::slot_delete);
  connect(default_action_, &QAction::triggered, this,
          &KeyGenerationTab::slot_set_default);
  connect(hide_action_, &QAction::triggered, this,
          &KeyGenerationTab::slot_toggle_hidden);
  connect(up_action_, &QAction::triggered, this, [this]() { slot_move(-1); });
  connect(down_action_, &QAction::triggered, this, [this]() { slot_move(1); });
  connect(restore_button, &QPushButton::clicked, this,
          &KeyGenerationTab::slot_restore_defaults);
  return page;
}

auto KeyGenerationTab::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (watched != profile_table_->viewport() || event->type() != QEvent::Drop) {
    return QWidget::eventFilter(watched, event);
  }

  auto* drop = static_cast<QDropEvent*>(event);
  const auto from = current_row();
  if (drop->source() != profile_table_ || from < 0) return false;

  // Above or below the row under the pointer, by which half it is in; past
  // the last row, to the end.
  const auto y = drop->position().toPoint().y();
  const auto over = profile_table_->rowAt(y);
  auto target = static_cast<int>(settings_.profiles.size());
  if (over >= 0) {
    const auto rect = profile_table_->visualRect(
        profile_table_->model()->index(over, kNameColumn));
    target = y > rect.center().y() ? over + 1 : over;
  }
  const auto to = target > from ? target - 1 : target;

  // Handled here, so the table does not move or overwrite anything itself.
  drop->setDropAction(Qt::IgnoreAction);
  drop->accept();
  if (to != from) {
    settings_.profiles.move(from, to);
    refresh_profiles(to);
  }
  return true;
}

void KeyGenerationTab::SetSettings() {
  settings_ = LoadKeyGenSettings();

  for (auto* edit : {name_pattern_, email_pattern_, comment_pattern_}) {
    edit->blockSignals(true);
  }
  name_pattern_->setText(settings_.uid_pattern.name);
  email_pattern_->setText(settings_.uid_pattern.email);
  comment_pattern_->setText(settings_.uid_pattern.comment);
  for (auto* edit : {name_pattern_, email_pattern_, comment_pattern_}) {
    edit->blockSignals(false);
  }

  refresh_profiles(0);
  refresh_pattern();
}

void KeyGenerationTab::ApplySettings() {
  settings_.uid_pattern = current_pattern();
  StoreKeyGenSettings(settings_);
}

auto KeyGenerationTab::current_pattern() const -> UidPattern {
  return {name_pattern_->text(), email_pattern_->text(),
          comment_pattern_->text()};
}

auto KeyGenerationTab::current_row() const -> int {
  const auto row = profile_table_->currentRow();
  return row >= 0 && row < settings_.profiles.size() ? row : -1;
}

auto KeyGenerationTab::resolve(const KeyGenProfile& profile) const
    -> std::optional<ResolvedKeyGenProfile> {
  return ResolveKeyGenProfile(
      profile, KeyGenerateInfo::GetSupportedKeyAlgo(channel_),
      KeyGenerateInfo::GetSupportedSubkeyAlgo(channel_),
      [this](const KeyAlgo& a) { return a.SubAlgos(channel_); });
}

auto KeyGenerationTab::facts(const KeyGenProfile& profile) const
    -> KeyGenProfileFacts {
  return DescribeKeyGenProfileFacts(
      profile, KeyGenerateInfo::GetSupportedKeyAlgo(channel_),
      KeyGenerateInfo::GetSupportedSubkeyAlgo(channel_),
      [this](const KeyAlgo& a) { return a.SubAlgos(channel_); });
}

void KeyGenerationTab::refresh_profiles(int select) {
  const QSignalBlocker blocker(profile_table_);

  const auto offered = OfferedKeyGenProfiles(channel_, settings_.profiles);
  const auto default_index =
      PickDefaultKeyGenProfile(offered, settings_.default_profile);
  default_name_ =
      default_index >= 0 ? offered.at(default_index).profile.name : QString();

  const auto muted = MutedTextColor(profile_table_->palette());
  profile_table_->setRowCount(static_cast<int>(settings_.profiles.size()));
  for (int row = 0; row < settings_.profiles.size(); ++row) {
    const auto& profile = settings_.profiles.at(row);
    const auto unavailable = !facts(profile).missing.isEmpty();

    // Every state in words, not only by a grey row; what the user set
    // (hidden) apart from what the engine allows (unavailable).
    QStringList status;
    if (is_default_profile(row)) status << tr("Default");
    if (profile.hidden) status << tr("Hidden");
    if (unavailable) status << tr("Unavailable");
    if (status.isEmpty()) status << tr("Offered");

    auto* name = new QTableWidgetItem(KeyGenProfileDisplayName(profile));
    name->setToolTip(KeyGenProfileDisplayName(profile));
    auto* state = new QTableWidgetItem(status.join(u", "_s));
    for (auto* item : {name, state}) {
      if (profile.hidden || unavailable) item->setForeground(muted);
    }
    if (is_default_profile(row)) {
      auto font = name->font();
      font.setBold(true);
      name->setFont(font);
    }
    profile_table_->setItem(row, kNameColumn, name);
    profile_table_->setItem(row, kStatusColumn, state);
  }

  if (!settings_.profiles.isEmpty()) {
    profile_table_->setCurrentCell(
        std::clamp(select, 0, static_cast<int>(settings_.profiles.size()) - 1),
        kNameColumn);
  }
  refresh_profile_detail();
  refresh_default_combo();
}

void KeyGenerationTab::refresh_default_combo() {
  const QSignalBlocker blocker(default_combo_);
  default_combo_->clear();

  const auto offered = OfferedKeyGenProfiles(channel_, settings_.profiles);
  for (const auto& r : offered) {
    default_combo_->addItem(KeyGenProfileDisplayName(r.profile),
                            r.profile.name);
  }

  const auto index =
      PickDefaultKeyGenProfile(offered, settings_.default_profile);
  default_combo_->setCurrentIndex(index);

  // Said when the stored choice is not what is used here.
  const auto named = settings_.default_profile;
  const auto missing =
      !named.isEmpty() && (index < 0 || offered.at(index).profile.name.compare(
                                            named, Qt::CaseInsensitive) != 0);
  default_note_->setText(
      missing ? tr("\"%1\" cannot be made in the current key database or is "
                   "hidden, so Quick Generate will ask before using another.")
                    .arg(named)
              : QString());
  SetLabelTextColor(default_note_, WarningColor(default_note_->palette()));
  default_note_->setVisible(missing);
}

void KeyGenerationTab::refresh_profile_detail() {
  const auto row = current_row();
  const auto valid = row >= 0;
  // Each action's tooltip says why it is off, when it is; reset here and set
  // below for this profile.
  for (auto* action : {edit_action_, duplicate_action_, delete_action_,
                       default_action_, hide_action_}) {
    action->setEnabled(valid);
    action->setToolTip(action->text().remove(u'&'));
  }
  up_action_->setEnabled(valid && row > 0);
  down_action_->setEnabled(valid && row + 1 < settings_.profiles.size());
  up_action_->setToolTip(tr("Move Up (Alt+Up)"));
  down_action_->setToolTip(tr("Move Down (Alt+Down)"));
  if (!valid) {
    for (auto* label : {detail_title_, detail_signing_, detail_encryption_,
                        detail_validity_, detail_availability_}) {
      label->clear();
    }
    return;
  }

  const auto& profile = settings_.profiles.at(row);
  const auto f = facts(profile);
  const auto unavailable = !f.missing.isEmpty();
  const auto is_default = is_default_profile(row);

  detail_title_->setText(KeyGenProfileDisplayName(profile));
  detail_title_->setToolTip(KeyGenProfileDisplayDescription(profile));
  detail_signing_->setText(f.signing);
  detail_encryption_->setText(f.encryption);
  detail_validity_->setText(f.validity);

  // One sentence: what the user set, then what the engine allows, each said
  // once.
  QString availability;
  if (unavailable) {
    auto& ctx = OpenPGPContext::GetInstance(channel_);
    const auto reason = tr("%1 %2 cannot generate %3.")
                            .arg(ConvertOpenPGPEngine2String(ctx.Engine()),
                                 ctx.EngineVersion(), f.missing.join(u", "_s));
    availability =
        profile.hidden
            ? tr("Hidden by you, and not available here: %1").arg(reason)
            : tr("Not available here: %1").arg(reason);
  } else if (profile.hidden) {
    availability = tr("Hidden by you; it could be made here.");
  } else {
    availability = tr("Offered in the wizard under Advanced options.");
    const auto offered = OfferedKeyGenProfiles(channel_, settings_.profiles);
    for (const auto& tile : FeaturedKeyGenProfiles(offered)) {
      if (offered.at(tile.index).profile.name == profile.name) {
        availability = tr("Offered in the wizard as \"%1\".")
                           .arg(KeyGenTileTitle(tile.role));
      }
    }
    if (is_default) {
      availability = tr("The default. %1").arg(availability);
    }
  }
  detail_availability_->setText(availability);

  // What each action can do with this one, and when it cannot, why.
  hide_action_->setText(profile.hidden ? tr("&Show") : tr("&Hide"));
  hide_action_->setToolTip(hide_action_->text().remove(u'&'));

  // The default stays: deleting or hiding it would leave the wizard and Quick
  // Generate pointing at nothing. Choosing another default first frees it.
  const auto default_reason =
      tr("This is the default profile. Choose another default first.");
  if (is_default) {
    delete_action_->setEnabled(false);
    delete_action_->setToolTip(default_reason);
    if (!profile.hidden) {
      hide_action_->setEnabled(false);
      hide_action_->setToolTip(default_reason);
    }
  }

  // Edited with the current engine's algorithms, which this one lacks;
  // duplicating, ordering, hiding and deleting it still work.
  if (unavailable) {
    edit_action_->setEnabled(false);
    edit_action_->setToolTip(
        tr("The current key database cannot make this profile, so it cannot "
           "be edited here."));
  }

  default_action_->setEnabled(!is_default && !unavailable && !profile.hidden);
  if (is_default) {
    default_action_->setToolTip(tr("This is already the default."));
  } else if (unavailable || profile.hidden) {
    default_action_->setToolTip(
        tr("Only a profile the wizard offers can be the default."));
  }
}

void KeyGenerationTab::refresh_pattern() {
  const auto pattern = current_pattern();
  const auto check = CheckUidPattern(pattern);
  const auto show = [this](QLabel* label, UidPatternProblem problem) {
    label->setText(DescribeUidPatternProblem(problem));
    label->setVisible(problem != UidPatternProblem::kNone);
  };
  show(name_problem_, check.name);
  show(email_problem_, check.email);
  show(comment_problem_, check.comment);

  // Made-up addresses end in a reserved domain; said only when they do.
  const auto reserved = pattern.email.contains(u"{tld}"_s);
  preview_note_->setText(
      reserved ? tr("Addresses made with {tld} end in a reserved domain "
                    "(.test, .example or .invalid): no mail can reach them.")
               : QString());
  preview_note_->setVisible(reserved);

  if (!check.Ok()) {
    preview_->setText(tr("No example: a pattern above has a problem."));
    SetLabelTextColor(preview_, MutedTextColor(preview_->palette()));
    return;
  }

  // The same draw until "Generate Example": typing in a pattern changes the
  // example where it changes the pattern, not everywhere at once.
  auto engine = std::make_shared<std::mt19937_64>(preview_seed_);
  const auto uid = ExpandUidPattern(
      pattern, [engine]() { return (*engine)(); }, CurrentDateTime());
  KeyGenerateInfo info;
  info.SetName(uid.name);
  info.SetEmail(uid.email);
  info.SetComment(uid.comment);
  // The address on a line of its own: it has no space to wrap at, and on
  // one line with the name it would run under the button. The user ID as it
  // will be written is the tooltip.
  const auto uid_text = info.GetUserid();
  const auto at = uid_text.lastIndexOf(u" <"_s);
  preview_->setText(at > 0 ? uid_text.left(at) + u'\n' + uid_text.mid(at + 1)
                           : uid_text);
  preview_->setToolTip(uid_text);
  SetLabelTextColor(preview_, palette().color(QPalette::WindowText));
}

auto KeyGenerationTab::edit(const ResolvedKeyGenProfile& start,
                            int ignore_index) -> std::optional<KeyGenProfile> {
  auto others = settings_.profiles;
  if (ignore_index >= 0) others.removeAt(ignore_index);

  KeyGenProfileEditDialog dialog(channel_, start, others, this);
  if (dialog.exec() != QDialog::Accepted) return std::nullopt;
  return dialog.Profile();
}

void KeyGenerationTab::slot_new() {
  // Starts from what Quick Generate would make, the most likely base.
  const auto offered = OfferedKeyGenProfiles(channel_, settings_.profiles);
  auto index = PickDefaultKeyGenProfile(offered, settings_.default_profile);
  std::optional<ResolvedKeyGenProfile> start;
  if (index >= 0) {
    start = offered.at(index);
  } else {
    start = resolve(MakeDefaultKeyGenProfiles().front());
  }
  if (!start) return;
  start->profile.name.clear();
  start->profile.description.clear();
  start->profile.hidden = false;

  if (auto profile = edit(*start, -1)) {
    settings_.profiles.append(*profile);
    refresh_profiles(static_cast<int>(settings_.profiles.size() - 1));
  }
}

void KeyGenerationTab::slot_edit() {
  const auto row = current_row();
  if (row < 0 || row >= settings_.profiles.size()) return;
  const auto r = resolve(settings_.profiles.at(row));
  if (!r) return;

  const auto old_name = settings_.profiles.at(row).name;
  if (auto profile = edit(*r, row)) {
    settings_.profiles[row] = *profile;
    // A renamed default stays the default.
    if (settings_.default_profile.compare(old_name, Qt::CaseInsensitive) == 0) {
      settings_.default_profile = profile->name;
    }
    refresh_profiles(row);
  }
}

void KeyGenerationTab::slot_duplicate() {
  const auto row = current_row();
  if (row < 0 || row >= settings_.profiles.size()) return;

  auto copy = settings_.profiles.at(row);
  copy.hidden = false;
  // "Name (2)", "Name (3)", ... up to the length a name may have.
  const auto base = KeyGenProfileDisplayName(copy);
  for (int n = 2;; ++n) {
    const auto suffix = u" (%1)"_s.arg(n);
    copy.name = base.left(kKeyGenProfileNameMaxLen - suffix.size()) + suffix;
    if (CheckKeyGenProfileName(copy.name, settings_.profiles) ==
        KeyGenProfileNameProblem::kNone) {
      break;
    }
  }
  copy.description = KeyGenProfileDisplayDescription(copy);
  settings_.profiles.insert(row + 1, copy);
  refresh_profiles(row + 1);
}

void KeyGenerationTab::slot_set_default() {
  const auto row = current_row();
  if (row < 0) return;
  // The same value the Quick Generate tab shows; it follows at once.
  settings_.default_profile = settings_.profiles.at(row).name;
  refresh_profiles(row);
}

auto KeyGenerationTab::is_default_profile(int row) const -> bool {
  return row >= 0 && row < settings_.profiles.size() &&
         !default_name_.isEmpty() &&
         settings_.profiles.at(row).name.compare(default_name_,
                                                 Qt::CaseInsensitive) == 0;
}

void KeyGenerationTab::slot_delete() {
  const auto row = current_row();
  if (row < 0 || row >= settings_.profiles.size()) return;
  if (is_default_profile(row)) return;

  const auto name = KeyGenProfileDisplayName(settings_.profiles.at(row));
  if (QMessageBox::question(
          this, tr("Delete Profile"),
          tr("Delete the profile \"%1\"? Keys made with it are not "
             "affected.")
              .arg(name)) != QMessageBox::Yes) {
    return;
  }
  settings_.profiles.removeAt(row);
  refresh_profiles(row);
}

void KeyGenerationTab::slot_toggle_hidden() {
  const auto row = current_row();
  if (row < 0 || row >= settings_.profiles.size()) return;
  if (is_default_profile(row) && !settings_.profiles.at(row).hidden) return;
  settings_.profiles[row].hidden = !settings_.profiles.at(row).hidden;
  refresh_profiles(row);
}

void KeyGenerationTab::slot_move(int delta) {
  const auto row = current_row();
  const auto to = row + delta;
  if (row < 0 || to < 0 || to >= settings_.profiles.size()) return;
  settings_.profiles.move(row, to);
  refresh_profiles(to);
}

void KeyGenerationTab::slot_restore_defaults() {
  // Adds back what is missing and nothing else: matched by what a profile
  // makes, so a renamed or reworded built-in is not added a second time.
  const auto restored = RestoreDefaultKeyGenProfiles(settings_.profiles);
  const auto added = restored.size() - settings_.profiles.size();
  if (added == 0) {
    QMessageBox::information(
        this, tr("Restore Defaults"),
        tr("Every profile GpgFrontend comes with is already in the list."));
    return;
  }
  if (QMessageBox::question(
          this, tr("Restore Defaults"),
          (added == 1
               ? tr("Add back 1 missing built-in profile? Your own profiles "
                    "stay as they are.")
               : tr("Add back %1 missing built-in profiles? Your own "
                    "profiles stay as they are.")
                     .arg(added))) != QMessageBox::Yes) {
    return;
  }
  const auto first_added = static_cast<int>(settings_.profiles.size());
  settings_.profiles = restored;
  refresh_profiles(first_added);
}

}  // namespace GpgFrontend::UI
