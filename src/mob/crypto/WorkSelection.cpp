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

#include "crypto/WorkSelection.h"

#include <QJSEngine>
#include <algorithm>

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/GpgUtils.h"
#include "model/KeyRoleModel.h"
#include "core/model/GpgKey.h"
#include "shell/rules/RecipientConfirm.h"
#include "shell/rules/RecipientRules.h"

namespace GpgFrontend::Mobile {

namespace {

QPointer<WorkSelection> g_instance;

auto Same(const QString& a, const QString& b) -> bool {
  return a.compare(b, Qt::CaseInsensitive) == 0;
}

/// Why a key of the key list's cannot be used, as far as its status goes.
auto StatusIssue(const QVariantMap& key) -> QString {
  const auto status = key.value(QStringLiteral("status")).toString();
  if (status == QStringLiteral("expired") ||
      status == QStringLiteral("revoked") ||
      status == QStringLiteral("disabled")) {
    return status;
  }
  return {};
}

}  // namespace

WorkSelection::WorkSelection(QAbstractItemModel* keys, QObject* parent)
    : QObject(parent), keys_(keys), database_([]() {
        return LookupChannelHandle(UsableDefaultChannelHandle()).database_id;
      }) {
  connect(this, &WorkSelection::SignalChanged, this, [this]() { ++revision_; });
  if (keys == nullptr) return;
  // Whatever the list learns of a key may change what can be done with it,
  // and where it is in the list.
  const auto changed = [this]() {
    rows_valid_ = false;
    emit SignalChanged();
  };
  // A list read anew may be another database's: its own choice is taken up
  // before anything shows the old one's keys as gone.
  connect(keys, &QAbstractItemModel::modelReset, this, [this, changed]() {
    rows_valid_ = false;
    follow_database();
    changed();
  });
  connect(keys, &QAbstractItemModel::layoutChanged, this, changed);
  connect(keys, &QAbstractItemModel::rowsInserted, this, changed);
  connect(keys, &QAbstractItemModel::rowsRemoved, this, changed);
  connect(keys, &QAbstractItemModel::dataChanged, this, changed);
}

void WorkSelection::SetDatabaseSource(DatabaseSource source) {
  database_ = std::move(source);
}

void WorkSelection::SetInstance(WorkSelection* instance) {
  g_instance = instance;
}

auto WorkSelection::create(QQmlEngine*, QJSEngine*) -> WorkSelection* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

auto WorkSelection::Instance() -> WorkSelection* { return g_instance; }

auto WorkSelection::Recipients() const -> QStringList { return recipients_; }

auto WorkSelection::Revision() const -> int { return revision_; }

auto WorkSelection::RecipientCount() const -> int {
  return static_cast<int>(recipients_.size());
}

auto WorkSelection::ShownRecipientKeys() const -> QVariantList {
  QVariantList troubled;
  QVariantList fine;
  for (const auto& key : recipient_keys()) {
    if (!key.toMap().value(QStringLiteral("issue")).toString().isEmpty()) {
      troubled << key;
    } else if (fine.size() < kShownRecipients) {
      fine << key;
    }
  }
  const auto room = std::max<qsizetype>(0, kShownRecipients - troubled.size());
  return troubled + fine.mid(0, room);
}

auto WorkSelection::RecipientKeys() const -> QVariantList {
  return recipient_keys();
}

auto WorkSelection::recipient_keys() const -> const QVariantList& {
  // Worked out once per change: every line and row that shows the choice
  // asks for it, as do the blocks and the issue count, and with hundreds of
  // recipients each answer was a pass over all of them.
  if (keys_revision_ != revision_) {
    recipient_keys_.clear();
    recipient_keys_.reserve(recipients_.size());
    for (const auto& fpr : recipients_) recipient_keys_ << recipient_key(fpr);
    keys_revision_ = revision_;
  }
  return recipient_keys_;
}

auto WorkSelection::PassphraseMode() const -> bool { return passphrase_mode_; }

void WorkSelection::SetPassphraseMode(bool on) {
  if (passphrase_mode_ == on) return;
  passphrase_mode_ = on;
  emit SignalChanged();
}

auto WorkSelection::SigningEnabled() const -> bool { return signing_enabled_; }

void WorkSelection::SetSigningEnabled(bool on) {
  if (signing_enabled_ == on) return;
  signing_enabled_ = on;
  emit SignalChanged();
}

auto WorkSelection::PreferredSigner() const -> QString {
  return preferred_signer_;
}

auto WorkSelection::SignerKey() const -> QVariantMap {
  if (preferred_signer_.isEmpty()) return {};
  auto key = lookup(preferred_signer_);
  if (key.isEmpty()) {
    key.insert(QStringLiteral("fingerprint"), preferred_signer_);
  }
  const auto issue = signer_issue();
  key.insert(QStringLiteral("issue"), issue);
  // Why, as a recipient's note says it: the shell's words where it has
  // them, the key's state, else what is missing.
  QString note;
  if (issue == QStringLiteral("missing")) {
    note = QCoreApplication::translate("Wording", "No longer in your keys");
  } else if (issue == QStringLiteral("no_secret")) {
    note = QCoreApplication::translate("Wording",
                                       "Its secret part is not on this device");
  } else if (!issue.isEmpty()) {
    note = key.value(QStringLiteral("signNote")).toString();
    if (note.isEmpty()) {
      note = key.value(QStringLiteral("statusText")).toString();
    }
  }
  key.insert(QStringLiteral("note"), note);
  return key;
}

auto WorkSelection::SuggestedSigner() const -> QString {
  if (!preferred_signer_.isEmpty() || keys_ == nullptr || !ready_) return {};
  QString only;
  for (int row = 0; row < keys_->rowCount(); ++row) {
    const auto at = keys_->index(row, 0);
    if (!at.data(KeyRoleModel::kIsKeyRole).toBool() ||
        !at.data(KeyRoleModel::kHasSecretRole).toBool() ||
        !at.data(KeyRoleModel::kCanSignRole).toBool() ||
        !at.data(KeyRoleModel::kUsableRole).toBool()) {
      continue;
    }
    if (!only.isEmpty()) return {};  // more than one: the person chooses
    only = at.data(KeyRoleModel::kFingerprintRole).toString();
  }
  return only;
}

auto WorkSelection::EncryptBlock() const -> QString {
  if (passphrase_mode_) return {};
  if (recipients_.isEmpty()) return QStringLiteral("no_recipients");
  if (any_recipient_issue()) return QStringLiteral("recipient_issue");
  if (signing_enabled_) {
    if (preferred_signer_.isEmpty()) return QStringLiteral("no_signer");
    if (!signer_issue().isEmpty()) return QStringLiteral("signer_issue");
  }
  return {};
}

auto WorkSelection::SignBlock() const -> QString {
  if (preferred_signer_.isEmpty()) return QStringLiteral("no_signer");
  if (!signer_issue().isEmpty()) return QStringLiteral("signer_issue");
  return {};
}

auto WorkSelection::SignsEncryption() const -> bool {
  // An encryption to a passphrase is not signed: the engine encrypts it
  // alone.
  return signing_enabled_ && !passphrase_mode_;
}

auto WorkSelection::RecipientIssueCount() const -> int {
  int count = 0;
  for (const auto& key : recipient_keys()) {
    if (!key.toMap().value(QStringLiteral("issue")).toString().isEmpty()) {
      ++count;
    }
  }
  return count;
}

auto WorkSelection::IssueCount() const -> int {
  auto count = RecipientIssueCount();
  if (!preferred_signer_.isEmpty() && !signer_issue().isEmpty()) ++count;
  return count;
}

auto WorkSelection::RecipientCategory() const -> QString {
  return recipient_category_;
}

void WorkSelection::SetRecipientCategory(const QString& category) {
  if (recipient_category_ == category) return;
  recipient_category_ = category;
  emit SignalChanged();
}

auto WorkSelection::SignerCategory() const -> QString {
  return signer_category_;
}

void WorkSelection::SetSignerCategory(const QString& category) {
  if (signer_category_ == category) return;
  signer_category_ = category;
  emit SignalChanged();
}

auto WorkSelection::DroppedCount() const -> int { return dropped_; }

void WorkSelection::SetKeysReady() {
  ready_ = true;
  database_id_ = database_ ? database_() : QString();
  auto settings = GetSettings();
  preferred_signer_ = SettingItems::kPreferredSigner.Read(settings);
  // As when another database is taken up (follow_database()): one that
  // cannot sign here is not offered; the setting keeps it for the
  // database it is of.
  if (!signer_issue().isEmpty()) preferred_signer_.clear();
  // What an earlier version kept to offer "Again": the fingerprints of the
  // last operation, which nothing reads any more.
  settings.remove(QLatin1StringView(SettingItems::kLastOperation.key));
  emit SignalChanged();
}

void WorkSelection::KeysRefreshed() { follow_database(); }

void WorkSelection::addRecipient(const QString& fingerprint) {
  if (fingerprint.isEmpty() || isRecipient(fingerprint)) return;
  touched();
  recipients_ << fingerprint;
  recipient_set_.insert(fingerprint.toUpper());
  emit SignalChanged();
}

void WorkSelection::removeRecipient(const QString& fingerprint) {
  if (!recipient_set_.remove(fingerprint.toUpper())) return;
  touched();
  recipients_.removeIf(
      [&fingerprint](const QString& f) { return Same(f, fingerprint); });
  emit SignalChanged();
}

void WorkSelection::toggleRecipient(const QString& fingerprint) {
  if (isRecipient(fingerprint)) {
    removeRecipient(fingerprint);
  } else {
    addRecipient(fingerprint);
  }
}

auto WorkSelection::isRecipient(const QString& fingerprint) const -> bool {
  return recipient_set_.contains(fingerprint.toUpper());
}

auto WorkSelection::encryptQuestions() const -> QVariantList {
  if (passphrase_mode_ || recipients_.isEmpty()) return {};
  auto& repository =
      AbstractKeyRepository::GetInstance(UsableDefaultChannelHandle());
  GpgAbstractKeyPtrList keys;
  // Judged on the final list, after any subkey pinned (as the desktop).
  for (const auto& fingerprint : operationRecipients()) {
    if (auto key = repository.GetKey(fingerprint); key != nullptr) {
      keys.append(key);
    }
  }
  const auto confirm_many =
      SettingItems::kConfirmMultipleRecipients.Read(GetSettings());

  QVariantList questions;
  for (const auto& question :
       Shell::EncryptQuestions(recipients_.size(), keys, confirm_many)) {
    questions.append(QVariantMap{
        {QStringLiteral("title"), question.title},
        {QStringLiteral("text"), question.text},
        {QStringLiteral("detail"), question.detail},
        {QStringLiteral("accept"), question.accept},
    });
  }
  return questions;
}

void WorkSelection::clearRecipients() {
  if (recipients_.isEmpty() && dropped_ == 0) return;
  touched();
  recipients_.clear();
  recipient_set_.clear();
  emit SignalChanged();
}

void WorkSelection::setPreferredSigner(const QString& fingerprint) {
  if (preferred_signer_ == fingerprint) return;
  touched();
  preferred_signer_ = fingerprint;
  if (ready_) {
    auto settings = GetSettings();
    SettingItems::kPreferredSigner.Write(settings, fingerprint);
  }
  emit SignalChanged();
}

auto WorkSelection::subkeyChoices(const QString& op) const -> QVariantList {
  // No keys before a profile is open.
  if (!ProfileSession::Loaded()) return {};
  auto& repository =
      AbstractKeyRepository::GetInstance(UsableDefaultChannelHandle());
  QVariantList choices;
  const auto ask = [&](const QString& fingerprint, const QString& role) {
    if (IsKeyGroupID(fingerprint)) return;
    const auto key = repository.GetKey(fingerprint);
    const auto subkeys = role == QLatin1String("sign")
                             ? UsableSigningSubkeys(key)
                             : UsableEncryptionSubkeys(key);
    if (subkeys.size() < 2) return;
    QVariantList listed;
    for (const auto& s_key : subkeys) {
      listed << QVariantMap{
          {QStringLiteral("fingerprint"), s_key.Fingerprint()},
          {QStringLiteral("keyId"), s_key.ID()},
          {QStringLiteral("algorithm"), s_key.Algo()},
          {QStringLiteral("created"), s_key.CreationTime()},
          {QStringLiteral("expires"), s_key.ExpirationTime()},
          {QStringLiteral("primary"),
           s_key.Fingerprint().compare(key->Fingerprint(),
                                       Qt::CaseInsensitive) == 0},
      };
    }
    choices << QVariantMap{
        {QStringLiteral("key"), fingerprint},
        {QStringLiteral("name"),
         key->Name().isEmpty() ? key->Email() : key->Name()},
        {QStringLiteral("role"), role},
        {QStringLiteral("subkeys"), listed},
    };
  };
  if (op == QLatin1String("encrypt") && !passphrase_mode_) {
    for (const auto& fingerprint : recipients_) {
      ask(fingerprint, QStringLiteral("encrypt"));
    }
  }
  const auto signs = op == QLatin1String("sign") ||
                     (op == QLatin1String("encrypt") && SignsEncryption());
  if (signs && !preferred_signer_.isEmpty()) {
    ask(preferred_signer_, QStringLiteral("sign"));
  }
  return choices;
}

void WorkSelection::pinSubkey(const QString& key, const QString& subkey) {
  if (subkey.isEmpty()) {
    pins_.remove(key.toUpper());
  } else {
    pins_.insert(key.toUpper(), subkey.toUpper());
  }
}

void WorkSelection::clearPins() { pins_.clear(); }

auto WorkSelection::pinned(const QString& fingerprint) const -> QString {
  const auto subkey = pins_.value(fingerprint.toUpper());
  return subkey.isEmpty() ? fingerprint : subkey + QLatin1Char('!');
}

auto WorkSelection::operationRecipients() const -> QStringList {
  QStringList taken;
  for (const auto& fingerprint : recipients_) taken << pinned(fingerprint);
  return taken;
}

auto WorkSelection::operationSigner() const -> QString {
  return preferred_signer_.isEmpty() ? QString() : pinned(preferred_signer_);
}

void WorkSelection::reset() {
  touched();
  pins_.clear();
  recipients_.clear();
  recipient_set_.clear();
  passphrase_mode_ = false;
  signing_enabled_ = false;
  emit SignalChanged();
}

void WorkSelection::revalidate() {
  if (!ready_) return;
  const auto before = database_id_;
  follow_database();
  // Said for this opening: what an earlier one left out is old news, but
  // a choice just taken up for another database says what it lost.
  const auto dropped = drop_unusable();
  const auto now = database_id_ != before ? dropped_ + dropped : dropped;
  if (now != dropped_) {
    dropped_ = now;
    emit SignalChanged();
  }
}

void WorkSelection::touched() { dropped_ = 0; }

void WorkSelection::follow_database() {
  if (!ready_ || !database_) return;
  const auto id = database_();
  if (id == database_id_) return;

  if (!database_id_.isEmpty()) {
    remembered_.insert(database_id_, {recipients_, preferred_signer_,
                                      recipient_category_, signer_category_});
  }
  database_id_ = id;
  rows_valid_ = false;

  const auto known = remembered_.constFind(id);
  const auto chosen = known != remembered_.constEnd() ? *known : Remembered{};
  recipients_.clear();
  recipient_set_.clear();
  for (const auto& fpr : chosen.recipients) {
    recipients_ << fpr;
    recipient_set_.insert(fpr.toUpper());
  }
  recipient_category_ = chosen.recipient_category;
  signer_category_ = chosen.signer_category;
  dropped_ = 0;
  if (known != remembered_.constEnd()) {
    preferred_signer_ = chosen.signer;
    dropped_ = drop_unusable();
  } else {
    // Never here before: the signer kept beyond the session, if this
    // database has it and it can sign; else none. Not chosen here, so not
    // counted as left out.
    preferred_signer_ = SettingItems::kPreferredSigner.Read(GetSettings());
    if (!signer_issue().isEmpty()) preferred_signer_.clear();
  }
  emit SignalChanged();
}

auto WorkSelection::drop_unusable() -> int {
  if (!ready_) return 0;
  int dropped = 0;
  QStringList kept;
  for (const auto& fpr : recipients_) {
    if (recipient_key(fpr)
            .value(QStringLiteral("issue"))
            .toString()
            .isEmpty()) {
      kept << fpr;
    } else {
      ++dropped;
    }
  }
  if (dropped > 0) {
    recipients_ = kept;
    recipient_set_.clear();
    for (const auto& fpr : kept) recipient_set_.insert(fpr.toUpper());
  }
  // Left out here only: the setting keeps it for the database it is of.
  if (!preferred_signer_.isEmpty() && !signer_issue().isEmpty()) {
    preferred_signer_.clear();
    ++dropped;
  }
  return dropped;
}

auto WorkSelection::lookup(const QString& fingerprint) const -> QVariantMap {
  if (keys_ == nullptr || !ready_) return {};
  // Found by an index of the list rather than a pass over it: asked once per
  // chosen key on every change, which with a thousand keys and hundreds
  // chosen was a pass over the list hundreds of times.
  if (!rows_valid_) {
    rows_.clear();
    key_info_.clear();
    for (int row = 0; row < keys_->rowCount(); ++row) {
      const auto at = keys_->index(row, 0);
      // Key groups too: one can be a recipient.
      rows_.insert(at.data(KeyRoleModel::kFingerprintRole).toString().toUpper(),
                   row);
    }
    rows_valid_ = true;
  }
  const auto upper = fingerprint.toUpper();
  if (const auto known = key_info_.constFind(upper);
      known != key_info_.constEnd()) {
    return *known;
  }
  const auto found = rows_.constFind(upper);
  if (found == rows_.constEnd()) return {};
  const auto at = keys_->index(*found, 0);
  // Every fact the list has of it (its details read them), and what a
  // choice needs besides.
  auto info = KeyRoleModel::MapOf(at);
  const auto is_key = at.data(KeyRoleModel::kIsKeyRole).toBool();
  info.insert(QStringLiteral("isGroup"), !is_key);
  info.insert(QStringLiteral("memberCount"),
              at.data(KeyRoleModel::kMembersRole).toStringList().size());
  return key_info_[upper] = info;
}

auto WorkSelection::recipient_key(const QString& fingerprint) const
    -> QVariantMap {
  auto key = lookup(fingerprint);
  QString issue;
  if (!ready_) {
    // Nothing known yet: not a problem to report.
  } else if (key.isEmpty()) {
    issue = QStringLiteral("missing");
  } else if (const auto status = StatusIssue(key); !status.isEmpty()) {
    issue = status;
  } else if (!key.value(QStringLiteral("canEncrypt")).toBool()) {
    issue = QStringLiteral("cannot_encrypt");
  } else if (key.value(QStringLiteral("isGroup")).toBool() &&
             key.value(QStringLiteral("memberCount")).toInt() == 0) {
    // A group with nobody in it would encrypt to nobody.
    issue = QStringLiteral("empty_group");
  } else if (key.value(QStringLiteral("isGroup")).toBool()) {
    // A member gone or disabled: the engine refuses the group (it would
    // otherwise go to fewer people than it names), so it is stopped here.
    const auto channel = UsableDefaultChannelHandle();
    const auto group =
        AbstractKeyRepository::GetInstance(channel).GetKey(fingerprint);
    if (group != nullptr && !UnusableKeyIds(channel, {group}).isEmpty()) {
      issue = QStringLiteral("cannot_encrypt");
      key.insert(
          QStringLiteral("encryptNote"),
          QCoreApplication::translate("Wording", "Cannot be encrypted to"));
    }
  }
  // A rule the shell decides for both interfaces: whatever it says stops
  // the key, said in its words.
  const auto note = key.value(QStringLiteral("encryptNote")).toString();
  if (issue.isEmpty() && !note.isEmpty())
    issue = QStringLiteral("cannot_encrypt");
  if (key.isEmpty()) key.insert(QStringLiteral("fingerprint"), fingerprint);
  key.insert(QStringLiteral("issue"), issue);
  key.insert(
      QStringLiteral("note"),
      issue == QStringLiteral("missing")
          ? QCoreApplication::translate("Wording", "No longer in your keys")
          : note);
  return key;
}

auto WorkSelection::signer_issue() const -> QString {
  if (preferred_signer_.isEmpty() || !ready_) return {};
  const auto key = lookup(preferred_signer_);
  if (key.isEmpty()) return QStringLiteral("missing");
  if (!key.value(QStringLiteral("hasSecret")).toBool()) {
    return QStringLiteral("no_secret");
  }
  if (const auto status = StatusIssue(key); !status.isEmpty()) return status;
  if (!key.value(QStringLiteral("canSign")).toBool()) {
    return QStringLiteral("cannot_sign");
  }
  // A rule the shell decides for both interfaces, as for recipients.
  if (!key.value(QStringLiteral("signNote")).toString().isEmpty()) {
    return QStringLiteral("cannot_sign");
  }
  return {};
}

auto WorkSelection::any_recipient_issue() const -> bool {
  const auto& keys = recipient_keys();
  return std::any_of(keys.begin(), keys.end(), [](const QVariant& key) {
    return !key.toMap().value(QStringLiteral("issue")).toString().isEmpty();
  });
}

}  // namespace GpgFrontend::Mobile
