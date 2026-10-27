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

#include "KeyGenerateHelper.h"

#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/key_generate/KeyGenerateSummary.h"
#include "ui/dialog/key_generate/KeyGenerateWizard.h"
#include "ui/function/GpgErrorMessageBox.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyGenSettings.h"

namespace GpgFrontend::UI {

void AddComboSectionHeader(QComboBox* combo_box, const QString& text) {
  if (combo_box == nullptr) return;

  combo_box->addItem(text);

  auto* model = qobject_cast<QStandardItemModel*>(combo_box->model());
  if (model == nullptr) return;

  auto* item = model->item(combo_box->count() - 1);
  if (item == nullptr) return;

  item->setFlags(item->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
  item->setData(combo_box->palette().color(QPalette::Disabled, QPalette::Text),
                Qt::ForegroundRole);
  item->setData(Qt::AlignCenter, Qt::TextAlignmentRole);

  auto font = item->font();
  font.setItalic(true);
  item->setData(font, Qt::FontRole);
}

auto FirstSelectableComboIndex(const QComboBox* combo_box) -> int {
  if (combo_box == nullptr) return -1;

  const auto* model = qobject_cast<QStandardItemModel*>(combo_box->model());
  for (int i = 0; i < combo_box->count(); ++i) {
    if (model == nullptr) return i;  // no flag info: any item is selectable

    const auto* item = model->item(i);
    if (item == nullptr) continue;

    const auto flags = item->flags();
    if (flags.testFlag(Qt::ItemIsSelectable) &&
        flags.testFlag(Qt::ItemIsEnabled)) {
      return i;
    }
  }

  return -1;
}

auto GetAlgoById(const QString& id, const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Id() != id) continue;
    return {true, algo};
  }

  return {};
}

auto SearchAlgoByNameType(const QString& name, const QString& type,
                          const QList<KeyAlgo>& algos) -> QList<KeyAlgo> {
  QList<KeyAlgo> res;

  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.Type() != type) continue;
    res.append(algo);
  }

  return res;
}

auto GetAlgoByNameTypeAndKeyLength(const QString& name, const QString& type,
                                   int key_length, const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.Type() != type) continue;
    if (algo.KeyLength() != key_length) continue;
    return {true, algo};
  }

  return {};
}

auto GetAlgoByNameType(const QString& name, const QString& type,
                       const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo> {
  for (const auto& algo : algos) {
    if (algo.Name() != name) continue;
    if (algo.Type() != type) continue;
    return {true, algo};
  }

  return {};
}

void SetKeyLengthComboxBoxByAlgo(QComboBox* combo,
                                 const QList<KeyAlgo>& algos) {
  if (combo == nullptr) return;

  combo->clear();

  QSet<int> added_lengths;

  for (const auto& algo : algos) {
    const auto key_length = algo.KeyLength();
    if (key_length <= 0) continue;

    if (added_lengths.contains(key_length)) continue;
    added_lengths.insert(key_length);

    combo->addItem(QString::number(key_length), key_length);
  }
}

void PopulateAlgoComboBox(QComboBox* combo_box, const QList<KeyAlgo>& algos) {
  if (combo_box == nullptr) return;

  auto sorted_algos = algos;

  // Order the list into family tiers: "none" first, then non-ECC classical
  // algorithms, then the elliptic-curve family, then post-quantum. ECC and
  // post-quantum each get a section header so they are easy to find.
  const auto family_tier = [](const KeyAlgo& a) -> int {
    if (a.Id() == "none") return 0;
    if (a.IsPostQuantum()) return 3;
    if (a.IsEcc()) return 2;
    return 1;
  };

  std::sort(sorted_algos.begin(), sorted_algos.end(),
            [&](const KeyAlgo& a, const KeyAlgo& b) -> bool {
              if (family_tier(a) != family_tier(b)) {
                return family_tier(a) < family_tier(b);
              }

              if (a.Name() != b.Name()) return a.Name() < b.Name();
              if (a.Type() != b.Type()) return a.Type() < b.Type();
              if (a.KeyLength() != b.KeyLength()) {
                return a.KeyLength() < b.KeyLength();
              }
              return a.Id() < b.Id();
            });

  combo_box->clear();

  // One entry per name and type, the lengths in the box beside it. Labelled
  // the way the algorithm is called -- "Ed25519", not "ED25519 (EdDSA)" --
  // when the entry is one algorithm; a family of lengths keeps the family's
  // name, and says its type only where two families share a name (NIST as
  // ECDSA and as ECDH).
  QHash<QString, int> group_sizes;
  QHash<QString, QSet<QString>> types_of_name;
  for (const auto& algo : sorted_algos) {
    group_sizes[algo.Name() + u'|' + algo.Type()]++;
    types_of_name[algo.Name()].insert(algo.Type());
  }
  const auto family = [](const QString& name) -> QString {
    static const QHash<QString, QString> kFamilies{
        {u"BrainPooL"_s, u"Brainpool"_s},
        {u"Kyber"_s, u"ML-KEM"_s},
        {u"ELG-E"_s, u"ElGamal"_s},
    };
    return kFamilies.value(name, name);
  };
  const auto label_of = [&](const KeyAlgo& algo) -> QString {
    if (algo.Id() == u"none"_s) return algo.Name();
    auto label = group_sizes.value(algo.Name() + u'|' + algo.Type()) == 1
                     ? DescribeKeyAlgo(algo, KeyGenerateInfo::kNoneAlgo)
                     : family(algo.Name());
    if (types_of_name.value(algo.Name()).size() > 1) {
      label += u" ("_s + algo.Type() + u')';
    }
    return label;
  };

  QString last_label;
  QString last_type;
  bool ecc_header_added = false;
  bool pqc_header_added = false;

  for (const auto& algo : sorted_algos) {
    const auto label = label_of(algo);

    if (label == last_label && algo.Type() == last_type) continue;
    last_label = label;
    last_type = algo.Type();

    // Label the elliptic-curve block so the many curve options read as one
    // family instead of a long flat list.
    if (algo.IsEcc() && !algo.IsPostQuantum() && !ecc_header_added) {
      AddComboSectionHeader(combo_box, QObject::tr("ECC"));
      ecc_header_added = true;
    }

    // Mark where the post-quantum block begins with a section header so these
    // quantum-resistant options stand out instead of being buried in the list.
    if (algo.IsPostQuantum() && !pqc_header_added) {
      AddComboSectionHeader(combo_box, QObject::tr("Post-Quantum"));
      pqc_header_added = true;
    }

    QVariantMap data;
    data["id"] = algo.Id();
    data["name"] = algo.Name();
    data["type"] = algo.Type();
    data["length"] = algo.KeyLength();

    combo_box->addItem(label, data);
  }
}

auto GetAlgoByIdType(const QString& id, const QString& type,
                     const QList<KeyAlgo>& algos) -> std::tuple<bool, KeyAlgo> {
  QList<KeyAlgo> same_id;
  for (const auto& algo : algos) {
    if (algo.Id() != id) continue;
    if (!type.isEmpty() && algo.Type() != type) {
      same_id.append(algo);
      continue;
    }
    return {true, algo};
  }

  if (same_id.size() == 1) return {true, same_id.front()};
  return {};
}

auto ConfirmShortUserIdName(QWidget* parent, const QString& name) -> bool {
  if (name.size() >= 5) return true;

  return QMessageBox::warning(
             parent,
             QCoreApplication::translate("GpgFrontend::UI", "Short Name"),
             QCoreApplication::translate(
                 "GpgFrontend::UI",
                 "The name \"%1\" is shorter than five characters. Short names "
                 "are allowed, but they are often a typo and make the key "
                 "harder for others to recognise.\n\nDo you want to continue?")
                 .arg(name),
             QMessageBox::Yes | QMessageBox::No,
             QMessageBox::No) == QMessageBox::Yes;
}

namespace {

/// The primary key's result. A key made with a subkey carries one result per
/// key, which Check<GpgGenerateKeyResult>() refuses for its count alone.
auto FirstGenerateKeyResult(const DataObjectPtr& data_obj)
    -> std::optional<GpgGenerateKeyResult> {
  if (data_obj == nullptr || data_obj->GetObjectSize() == 0 ||
      (*data_obj)[0].type() != typeid(GpgGenerateKeyResult)) {
    return std::nullopt;
  }
  return ExtractParams<GpgGenerateKeyResult>(data_obj, 0);
}

}  // namespace

auto KeyGenerationErrorDetail(const DataObjectPtr& data_obj) -> QString {
  const auto result = FirstGenerateKeyResult(data_obj);
  return result ? result->ErrorDetail() : QString();
}

auto KeyGenerationFingerprint(const DataObjectPtr& data_obj) -> QString {
  auto result = FirstGenerateKeyResult(data_obj);
  return result ? result->GetFingerprint() : QString();
}

auto RunKeyGeneration(QWidget* parent, int channel,
                      const QSharedPointer<KeyGenerateInfo>& key,
                      const QSharedPointer<KeyGenerateInfo>& subkey)
    -> std::optional<QString> {
  // WaitForOpera() runs a nested event loop and returns only after the
  // completion callback has run, so the result is settled when it returns.
  auto result = std::make_shared<std::optional<QString>>();

  GpgOperaHelper::WaitForOpera(
      parent, QObject::tr("Generating"), [=](const OperaWaitingHd& hd) -> void {
        KeyGenerationOperation::GetInstance(channel).GenerateKeyWithSubkey(
            key, subkey, [=](GpgError err, const DataObjectPtr& data_obj) {
              hd();

              if (CheckGpgError(err) == GPG_ERR_USER_1) {
                QMessageBox::critical(parent, QObject::tr("Error"),
                                      QObject::tr("Unknown error occurred"));
                return;
              }
              if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
                RaiseFailureMessageBox(parent, err,
                                       KeyGenerationErrorDetail(data_obj));
                return;
              }

              *result = KeyGenerationFingerprint(data_obj);
              emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
            });
      });

  return *result;
}

void ShowKeyGenerated(QWidget* parent, const QString& uid,
                      const QString& fingerprint) {
  auto text =
      QObject::tr("The key for %1 was generated.").arg(uid.toHtmlEscaped());
  if (!fingerprint.isEmpty()) {
    text +=
        u"<br><br>"_s +
        QObject::tr("Fingerprint: %1")
            .arg(u"<code>%1</code>"_s.arg(BeautifyFingerprint(fingerprint)));
  }

  QMessageBox box(QMessageBox::Information, QObject::tr("Key Generated"), text,
                  QMessageBox::Ok, parent);
  box.setTextFormat(Qt::RichText);
  box.setTextInteractionFlags(Qt::TextSelectableByMouse);
  box.exec();
}

void QuickGenerateKey(int channel, QWidget* parent) {
  const auto settings = LoadKeyGenSettings();
  const auto offered = OfferedKeyGenProfiles(channel, settings.profiles);
  const auto index =
      PickDefaultKeyGenProfile(offered, settings.default_profile);

  const auto check = CheckUidPattern(settings.uid_pattern);

  // The default profile named in Settings, not merely one that will do: a
  // one-click key is only safe to make when it is the key the user asked for.
  const auto named_missing =
      !settings.default_profile.isEmpty() &&
      (index < 0 || offered.at(index).profile.name.compare(
                        settings.default_profile, Qt::CaseInsensitive) != 0);

  QString problem;
  if (index < 0 || named_missing) {
    problem = QObject::tr(
        "The default key generation profile cannot be generated in this key "
        "database. Choose another one in Settings, under Key Generation.");
  } else if (!check.Ok()) {
    problem = QObject::tr(
        "The user ID pattern in Settings, under Key Generation, does not make "
        "a valid user ID.");
  }

  if (!problem.isEmpty()) {
    const auto answer = QMessageBox::warning(
        parent, QObject::tr("Quick Generate Key"),
        problem + u"\n\n"_s +
            QObject::tr(
                "Do you want to generate the key step by step instead?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer == QMessageBox::Yes) new KeyGenerateWizard(channel, parent);
    return;
  }

  const auto& resolved = offered.at(index);
  const auto uid = ExpandUidPattern(
      settings.uid_pattern, SystemUidPatternRandom(), CurrentDateTime());

  auto key = SecureCreateSharedObject<KeyGenerateInfo>();
  QSharedPointer<KeyGenerateInfo> subkey;
  ApplyKeyGenProfile(resolved, CurrentDateTime(), *key, subkey);
  key->SetName(uid.name);
  key->SetEmail(uid.email);
  key->SetComment(uid.comment);

  // Post-quantum algorithms only exist in the v6 format; anything else keeps
  // the interoperable v4 one, as the wizard does by default.
  const auto version = RequiredKeyVersion(*key, subkey);
  key->SetKeyVersion(version != 0 ? version : 4);

  const auto fingerprint = RunKeyGeneration(parent, channel, key, subkey);
  if (!fingerprint) return;
  ShowKeyGenerated(parent, key->GetUserid(), *fingerprint);
}

auto NormalizeKeyAlgoId(const QString& engine_name) -> QString {
  auto id = engine_name.toLower();
  id.remove(u' ').remove(u'-').remove(u'_');

  // A hybrid is named after both halves; the generation id is the first.
  for (const auto& hybrid : {u"mldsa65"_s, u"mldsa87"_s}) {
    if (id.startsWith(hybrid)) return hybrid;
  }
  // GnuPG says "brainpoolp256r1", rPGP "Brainpool P-256".
  if (id.startsWith(u"brainpoolp"_s) && !id.endsWith(u"r1"_s)) id += u"r1"_s;
  return id;
}

auto SuggestSubkeyAlgo(SubkeyPurpose purpose, const QString& primary,
                       const QList<KeyAlgo>& supported) -> KeyAlgo {
  const auto can = [purpose](const KeyAlgo& a) {
    switch (purpose) {
      case SubkeyPurpose::kEncrypt:
        return a.CanEncrypt();
      case SubkeyPurpose::kSign:
        return a.CanSign();
      case SubkeyPurpose::kAuthenticate:
        return a.CanAuth();
    }
    return false;
  };

  // The primary key's own family, in order of preference. A curve keeps its
  // id across ECDSA and ECDH, so the capability decides which of the two.
  QStringList family;
  const auto encrypt = purpose == SubkeyPurpose::kEncrypt;
  if (primary == u"ed25519"_s) {
    family << (encrypt ? u"cv25519"_s : u"ed25519"_s);
  } else if (primary == u"ed448"_s) {
    family << (encrypt ? u"x448"_s : u"ed448"_s);
  } else if (primary.startsWith(u"dsa"_s)) {
    family << (encrypt ? u"elg"_s + primary.mid(3) : primary);
  } else if (primary == u"mldsa65"_s ||
             primary.startsWith(u"slhdsashake128"_s)) {
    family << (encrypt ? u"ky768"_s : primary) << u"ed25519"_s << u"cv25519"_s;
  } else if (primary == u"mldsa87"_s ||
             primary.startsWith(u"slhdsashake256"_s)) {
    family << (encrypt ? u"kyber1024"_s : primary) << u"ed448"_s << u"x448"_s;
  } else {
    // RSA and the named curves: the same id does both jobs.
    family << primary;
  }

  for (const auto& id : family) {
    for (const auto& algo : supported) {
      if (algo.Id() == id && can(algo)) return algo;
    }
  }

  for (const auto& algo : supported) {
    if (algo != KeyGenerateInfo::kNoneAlgo && can(algo)) return algo;
  }
  return KeyGenerateInfo::kNoneAlgo;
}

}  // namespace GpgFrontend::UI
