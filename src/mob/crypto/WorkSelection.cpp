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

#include <QCryptographicHash>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "model/KeyRoleModel.h"
#include "shell/rules/RecipientConfirm.h"

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
    : QObject(parent), keys_(keys) {
  connect(this, &WorkSelection::SignalChanged, this, [this]() { ++revision_; });
  if (keys == nullptr) return;
  // Whatever the list learns of a key may change what can be done with it,
  // and where it is in the list.
  const auto changed = [this]() {
    rows_valid_ = false;
    emit SignalChanged();
  };
  connect(keys, &QAbstractItemModel::modelReset, this, changed);
  connect(keys, &QAbstractItemModel::layoutChanged, this, changed);
  connect(keys, &QAbstractItemModel::rowsInserted, this, changed);
  connect(keys, &QAbstractItemModel::rowsRemoved, this, changed);
  connect(keys, &QAbstractItemModel::dataChanged, this, changed);
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
  key.insert(QStringLiteral("issue"), signer_issue());
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

void WorkSelection::SetKeysReady() {
  ready_ = true;
  auto settings = GetSettings();
  preferred_signer_ = SettingItems::kPreferredSigner.Read(settings);
  last_ =
      QJsonDocument::fromJson(
          settings.value(QLatin1StringView(SettingItems::kLastOperation.key))
              .toByteArray())
          .object()
          .toVariantMap();
  KeysRefreshed();
  emit SignalChanged();
}

void WorkSelection::KeysRefreshed() {
  if (!ready_ || last_.isEmpty()) return;
  // Made with other keys than there are now: what it would do is not what
  // was done, so it is not offered again.
  if (last_.value(QStringLiteral("digest")).toString() != KeysDigest()) {
    forgetLast();
  }
}

auto WorkSelection::KeysDigest() const -> QString {
  QStringList rows;
  if (keys_ != nullptr) {
    rows.reserve(keys_->rowCount());
    for (int row = 0; row < keys_->rowCount(); ++row) {
      const auto at = keys_->index(row, 0);
      if (!at.data(KeyRoleModel::kIsKeyRole).toBool()) {
        // A key group: who it is, and who is in it.
        auto members = at.data(KeyRoleModel::kMembersRole).toStringList();
        members.sort();
        rows << at.data(KeyRoleModel::kFingerprintRole).toString() +
                    QStringLiteral("|group|") + members.join(QLatin1Char(','));
        continue;
      }
      rows << QStringList{at.data(KeyRoleModel::kFingerprintRole)
                              .toString()
                              .toUpper(),
                          // Usable or not, not the status: a key that comes to
                          // expire soon is still the same choice.
                          at.data(KeyRoleModel::kUsableRole).toString(),
                          at.data(KeyRoleModel::kHasSecretRole).toString(),
                          at.data(KeyRoleModel::kCanEncryptRole).toString(),
                          at.data(KeyRoleModel::kCanSignRole).toString(),
                          at.data(KeyRoleModel::kExpiresRole)
                              .toDateTime()
                              .toString(Qt::ISODate)}
                  .join(QLatin1Char('|'));
    }
  }
  // In an order of its own: the list's may be sorted any way.
  rows.sort();
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(
      LookupChannelHandle(UsableDefaultChannelHandle()).database_id.toUtf8());
  for (const auto& row : rows) {
    hash.addData(QByteArrayView("\n"));
    hash.addData(row.toUtf8());
  }
  return QString::fromLatin1(hash.result().toHex());
}

auto WorkSelection::LastOperation() const -> QVariantMap {
  if (!ready_ || last_.isEmpty()) return {};
  const auto op = last_.value(QStringLiteral("op")).toString();
  const auto fprs = last_.value(QStringLiteral("recipients")).toStringList();
  QVariantList shown;
  for (const auto& fpr : fprs.mid(0, 2)) shown << recipient_key(fpr);
  const auto signer = last_.value(QStringLiteral("signer")).toString();
  QVariantMap signer_key;
  if (!signer.isEmpty()) {
    signer_key = lookup(signer);
    if (signer_key.isEmpty()) {
      signer_key.insert(QStringLiteral("fingerprint"), signer);
    }
  }
  return {
      {QStringLiteral("op"), op},
      {QStringLiteral("recipientCount"), fprs.size()},
      {QStringLiteral("recipientKeys"), shown},
      {QStringLiteral("signs"), last_.value(QStringLiteral("signs")).toBool()},
      {QStringLiteral("signerKey"), signer_key},
  };
}

void WorkSelection::rememberOperation(const QString& op) {
  if (!ready_) return;
  const bool encrypt = op == QStringLiteral("encrypt");
  const bool signs =
      (encrypt && signing_enabled_) || op == QStringLiteral("sign");
  last_ = {
      {QStringLiteral("op"), op},
      {QStringLiteral("recipients"), encrypt ? recipients_ : QStringList{}},
      {QStringLiteral("signs"), encrypt && signing_enabled_},
      {QStringLiteral("signer"), signs ? preferred_signer_ : QString()},
      {QStringLiteral("digest"), KeysDigest()},
  };
  auto settings = GetSettings();
  settings.setValue(
      QLatin1StringView(SettingItems::kLastOperation.key),
      QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(last_))
                            .toJson(QJsonDocument::Compact)));
  emit SignalChanged();
}

auto WorkSelection::restoreLast() -> QString {
  if (!ready_ || last_.isEmpty()) return {};
  const auto op = last_.value(QStringLiteral("op")).toString();
  // Only an encryption names recipients: signing or a passphrase leaves the
  // ones chosen meanwhile as they are.
  if (op == QStringLiteral("encrypt")) {
    recipients_.clear();
    recipient_set_.clear();
    for (const auto& fpr :
         last_.value(QStringLiteral("recipients")).toStringList()) {
      recipients_ << fpr;
      recipient_set_.insert(fpr.toUpper());
    }
  }
  passphrase_mode_ = op == QStringLiteral("symmetric");
  signing_enabled_ = last_.value(QStringLiteral("signs")).toBool();
  const auto signer = last_.value(QStringLiteral("signer")).toString();
  if (!signer.isEmpty() && signer != preferred_signer_) {
    preferred_signer_ = signer;
    auto settings = GetSettings();
    SettingItems::kPreferredSigner.Write(settings, signer);
  }
  emit SignalChanged();
  return op;
}

void WorkSelection::forgetLast() {
  if (last_.isEmpty()) return;
  last_.clear();
  if (ready_) {
    auto settings = GetSettings();
    settings.remove(QLatin1StringView(SettingItems::kLastOperation.key));
  }
  emit SignalChanged();
}

void WorkSelection::addRecipient(const QString& fingerprint) {
  if (fingerprint.isEmpty() || isRecipient(fingerprint)) return;
  recipients_ << fingerprint;
  recipient_set_.insert(fingerprint.toUpper());
  emit SignalChanged();
}

void WorkSelection::removeRecipient(const QString& fingerprint) {
  if (!recipient_set_.remove(fingerprint.toUpper())) return;
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
  for (const auto& fingerprint : recipients_) {
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
  if (recipients_.isEmpty()) return;
  recipients_.clear();
  recipient_set_.clear();
  emit SignalChanged();
}

void WorkSelection::setPreferredSigner(const QString& fingerprint) {
  if (preferred_signer_ == fingerprint) return;
  preferred_signer_ = fingerprint;
  if (ready_) {
    auto settings = GetSettings();
    SettingItems::kPreferredSigner.Write(settings, fingerprint);
  }
  emit SignalChanged();
}

void WorkSelection::reset() {
  recipients_.clear();
  recipient_set_.clear();
  passphrase_mode_ = false;
  signing_enabled_ = false;
  emit SignalChanged();
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
  return key_info_[upper] = {
             {QStringLiteral("fingerprint"),
              at.data(KeyRoleModel::kFingerprintRole)},
             {QStringLiteral("name"), at.data(KeyRoleModel::kNameRole)},
             {QStringLiteral("email"), at.data(KeyRoleModel::kEmailRole)},
             {QStringLiteral("keyId"), at.data(KeyRoleModel::kKeyIdRole)},
             {QStringLiteral("hasSecret"),
              at.data(KeyRoleModel::kHasSecretRole)},
             {QStringLiteral("canEncrypt"),
              at.data(KeyRoleModel::kCanEncryptRole)},
             {QStringLiteral("canSign"), at.data(KeyRoleModel::kCanSignRole)},
             {QStringLiteral("status"), at.data(KeyRoleModel::kStatusRole)},
             {QStringLiteral("statusText"),
              at.data(KeyRoleModel::kStatusTextRole)},
             {QStringLiteral("encryptNote"),
              at.data(KeyRoleModel::kEncryptNoteRole)},
             {QStringLiteral("signNote"), at.data(KeyRoleModel::kSignNoteRole)},
             {QStringLiteral("isGroup"),
              !at.data(KeyRoleModel::kIsKeyRole).toBool()},
             {QStringLiteral("memberCount"),
              at.data(KeyRoleModel::kMembersRole).toStringList().size()},
         };
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
  return {};
}

auto WorkSelection::any_recipient_issue() const -> bool {
  const auto& keys = recipient_keys();
  return std::any_of(keys.begin(), keys.end(), [](const QVariant& key) {
    return !key.toMap().value(QStringLiteral("issue")).toString().isEmpty();
  });
}

}  // namespace GpgFrontend::Mobile
