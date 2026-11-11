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

#include "crypto/KeySelection.h"

#include <QCoreApplication>
#include <typeindex>

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "shell/result_analyse/GpgDecryptResultAnalyse.h"
#include "shell/result_analyse/GpgEncryptResultAnalyse.h"
#include "shell/result_analyse/GpgSignResultAnalyse.h"
#include "shell/result_analyse/GpgVerifyResultAnalyse.h"

namespace GpgFrontend::Mobile {

auto SelectKeys(int channel, const QStringList& fingerprints) -> KeySelection {
  KeySelection selection;
  QSet<QString> taken;  ///< fingerprints in, so a key goes in once
  const auto take = [&selection, &taken](const GpgAbstractKeyPtr& key) {
    if (taken.contains(key->Fingerprint().toUpper())) return;
    taken.insert(key->Fingerprint().toUpper());
    selection.keys.append(key);
  };
  for (const auto& fpr : fingerprints) {
    if (IsKeyGroupID(fpr)) {
      // A key group stands for its keys, nested groups' too, as the engine
      // would expand it (Convert2GpgKeyList): so the result names them, and
      // a key chosen twice goes in once. A group with none is as gone.
      const auto group =
          AbstractKeyRepository::GetInstance(channel).GetKey(fpr);
      const auto members = group == nullptr
                               ? GpgKeyPtrList{}
                               : ConvertKey2GpgKeyList(channel, {group});
      if (members.isEmpty()) {
        selection.missing.append(fpr);
        continue;
      }
      // A member gone or disabled is left out of the expansion without a
      // word: the group goes to the engine as it is instead, which refuses
      // it naming those members (UnusableRecipientsResult), as the desktop.
      if (!UnusableKeyIds(channel, {group}).isEmpty()) {
        take(group);
        continue;
      }
      for (const auto& member : members) take(member);
      continue;
    }
    // A subkey pinned for the operation ("<fingerprint>!", as the desktop's
    // pickers hand one on) stands alone.
    auto key =
        fpr.endsWith(QLatin1Char('!'))
            ? AbstractKeyRepository::GetInstance(channel).GetKey(fpr.toUpper())
            : GpgAbstractKeyPtr(
                  GpgKeyRepository::GetInstance(channel).GetKeyPtr(
                      fpr.toUpper()));
    if (key != nullptr) {
      take(key);
    } else {
      selection.missing.append(fpr);
    }
  }
  return selection;
}

auto MissingKeysOutcome(GpgOperation op, int channel,
                        const QStringList& missing, bool secret)
    -> GpgOperaOutcome {
  // In the words the key lists use for a key that has gone, then which.
  const auto detail = QStringLiteral("%1: %2").arg(
      QCoreApplication::translate("Wording", "No longer in your keys"),
      missing.join(QStringLiteral(", ")));
  return {MakeAbortedOpInfo(
              op,
              GFError::Application(secret ? GFErrorCode::kNO_SECRET_KEY
                                          : GFErrorCode::kNO_PUBLIC_KEY),
              DescribeOpenPGPEngine(channel), {{op, detail}}),
          std::nullopt};
}

void AttachRecipients(GpgOperaOutcome& outcome, const DataObjectPtr& data,
                      const GpgAbstractKeyPtrList& keys) {
  // The encryption result comes first, the output (if any) after it.
  if (outcome.info.status < 0 || data == nullptr ||
      data->GetObjectSize() == 0 ||
      std::type_index((*data)[0].type()) !=
          std::type_index(typeid(GpgEncryptResult))) {
    return;
  }
  AttachEncryptRecipients(outcome.info,
                          ExtractParams<GpgEncryptResult>(data, 0), keys);
}

auto SelectEncryptKeys(int channel, const QStringList& recipients,
                       const QString& signer)
    -> std::variant<EncryptKeys, GpgOperaOutcome> {
  const auto chosen = SelectKeys(channel, recipients);
  if (!chosen.missing.isEmpty()) {
    return MissingKeysOutcome(GpgOperation::kENCRYPT, channel, chosen.missing);
  }
  if (signer.isEmpty()) return EncryptKeys{chosen.keys, {}};
  const auto signers = SelectKeys(channel, {signer});
  if (!signers.missing.isEmpty()) {
    return MissingKeysOutcome(GpgOperation::kSIGN, channel, signers.missing,
                              true);
  }
  return EncryptKeys{chosen.keys, signers.keys};
}

template <bool kWithOutput>
auto AnalyseEncrypt(int channel, const GFError& err, const DataObjectPtr& data,
                    const GpgAbstractKeyPtrList& recipients, bool signs)
    -> GpgOperaOutcome {
  auto outcome =
      signs ? AnalyseOperaOutcome<GpgEncryptResult, GpgEncryptResultAnalyse,
                                  GpgSignResult, GpgSignResultAnalyse,
                                  kWithOutput>(channel, err, data)
            : AnalyseOperaOutcome<GpgEncryptResult, GpgEncryptResultAnalyse,
                                  kWithOutput>(channel, err, data);
  // No keys: said to be for a passphrase.
  AttachRecipients(outcome, data, recipients);
  return outcome;
}

template auto AnalyseEncrypt<true>(int, const GFError&, const DataObjectPtr&,
                                   const GpgAbstractKeyPtrList&, bool)
    -> GpgOperaOutcome;
template auto AnalyseEncrypt<false>(int, const GFError&, const DataObjectPtr&,
                                    const GpgAbstractKeyPtrList&, bool)
    -> GpgOperaOutcome;

template <bool kWithOutput>
auto AnalyseDecryptVerify(int channel, const GFError& err, const DataObjectPtr& data)
    -> GpgOperaOutcome {
  const bool unsigned_message =
      HoldsOperaResults<kWithOutput, GpgDecryptResult, GpgVerifyResult>(data) &&
      ExtractParams<GpgVerifyResult>(data, 1).GetSignature().isEmpty();
  if (!unsigned_message) {
    return AnalyseOperaOutcome<GpgDecryptResult, GpgDecryptResultAnalyse,
                               GpgVerifyResult, GpgVerifyResultAnalyse,
                               kWithOutput>(channel, err, data);
  }

  auto decrypt_only = TransferParams(ExtractParams<GpgDecryptResult>(data, 0));
  if constexpr (kWithOutput) {
    decrypt_only = TransferParams(ExtractParams<GpgDecryptResult>(data, 0),
                                  ExtractParams<GFBuffer>(data, 2));
  }
  return AnalyseOperaOutcome<GpgDecryptResult, GpgDecryptResultAnalyse,
                             kWithOutput>(channel, err, decrypt_only);
}

template <bool kWithOutput>
auto AnalyseDecrypt(int channel, const GFError& err, const DataObjectPtr& data)
    -> GpgOperaOutcome {
  return AnalyseOperaOutcome<GpgDecryptResult, GpgDecryptResultAnalyse,
                             kWithOutput>(channel, err, data);
}

template auto AnalyseDecrypt<true>(int, const GFError&, const DataObjectPtr&)
    -> GpgOperaOutcome;
template auto AnalyseDecrypt<false>(int, const GFError&, const DataObjectPtr&)
    -> GpgOperaOutcome;

template auto AnalyseDecryptVerify<true>(int, const GFError&, const DataObjectPtr&)
    -> GpgOperaOutcome;
template auto AnalyseDecryptVerify<false>(int, const GFError&, const DataObjectPtr&)
    -> GpgOperaOutcome;

}  // namespace GpgFrontend::Mobile
