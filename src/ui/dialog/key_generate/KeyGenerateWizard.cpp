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

#include "ui/dialog/key_generate/KeyGenerateWizard.h"

#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/function/KeyDatabaseDisplayNames.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/KeyGenSettings.h"
#include "ui/function/KeyGenerateHelper.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/ChoiceTile.h"
#include "ui/widgets/ExpiryPicker.h"
#include "ui/widgets/KeyAlgoEditor.h"
#include "ui/widgets/MetaListPanel.h"
#include "ui/widgets/ToggleSwitch.h"

namespace GpgFrontend::UI {

namespace {

auto StepIndex(KeyGenerateStep step) -> int { return static_cast<int>(step); }

/// A line under a field that says what is wrong with it, or nothing.
auto MakeProblemLabel(QWidget* parent) -> QLabel* {
  auto* label = CreateDetailLabel({}, parent);
  SetLabelTextColor(label, WarningColor(label->palette()));
  label->hide();
  return label;
}

void ShowProblem(QLabel* label, const QString& problem) {
  label->setText(problem);
  label->setVisible(!problem.isEmpty());
}

auto BoldLabel(const QString& text, QWidget* parent) -> QLabel* {
  auto* label = new QLabel(text, parent);
  auto font = label->font();
  font.setBold(true);
  label->setFont(font);
  return label;
}

}  // namespace

KeyGenerateWizard::KeyGenerateWizard(int channel, QWidget* parent)
    : GeneralWizard(typeid(KeyGenerateWizard).name(), tr("Generate"), parent),
      channel_(channel),
      expiry_supported_(IsOpSupported<SetExpireOpTag>(channel)),
      format_choice_(OpenPGPContext::GetInstance(channel).Engine() ==
                     OpenPGPEngine::kRPGP),
      primary_algos_(KeyGenerateInfo::GetSupportedKeyAlgo(channel)),
      subkey_algos_(KeyGenerateInfo::GetSupportedSubkeyAlgo(channel)),
      settings_(LoadKeyGenSettings()),
      key_info_(SecureCreateSharedObject<KeyGenerateInfo>()) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return channel_; },
      [this](int channel) { channel_ = channel; });

  setWindowTitle(tr("Generate Key"));
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);

  offered_ = OfferedKeyGenProfiles(channel_, settings_.profiles);
  default_profile_ =
      PickDefaultKeyGenProfile(offered_, settings_.default_profile);

  build_identity_page();
  build_key_type_page();
  build_key_details_page();
  build_protection_page();
  build_review_page();
  setup_expert_switch();

  connect(this, &QWizard::currentIdChanged, this,
          &KeyGenerateWizard::slot_page_changed);

  // The default profile is what a person who reads nothing gets.
  if (default_profile_ >= 0) {
    apply_profile(default_profile_);
  } else {
    primary_editor_->Refresh();
    mark_custom();
    expert_switch_->setChecked(true);
  }
  sync_passphrase();
  refresh_identity_problems();

  show();
  raise();
  activateWindow();
  name_edit_->setFocus();
}

auto KeyGenerateWizard::add_page(KeyGenerateStep step) -> GeneralWizardPage* {
  auto* page = AddStep(KeyGenerateStepNumber(step), kKeyGenerateStepCount,
                       KeyGenerateStepTitle(step), StepIndex(step));
  page->complete = [this, step]() {
    return CanAdvanceKeyGenerate(step, state());
  };
  pages_.at(StepIndex(step)) = page;
  return page;
}

void KeyGenerateWizard::build_identity_page() {
  auto* page = add_page(KeyGenerateStep::kIdentity);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Who is this key for? Others see this name and address when they "
             "look up your key."),
          page);

  // Named for the manual's screenshots, which point at these.
  name_edit_ = new QLineEdit(page);
  name_edit_->setObjectName(u"nameEdit"_s);
  name_edit_->setPlaceholderText(tr("for example: Alex Doe"));
  email_edit_ = new QLineEdit(page);
  email_edit_->setObjectName(u"emailEdit"_s);
  email_edit_->setPlaceholderText(tr("optional: alex@example.com"));
  comment_edit_ = new QLineEdit(page);
  comment_edit_->setObjectName(u"commentEdit"_s);
  comment_edit_->setPlaceholderText(tr("optional: Work"));

  name_problem_ = MakeProblemLabel(page);
  email_problem_ = MakeProblemLabel(page);
  comment_problem_ = MakeProblemLabel(page);

  const auto field = [page](QLineEdit* edit, QLabel* problem) {
    auto* cell = new QWidget(page);
    auto* cell_layout = new QVBoxLayout(cell);
    cell_layout->setContentsMargins(0, 0, 0, 0);
    cell_layout->setSpacing(2);
    edit->setClearButtonEnabled(true);
    cell_layout->addWidget(edit);
    cell_layout->addWidget(problem);
    return cell;
  };

  auto* form = new QFormLayout();
  form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  form->addRow(tr("Name"), field(name_edit_, name_problem_));
  form->addRow(tr("Email"), field(email_edit_, email_problem_));
  form->addRow(tr("Comment"), field(comment_edit_, comment_problem_));
  layout->addLayout(form);

  // For a key nobody needs to recognise: a test, a throwaway, a pseudonym.
  auto* pattern_row = new QHBoxLayout();
  auto* pattern_button = new QPushButton(QIcon(u":/icons/refresh.png"_s),
                                         tr("Fill In Anonymously"), page);
  pattern_button->setAutoDefault(false);
  pattern_row->addWidget(pattern_button);
  pattern_row->addWidget(
      CreateDetailLabel(tr("Makes up a name and an address that reach "
                           "nobody, from the pattern in Settings."),
                        page),
      1);
  layout->addSpacing(4);
  layout->addLayout(pattern_row);

  connect(pattern_button, &QPushButton::clicked, this,
          &KeyGenerateWizard::fill_from_pattern);

  for (auto* edit : {name_edit_, email_edit_, comment_edit_}) {
    connect(edit, &QLineEdit::textChanged, this, [this]() {
      refresh_identity_problems();
      refresh_complete();
    });
  }

  // Where the key goes, said once: it is decided by where the wizard was
  // opened from, not here.
  QString database;
  for (const auto& key_db : GetGpgKeyDatabaseInfos()) {
    if (key_db.channel != channel_) continue;
    database = KeyDatabaseChoiceLabel(
        key_db.id, key_db.name,
        ConvertOpenPGPEngine2String(
            OpenPGPContext::GetInstance(key_db.channel).Engine()),
        false);
  }
  layout->addStretch(1);
  if (!database.isEmpty()) {
    layout->addWidget(CreateDetailLabel(
        tr("The key is saved in the key database \"%1\".").arg(database),
        page));
  }

  page->validate = [this]() {
    // Passing through again only to swap the second step: already asked.
    if (switching_mode_) return true;
    return ConfirmShortUserIdName(this, name_edit_->text().trimmed());
  };
}

void KeyGenerateWizard::build_key_type_page() {
  auto* page = add_page(KeyGenerateStep::kKeyType);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Choose what kind of key to make. If you are unsure, keep the "
             "recommended one."),
          page);

  profile_group_ = new QButtonGroup(page);

  // One tile per purpose: a short title for what it is for, a sentence on
  // what to expect, and the algorithms last and quietly, for whoever needs
  // them.
  QList<int> featured;
  for (const auto& f : FeaturedKeyGenProfiles(offered_)) {
    const auto& r = offered_.at(f.index);
    auto* tile = new ChoiceTile(
        KeyGenTileTitle(f.role), KeyGenTileDescription(f.role),
        f.role == KeyGenTileRole::kModern ? tr("Recommended") : QString(),
        page);
    tile->SetSecondary(DescribeKeyGenProfileAlgos(r));
    profile_group_->addButton(tile->Radio(), f.index);
    layout->addWidget(tile);
    featured.append(f.index);
  }

  // Every other profile, behind Advanced options: there, not in the way.
  auto* advanced = new QWidget(page);
  auto* advanced_layout = new QVBoxLayout(advanced);
  advanced_layout->setContentsMargins(0, 0, 0, 0);
  more_combo_ = new QComboBox(advanced);
  more_combo_->setMinimumContentsLength(20);
  more_combo_->setSizeAdjustPolicy(
      QComboBox::AdjustToMinimumContentsLengthWithIcon);
  more_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  more_combo_->addItem(tr("Choose…"), -1);
  const auto add_family = [this, &featured](bool post_quantum, bool ecc,
                                            const QString& header) {
    bool first = true;
    for (int i = 0; i < offered_.size(); ++i) {
      const auto& r = offered_.at(i);
      if (featured.contains(i)) continue;
      if (r.post_quantum != post_quantum || r.ecc != ecc) continue;
      if (first && !header.isEmpty()) {
        AddComboSectionHeader(more_combo_, header);
      }
      first = false;
      more_combo_->addItem(KeyGenProfileDisplayName(r.profile), i);
      more_combo_->setItemData(more_combo_->count() - 1,
                               DescribeKeyGenProfileAlgos(r), Qt::ToolTipRole);
    }
  };
  add_family(false, false, {});
  add_family(false, true, tr("ECC"));
  add_family(true, false, tr("Post-Quantum"));
  more_combo_->setEnabled(more_combo_->count() > 1);

  auto* more_form = new QFormLayout();
  more_form->setContentsMargins(0, 0, 0, 0);
  more_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  more_form->addRow(tr("Other profile"), more_combo_);
  advanced_layout->addLayout(more_form);
  advanced_layout->addWidget(CreateDetailLabel(
      tr("Profiles are kept in Settings, under Key Generation. For every "
         "choice by hand, turn on Expert Mode."),
      advanced));

  auto* disclosure = CreateDisclosure(tr("Advanced options"), advanced, page);
  options_toggle_ = disclosure->findChild<QToolButton*>();
  connect(options_toggle_, &QToolButton::toggled, this,
          [this]() { RefitPages(); });
  layout->addSpacing(2);
  layout->addWidget(disclosure);
  layout->addStretch(1);

  connect(profile_group_, &QButtonGroup::idToggled, this,
          [this](int id, bool checked) {
            if (checked) apply_profile(id);
          });
  connect(more_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    const auto index = more_combo_->currentData().toInt();
    if (index >= 0) apply_profile(index);
  });
}

void KeyGenerateWizard::build_key_details_page() {
  auto* page = add_page(KeyGenerateStep::kKeyDetails);
  auto* outer = PageLayout(page);

  // Taller than any other step once a subkey is added; it scrolls as a whole
  // rather than stretching the wizard.
  auto* scroll = new QScrollArea(page);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto* body = new QWidget(scroll);
  scroll->setWidget(body);
  outer->addWidget(scroll);

  auto* layout = new QVBoxLayout(body);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  AddLead(layout,
          tr("Choose every algorithm, key length, usage and expiry yourself, "
             "or start from a profile and change what you need."),
          body);

  // A profile as a starting point, so an expert rarely starts from nothing.
  start_combo_ = new QComboBox(body);
  start_combo_->setMinimumContentsLength(24);
  start_combo_->setSizeAdjustPolicy(
      QComboBox::AdjustToMinimumContentsLengthWithIcon);
  start_combo_->addItem(tr("Custom"), -1);
  for (int i = 0; i < offered_.size(); ++i) {
    start_combo_->addItem(KeyGenProfileDisplayName(offered_.at(i).profile), i);
    start_combo_->setItemData(start_combo_->count() - 1,
                              DescribeKeyGenProfileAlgos(offered_.at(i)),
                              Qt::ToolTipRole);
  }
  auto* start_form = new QFormLayout();
  start_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  start_form->addRow(tr("Start from"), start_combo_);
  layout->addLayout(start_form);

  const auto section = [body]() {
    auto* rule = new QFrame(body);
    rule->setFrameShape(QFrame::HLine);
    rule->setFrameShadow(QFrame::Sunken);
    return rule;
  };

  layout->addWidget(section());
  layout->addWidget(BoldLabel(tr("Primary key"), body));
  primary_editor_ = new KeyAlgoEditor(channel_, primary_algos_, body);
  primary_editor_->SetExpiryVisible(expiry_supported_);
  primary_editor_->SetInfo(key_info_);
  layout->addWidget(primary_editor_);

  format_row_ = new QWidget(body);
  auto* format_layout = new QFormLayout(format_row_);
  format_layout->setContentsMargins(0, 0, 0, 0);
  format_combo_ = new QComboBox(format_row_);
  // v4 stays the interoperable default; v6 (RFC 9580) is opt-in because much
  // of the ecosystem still reads it poorly.
  format_combo_->addItem(tr("v4 (Compatible)"), 4);
  format_combo_->addItem(tr("v6 (Modern)"), 6);
  format_layout->addRow(tr("Key format"), format_combo_);
  format_row_->setVisible(format_choice_);
  layout->addWidget(format_row_);

  layout->addWidget(section());
  subkey_check_ = new QCheckBox(tr("Add a subkey"), body);
  auto subkey_font = subkey_check_->font();
  subkey_font.setBold(true);
  subkey_check_->setFont(subkey_font);
  layout->addWidget(subkey_check_);
  layout->addWidget(CreateDetailLabel(
      tr("A separate key under the primary one, usually for encryption, so it "
         "can be replaced without changing your identity."),
      body));
  subkey_editor_ = new KeyAlgoEditor(channel_, subkey_algos_, body);
  subkey_editor_->SetExpiryVisible(expiry_supported_);
  subkey_editor_->hide();
  layout->addWidget(subkey_editor_);

  auto* save_row = new QHBoxLayout();
  save_row->addStretch(1);
  auto* save_button = new QPushButton(tr("Save as Profile…"), body);
  save_button->setAutoDefault(false);
  save_row->addWidget(save_button);
  layout->addLayout(save_row);
  layout->addStretch(1);

  connect(start_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    const auto index = start_combo_->currentData().toInt();
    if (index >= 0) apply_profile(index);
  });
  connect(primary_editor_, &KeyAlgoEditor::SignalChanged, this,
          &KeyGenerateWizard::mark_custom);
  connect(subkey_editor_, &KeyAlgoEditor::SignalChanged, this,
          &KeyGenerateWizard::mark_custom);
  connect(subkey_check_, &QCheckBox::toggled, this, [this](bool on) {
    set_has_subkey(on);
    mark_custom();
  });
  connect(format_combo_, &QComboBox::currentIndexChanged, this, [this]() {
    key_info_->SetKeyVersion(format_combo_->currentData().toInt());
  });
  connect(save_button, &QPushButton::clicked, this,
          &KeyGenerateWizard::save_as_profile);
}

void KeyGenerateWizard::setup_expert_switch() {
  // In the button row, beside Back and Continue: it decides which page comes
  // next, so it sits with the buttons that move between pages. The same
  // remembered switch as in Add Subkey.
  expert_switch_ = new ToggleSwitch(tr("Expert Mode"), this);
  expert_switch_->setToolTip(
      tr("Choose every algorithm, key length, usage and expiry yourself."));
  setButton(QWizard::CustomButton1, expert_switch_);
  setButtonText(QWizard::CustomButton1, tr("Expert Mode"));
  setOption(QWizard::HaveCustomButton1, true);
#ifdef Q_OS_MACOS
  setButtonLayout({QWizard::CustomButton1, QWizard::Stretch,
                   QWizard::CancelButton, QWizard::BackButton,
                   QWizard::NextButton, QWizard::CommitButton,
                   QWizard::FinishButton});
#else
  setButtonLayout({QWizard::CustomButton1, QWizard::Stretch,
                   QWizard::BackButton, QWizard::NextButton,
                   QWizard::CommitButton, QWizard::FinishButton,
                   QWizard::CancelButton});
#endif

  expert_switch_->setChecked(settings_.expert_mode);
  connect(expert_switch_, &QAbstractButton::toggled, this, [this](bool on) {
    StoreKeyGenExpertMode(on);
    sync_expert_mode();

    // On the second step, the other mode's page takes its place at once.
    const auto step = static_cast<KeyGenerateStep>(currentId());
    if (step == KeyGenerateStep::kKeyType ||
        step == KeyGenerateStep::kKeyDetails) {
      switching_mode_ = true;
      back();
      next();
      switching_mode_ = false;
    }
  });
  sync_expert_mode();
}

void KeyGenerateWizard::sync_expert_mode() {
  // Each key's expiry is set on Key Details; one lifetime for both would
  // quietly overwrite them.
  if (validity_card_ != nullptr) {
    validity_card_->setVisible(expiry_supported_ &&
                               !expert_switch_->isChecked());
  }
}

auto KeyGenerateWizard::nextId() const -> int {
  const auto next = NextKeyGenerateStep(
      static_cast<KeyGenerateStep>(currentId()),
      expert_switch_ != nullptr && expert_switch_->isChecked());
  return next ? StepIndex(*next) : -1;
}

void KeyGenerateWizard::build_protection_page() {
  auto* page = add_page(KeyGenerateStep::kProtection);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Decide how long the key is valid and how its secret part is "
             "protected."),
          page);

  auto* validity = new QWidget(page);
  auto* validity_layout = new QVBoxLayout(validity);
  validity_layout->setContentsMargins(0, 0, 0, 0);
  expiry_ = new ExpiryPicker(validity);
  validity_layout->addWidget(expiry_);
  validity_layout->addWidget(CreateDetailLabel(
      tr("A key that expires stops being usable on its own if you lose it. "
         "You can extend it at any time before then."),
      validity));
  validity_card_ = CreateCard(tr("Valid for"), validity, page);
  layout->addWidget(validity_card_);

  auto* protection = new QWidget(page);
  auto* protection_layout = new QVBoxLayout(protection);
  protection_layout->setContentsMargins(0, 0, 0, 0);
  passphrase_check_ =
      new QCheckBox(tr("Protect the secret key with a passphrase"), protection);
  protection_layout->addWidget(passphrase_check_);

  const auto gnupg =
      OpenPGPContext::GetInstance(channel_).Engine() == OpenPGPEngine::kGNUPG;
  protection_layout->addWidget(CreateDetailLabel(
      gnupg ? tr("GnuPG asks for the passphrase in a window of its own while "
                 "the key is generated.")
            : tr("You are asked for the passphrase while the key is "
                 "generated."),
      protection));
  passphrase_twice_ = CreateDetailLabel(
      tr("With a subkey it may be asked for twice: once for each key."),
      protection);
  passphrase_twice_->setVisible(false);
  protection_layout->addWidget(passphrase_twice_);

  passphrase_warning_ = MakeProblemLabel(protection);
  passphrase_warning_->setText(
      tr("Without a passphrase, anyone who obtains the secret key file can "
         "sign and decrypt as you."));
  protection_layout->addWidget(passphrase_warning_);
  layout->addWidget(CreateCard(tr("Protection"), protection, page));
  layout->addStretch(1);

  connect(expiry_, &ExpiryPicker::SignalChanged, this, [this]() {
    // One lifetime for both keys here; Expert Mode can set them apart.
    for (const auto& info : {key_info_, subkey_info_}) {
      if (info == nullptr) continue;
      info->SetExpireTime(expiry_->Expire());
      info->SetNonExpired(expiry_->NonExpired());
    }
    primary_editor_->Refresh();
    subkey_editor_->Refresh();
  });
  connect(passphrase_check_, &QCheckBox::toggled, this, [this](bool on) {
    key_info_->SetNonPassPhrase(!on);
    if (subkey_info_ != nullptr) subkey_info_->SetNonPassPhrase(!on);
    sync_passphrase();
  });
}

void KeyGenerateWizard::build_review_page() {
  auto* page = add_page(KeyGenerateStep::kReview);
  auto* layout = PageLayout(page);
  AddLead(layout, tr("Check the details, then generate the key."), page);

  review_ = new MetaListPanel(page);
  layout->addWidget(review_);
  layout->addStretch(1);

  page->validate = [this]() { return generate(); };
}

auto KeyGenerateWizard::state() const -> KeyGenerateState {
  KeyGenerateState s;
  s.name = name_edit_->text();
  s.email = email_edit_->text();
  s.comment = comment_edit_->text();
  s.algorithms_complete =
      primary_editor_->IsComplete() &&
      (subkey_info_ == nullptr || subkey_editor_->IsComplete());
  return s;
}

void KeyGenerateWizard::refresh_complete() {
  for (auto* page : pages_) {
    if (page != nullptr) emit page->completeChanged();
  }
}

void KeyGenerateWizard::refresh_identity_problems() {
  const auto check = CheckKeyGenIdentity(
      name_edit_->text(), email_edit_->text(), comment_edit_->text());
  // An empty name is not yet a mistake: it is only one once Continue is
  // refused, and that says so itself.
  ShowProblem(name_problem_,
              name_edit_->text().trimmed().isEmpty() ? QString() : check.name);
  ShowProblem(email_problem_, check.email);
  ShowProblem(comment_problem_, check.comment);
}

void KeyGenerateWizard::fill_from_pattern() {
  const auto uid = ExpandUidPattern(
      settings_.uid_pattern, SystemUidPatternRandom(), CurrentDateTime());
  name_edit_->setText(uid.name);
  email_edit_->setText(uid.email);
  comment_edit_->setText(uid.comment);
}

void KeyGenerateWizard::apply_profile(int index) {
  if (index < 0 || index >= offered_.size()) return;
  current_profile_ = index;

  ApplyKeyGenProfile(offered_.at(index), CurrentDateTime(), *key_info_,
                     subkey_info_);

  // The tile that stands for it, or the list that holds it; never both.
  {
    const QSignalBlocker group_blocker(profile_group_);
    const QSignalBlocker combo_blocker(more_combo_);
    const QSignalBlocker start_blocker(start_combo_);
    start_combo_->setCurrentIndex(std::max(0, start_combo_->findData(index)));
    more_combo_->setItemText(0, tr("Choose…"));
    if (auto* button = profile_group_->button(index); button != nullptr) {
      button->setChecked(true);
      more_combo_->setCurrentIndex(0);
    } else {
      // Chosen from the list: the list is shown, so the choice is in sight.
      clear_tiles();
      more_combo_->setCurrentIndex(std::max(0, more_combo_->findData(index)));
      options_toggle_->setChecked(true);
    }
  }

  {
    const QSignalBlocker blocker(subkey_check_);
    subkey_check_->setChecked(subkey_info_ != nullptr);
  }
  subkey_editor_->setVisible(subkey_info_ != nullptr);
  subkey_editor_->SetInfo(subkey_info_);
  primary_editor_->Refresh();

  key_info_->SetKeyVersion(0);
  sync_key_format();
  sync_passphrase();
  refresh_complete();
}

void KeyGenerateWizard::mark_custom() {
  current_profile_ = -1;
  {
    // No tile and no list entry: what Expert Mode shows is the choice now.
    const QSignalBlocker group_blocker(profile_group_);
    const QSignalBlocker combo_blocker(more_combo_);
    clear_tiles();
    more_combo_->setItemText(0, tr("Custom (Expert Mode)"));
    more_combo_->setCurrentIndex(0);
    options_toggle_->setChecked(true);
    const QSignalBlocker start_blocker(start_combo_);
    start_combo_->setCurrentIndex(0);
  }
  sync_key_format();
  sync_passphrase();
  refresh_complete();
}

void KeyGenerateWizard::clear_tiles() {
  // An exclusive group never lets its last checked button go.
  profile_group_->setExclusive(false);
  for (auto* button : profile_group_->buttons()) button->setChecked(false);
  profile_group_->setExclusive(true);
}

void KeyGenerateWizard::set_has_subkey(bool on) {
  if (!on) {
    subkey_info_ = nullptr;
  } else if (subkey_info_ == nullptr) {
    subkey_info_ = SecureCreateSharedObject<KeyGenerateInfo>(true);
    subkey_info_->SetNonPassPhrase(key_info_->IsNoPassPhrase());
    subkey_info_->SetAlgo(SuggestSubkeyAlgo(
        SubkeyPurpose::kEncrypt, key_info_->GetAlgo().Id(), subkey_algos_));
    subkey_info_->SetExpireTime(key_info_->GetExpireTime());
    subkey_info_->SetNonExpired(key_info_->IsNonExpired());
  }
  subkey_editor_->SetInfo(subkey_info_);
  subkey_editor_->setVisible(on);
}

void KeyGenerateWizard::sync_key_format() {
  if (!format_choice_) return;

  // Post-quantum algorithms are only defined for the v6 format, so one of
  // them pins it; otherwise the interoperable v4 stays until changed.
  const auto required = RequiredKeyVersion(*key_info_, subkey_info_);
  if (required != 0) {
    key_info_->SetKeyVersion(required);
  } else if (key_info_->GetKeyVersion() == 0) {
    key_info_->SetKeyVersion(4);
  }

  const QSignalBlocker blocker(format_combo_);
  format_combo_->setCurrentIndex(
      std::max(0, format_combo_->findData(key_info_->GetKeyVersion())));
  format_combo_->setDisabled(required != 0);
  format_combo_->setToolTip(
      required != 0 ? tr("Post-quantum algorithms require the v6 key format.")
                    : QString());
}

void KeyGenerateWizard::sync_passphrase() {
  const QSignalBlocker blocker(passphrase_check_);
  passphrase_check_->setChecked(!key_info_->IsNoPassPhrase());
  passphrase_check_->setEnabled(key_info_->IsAllowNoPassPhrase());
  passphrase_warning_->setVisible(key_info_->IsNoPassPhrase());
  passphrase_twice_->setVisible(
      subkey_info_ != nullptr && !key_info_->IsNoPassPhrase() &&
      OpenPGPContext::GetInstance(channel_).Engine() == OpenPGPEngine::kGNUPG);
}

void KeyGenerateWizard::save_as_profile() {
  bool ok = false;
  const auto name = QInputDialog::getText(
      this, tr("Save as Profile"),
      tr("A name for these choices, to find them again in the profile list:"),
      QLineEdit::Normal, {}, &ok);
  if (!ok) return;

  const auto problem = CheckKeyGenProfileName(name, settings_.profiles);
  if (problem != KeyGenProfileNameProblem::kNone) {
    QMessageBox::warning(this, tr("Profile Not Saved"),
                         DescribeKeyGenProfileNameProblem(problem));
    return;
  }

  auto profile =
      KeyGenProfileFromInfo(*key_info_, subkey_info_, CurrentDateTime());
  profile.name = name.trimmed();

  // Read again, so a change made in Settings meanwhile is not overwritten.
  settings_ = LoadKeyGenSettings();
  settings_.profiles.append(profile);
  StoreKeyGenSettings(settings_);

  if (auto resolved = ResolveKeyGenProfile(
          profile, primary_algos_, subkey_algos_,
          [this](const KeyAlgo& a) { return a.SubAlgos(channel_); })) {
    offered_.append(*resolved);
    const auto index = static_cast<int>(offered_.size() - 1);
    more_combo_->addItem(KeyGenProfileDisplayName(profile), index);
    more_combo_->setEnabled(true);
    start_combo_->addItem(KeyGenProfileDisplayName(profile), index);
    apply_profile(index);
  }
}

auto KeyGenerateWizard::generate() -> bool {
  key_info_->SetName(name_edit_->text().trimmed());
  key_info_->SetEmail(email_edit_->text().trimmed());
  key_info_->SetComment(comment_edit_->text().trimmed());

  const auto fingerprint =
      RunKeyGeneration(this, channel_, key_info_, subkey_info_);
  if (!fingerprint) return false;

  // Closed first, so the notice is not left over a wizard about to vanish.
  hide();
  ShowKeyGenerated(parentWidget(), key_info_->GetUserid(), *fingerprint);
  return true;
}

void KeyGenerateWizard::slot_page_changed(int id) {
  const auto step = static_cast<KeyGenerateStep>(id);

  // Only before the path splits: later, Back would lead into the page of the
  // mode just left.
  expert_switch_->setEnabled(step == KeyGenerateStep::kIdentity ||
                             step == KeyGenerateStep::kKeyType ||
                             step == KeyGenerateStep::kKeyDetails);

  if (step == KeyGenerateStep::kProtection) {
    const QSignalBlocker blocker(expiry_);
    expiry_->SetExpiry(key_info_->IsNonExpired(), key_info_->GetExpireTime());
  }

  if (step == KeyGenerateStep::kReview) {
    key_info_->SetName(name_edit_->text().trimmed());
    key_info_->SetEmail(email_edit_->text().trimmed());
    key_info_->SetComment(comment_edit_->text().trimmed());
    review_->SetRows(KeyGenerateReviewRows(
        *key_info_, subkey_info_,
        current_profile_ >= 0
            ? KeyGenProfileDisplayName(offered_.at(current_profile_).profile)
            : QString(),
        format_choice_, expiry_supported_));
  }

  // Queued: QWizard settles focus on its own buttons after announcing the new
  // page, and this has to land after that.
  QTimer::singleShot(0, this, [this, id]() {
    if (currentId() != id) return;
    if (static_cast<KeyGenerateStep>(id) == KeyGenerateStep::kIdentity) {
      name_edit_->setFocus();
    }
  });
}

}  // namespace GpgFrontend::UI
