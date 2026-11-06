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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <functional>

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend::Mobile {

/**
 * @brief The keys the Text and Files pages work with, chosen once for the
 * whole session: who to encrypt to, or a passphrase instead; and the
 * signing key, with whether encryptions are signed.
 *
 * It lasts until Reset keys or the end of the process. Clearing the text or
 * the file list leaves it, and no operation changes it. Only the preferred
 * signer is kept beyond the session (SettingItems::kPreferredSigner), and
 * remembering it never turns signing on.
 *
 * Each key database has a choice of its own: switching to another puts the
 * recipients, the signer and the category each step showed away under the
 * database left, and takes up what was chosen in the one switched to (none
 * the first time). A key of one database is never offered as chosen in
 * another.
 *
 * A chosen key that can no longer be used -- gone, expired, revoked,
 * disabled, unable to encrypt or sign -- stays chosen and is reported
 * (issues, encryptBlock, signBlock) while it is being chosen from: dropping
 * it would encrypt to fewer people than were chosen, and look no different.
 * A choice taken up again (revalidate(), a database switched back to) leaves
 * such keys out, and says how many (droppedCount).
 *
 * What it knows of each key it reads from @p keys, the key list's
 * KeyRoleModel, and nothing of a key before SetKeysReady().
 */
class WorkSelection : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  /// Fingerprints, in the order chosen.
  Q_PROPERTY(QStringList recipients READ Recipients NOTIFY SignalChanged)
  /// [{fingerprint, name, email, keyId, status, statusText, issue}]: issue
  /// is empty for a key that can be used, else "missing", "expired",
  /// "revoked", "disabled", "cannot_encrypt"; status is the key list's
  /// ("expiring_soon" warns but does not block).
  Q_PROPERTY(QVariantList recipientKeys READ RecipientKeys NOTIFY SignalChanged)
  /// Encrypt to a passphrase instead of to the recipients.
  Q_PROPERTY(bool passphraseMode READ PassphraseMode WRITE SetPassphraseMode
                 NOTIFY SignalChanged)
  /// Whether an encryption to keys is signed too; never remembered.
  Q_PROPERTY(bool signingEnabled READ SigningEnabled WRITE SetSigningEnabled
                 NOTIFY SignalChanged)
  /// The remembered signing key's fingerprint; empty when none.
  Q_PROPERTY(QString preferredSigner READ PreferredSigner NOTIFY SignalChanged)
  /// The preferred signer as recipientKeys has a key, with "no_secret" and
  /// "cannot_sign" as further issues; empty when there is none.
  Q_PROPERTY(QVariantMap signerKey READ SignerKey NOTIFY SignalChanged)
  /// The one usable signing key of your own when no signer is preferred
  /// and there is exactly one; offered, never used unchosen.
  Q_PROPERTY(QString suggestedSigner READ SuggestedSigner NOTIFY SignalChanged)
  /// Why Encrypt cannot run now: "no_recipients", "recipient_issue",
  /// "no_signer", "signer_issue"; empty when it can.
  Q_PROPERTY(QString encryptBlock READ EncryptBlock NOTIFY SignalChanged)
  /// Why Sign cannot run now: "no_signer", "signer_issue"; empty when it
  /// can.
  Q_PROPERTY(QString signBlock READ SignBlock NOTIFY SignalChanged)
  /// Whether an encryption now would be signed.
  Q_PROPERTY(bool signsEncryption READ SignsEncryption NOTIFY SignalChanged)
  /// Of those, the recipients' alone: what the summary of the chosen keys
  /// warns of (a signer's problem is the signing step's).
  Q_PROPERTY(
      int recipientIssueCount READ RecipientIssueCount NOTIFY SignalChanged)
  /// Changes whenever anything above does: a row binds `revision` and calls
  /// isRecipient(), rather than copying and searching `recipients` (a
  /// thousand rows, each searching the whole list, on every tick).
  Q_PROPERTY(int revision READ Revision NOTIFY SignalChanged)
  /// How many recipients there are, without the list.
  Q_PROPERTY(int recipientCount READ RecipientCount NOTIFY SignalChanged)
  /// What a summary shows of them: every one with a problem, then the
  /// others, kShownRecipients in all unless more have problems. A page that
  /// bound `recipientKeys` copied every recipient into the interface on
  /// every tick.
  Q_PROPERTY(QVariantList shownRecipientKeys READ ShownRecipientKeys NOTIFY
                 SignalChanged)
  /// The last operation that needed keys, to do again in one tap (the quick
  /// path): {op: "encrypt" | "symmetric" | "sign", recipientCount,
  /// recipientKeys (the first few), signs, signerKey}; empty when there is
  /// none, or when the keys have changed since (any import, deletion, edit
  /// or switch of key database makes it stale).
  Q_PROPERTY(QVariantMap lastOperation READ LastOperation NOTIFY SignalChanged)
  /// The key category the recipients were last chosen in; "" for all keys.
  Q_PROPERTY(QString recipientCategory READ RecipientCategory WRITE
                 SetRecipientCategory NOTIFY SignalChanged)
  /// The key category the signer was last chosen in; "" for all keys.
  Q_PROPERTY(QString signerCategory READ SignerCategory WRITE SetSignerCategory
                 NOTIFY SignalChanged)
  /// How many chosen keys (the signer among them) were left out when the
  /// choice was last taken up again, because they could no longer be used;
  /// 0 once anything is chosen or taken out.
  Q_PROPERTY(int droppedCount READ DroppedCount NOTIFY SignalChanged)

 public:
  /// @p keys is the key list's KeyRoleModel (KeyListController::Keys()).
  explicit WorkSelection(QAbstractItemModel* keys, QObject* parent = nullptr);

  /// Where the key database in use is read from: by default the default
  /// channel's. Tests stand in their own.
  using DatabaseSource = std::function<QString()>;
  void SetDatabaseSource(DatabaseSource source);

  static void SetInstance(WorkSelection* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> WorkSelection*;
  static auto Instance() -> WorkSelection*;

  [[nodiscard]] auto Recipients() const -> QStringList;
  [[nodiscard]] auto RecipientKeys() const -> QVariantList;
  [[nodiscard]] auto Revision() const -> int;
  [[nodiscard]] auto RecipientCount() const -> int;
  [[nodiscard]] auto ShownRecipientKeys() const -> QVariantList;

  /// How many recipients a summary shows, at least.
  static constexpr int kShownRecipients = 5;
  [[nodiscard]] auto PassphraseMode() const -> bool;
  void SetPassphraseMode(bool on);
  [[nodiscard]] auto SigningEnabled() const -> bool;
  void SetSigningEnabled(bool on);
  [[nodiscard]] auto PreferredSigner() const -> QString;
  [[nodiscard]] auto SignerKey() const -> QVariantMap;
  [[nodiscard]] auto SuggestedSigner() const -> QString;
  [[nodiscard]] auto EncryptBlock() const -> QString;
  [[nodiscard]] auto SignBlock() const -> QString;
  [[nodiscard]] auto SignsEncryption() const -> bool;
  [[nodiscard]] auto IssueCount() const -> int;
  [[nodiscard]] auto RecipientIssueCount() const -> int;
  [[nodiscard]] auto RecipientCategory() const -> QString;
  void SetRecipientCategory(const QString& category);
  [[nodiscard]] auto SignerCategory() const -> QString;
  void SetSignerCategory(const QString& category);
  [[nodiscard]] auto DroppedCount() const -> int;

  /// The keys can be read now (a profile is open and its keys listed):
  /// the preferred signer and the last operation are read from the
  /// settings, the keys looked up.
  void SetKeysReady();

  /// The key list was read again: a last operation remembered with other
  /// keys than there are now is forgotten, and a key database switched to
  /// takes up its own choice.
  void KeysRefreshed();

  [[nodiscard]] auto LastOperation() const -> QVariantMap;

  /// What the key list holds now, as a digest: its database, and each key
  /// and what it can do. A last operation is kept with the digest it was
  /// made with.
  [[nodiscard]] auto KeysDigest() const -> QString;

  /// Remember the operation @p op ("encrypt", "symmetric" or "sign") with
  /// the keys chosen now, as the quick path. Kept in the settings.
  Q_INVOKABLE void rememberOperation(const QString& op);
  /// Choose the keys of the last operation again; its op, or empty when
  /// there is none. The caller then runs it, or asks for what is missing.
  Q_INVOKABLE QString restoreLast();
  /// Forget the last operation (any change to the keys does it).
  void forgetLast();

  Q_INVOKABLE void addRecipient(const QString& fingerprint);
  Q_INVOKABLE void removeRecipient(const QString& fingerprint);
  Q_INVOKABLE void toggleRecipient(const QString& fingerprint);
  [[nodiscard]] Q_INVOKABLE bool isRecipient(const QString& fingerprint) const;
  Q_INVOKABLE void clearRecipients();
  /// The choice taken up again (the toolbox opened): chosen keys that can no
  /// longer be used are left out, counted in droppedCount. Never runs
  /// anything.
  Q_INVOKABLE void revalidate();
  /// What to ask before encrypting to the chosen recipients, in order, as
  /// the desktop asks it (Shell::EncryptQuestions): [{title, text, detail,
  /// accept}]; empty when it may go ahead.
  [[nodiscard]] Q_INVOKABLE QVariantList encryptQuestions() const;

  /// Remember @p fingerprint as the signing key; empty forgets it.
  Q_INVOKABLE void setPreferredSigner(const QString& fingerprint);

  /// Reset keys: no recipients, encrypt to keys, unsigned. The preferred
  /// signer stays.
  Q_INVOKABLE void reset();

 signals:
  void SignalChanged();

 private:
  /// What was chosen in a key database, put away while another is in use.
  struct Remembered {
    QStringList recipients;
    QString signer;
    QString recipient_category;
    QString signer_category;
  };

  QPointer<QAbstractItemModel> keys_;
  DatabaseSource database_;
  /// The database the choice is of; empty before the keys are ready.
  QString database_id_;
  QHash<QString, Remembered> remembered_;
  QString recipient_category_;
  QString signer_category_;
  int dropped_ = 0;
  /// The last operation as stored: op, recipients, signs, signer, digest.
  QVariantMap last_;
  QStringList recipients_;           ///< in the order chosen
  QSet<QString> recipient_set_;      ///< the same, upper-cased, to look up
  int revision_ = 0;
  /// Upper-cased fingerprint to row of the key list, made when first asked
  /// and dropped whenever the list changes.
  mutable QHash<QString, int> rows_;
  /// What lookup() made of each key, by the same fingerprint, dropped with
  /// rows_: reading a key's roles is most of what a summary of hundreds of
  /// recipients cost on every tick.
  mutable QHash<QString, QVariantMap> key_info_;
  mutable bool rows_valid_ = false;
  /// RecipientKeys() as of keys_revision_.
  mutable QVariantList recipient_keys_;
  mutable int keys_revision_ = -1;

  [[nodiscard]] auto recipient_keys() const -> const QVariantList&;
  bool passphrase_mode_ = false;
  bool signing_enabled_ = false;
  QString preferred_signer_;
  bool ready_ = false;

  /// What the key list says of @p fingerprint; empty when it is not there.
  [[nodiscard]] auto lookup(const QString& fingerprint) const -> QVariantMap;
  [[nodiscard]] auto recipient_key(const QString& fingerprint) const
      -> QVariantMap;
  [[nodiscard]] auto signer_issue() const -> QString;
  [[nodiscard]] auto any_recipient_issue() const -> bool;
  /// The key database in use changed: put the choice away, take up its own.
  void follow_database();
  /// Leave out what can no longer be used; how many were.
  auto drop_unusable() -> int;
  /// Something chosen or taken out: what was dropped before is old news.
  void touched();
};

}  // namespace GpgFrontend::Mobile
