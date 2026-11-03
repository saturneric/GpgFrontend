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

#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgVerifyResult.h"

namespace GpgFrontend::Mobile {

auto SelectKeys(int channel, const QStringList& fingerprints) -> KeySelection {
  KeySelection selection;
  for (const auto& fpr : fingerprints) {
    auto key = GpgKeyRepository::GetInstance(channel).GetKeyPtr(fpr.toUpper());
    if (key != nullptr) {
      selection.keys.append(key);
    } else {
      selection.missing.append(fpr);
    }
  }
  return selection;
}

auto MissingKeysOutcome(GpgOperation op, int channel,
                        const QStringList& missing, bool secret)
    -> GpgOperaOutcome {
  const auto detail =
      QCoreApplication::translate("GpgFrontend::Mobile::KeySelection",
                                  "No longer in the keyring: %1")
          .arg(missing.join(QStringLiteral(", ")));
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

template <bool kWithOutput>
auto AnalyseDecryptVerify(int channel, const GFError& err, const DataObjectPtr& data)
    -> GpgOperaOutcome {
  const bool unsigned_message =
      detail::HoldsOperaResults<kWithOutput, GpgDecryptResult, GpgVerifyResult>(
          data) &&
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

template auto AnalyseDecryptVerify<true>(int, const GFError&, const DataObjectPtr&)
    -> GpgOperaOutcome;
template auto AnalyseDecryptVerify<false>(int, const GFError&, const DataObjectPtr&)
    -> GpgOperaOutcome;

}  // namespace GpgFrontend::Mobile
