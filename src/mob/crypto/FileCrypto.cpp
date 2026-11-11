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

#include "crypto/FileCrypto.h"

#include <QPointer>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "crypto/KeySelection.h"
#include "shell/result_analyse/GpgDecryptResultAnalyse.h"
#include "shell/result_analyse/GpgEncryptResultAnalyse.h"
#include "shell/result_analyse/GpgOperaOutcome.h"
#include "shell/result_analyse/GpgSignResultAnalyse.h"
#include "shell/result_analyse/GpgVerifyResultAnalyse.h"

namespace GpgFrontend::Mobile {

namespace {

/// @p done, unless @p context has gone in the meantime.
auto Guarded(QObject* context, OutcomeCallback done) {
  return [guard = QPointer<QObject>(context),
          done = std::move(done)](const GpgOperaOutcome& outcome) {
    if (guard != nullptr) done(outcome);
  };
}

}  // namespace

void EncryptStagedFile(const QString& in, const QString& out,
                       const QStringList& recipients, const QString& signer,
                       QObject* context, OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  auto& opera = FileCryptoOperation::GetInstance(channel);

  if (recipients.isEmpty()) {
    opera.EncryptFileSymmetric(
        in, false, out, [finish, channel](GFError err, DataObjectPtr data) {
          finish(AnalyseEncrypt<false>(channel, err, data, {}, false));
        });
    return;
  }

  const auto selected = SelectEncryptKeys(channel, recipients, signer);
  if (const auto* missing = std::get_if<GpgOperaOutcome>(&selected)) {
    finish(*missing);
    return;
  }
  const auto keys = std::get<EncryptKeys>(selected);
  const auto signs = !keys.signers.isEmpty();
  auto analysed = [finish, channel, keys, signs](GFError err,
                                                 DataObjectPtr data) {
    finish(AnalyseEncrypt<false>(channel, err, data, keys.recipients, signs));
  };
  if (signs) {
    opera.EncryptSignFile(keys.recipients, keys.signers, in, false, out,
                          std::move(analysed));
  } else {
    opera.EncryptFile(keys.recipients, in, false, out, std::move(analysed));
  }
}

void DecryptStagedFile(const QString& in, const QString& out, bool verify,
                       QObject* context, OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  if (!verify) {
    FileCryptoOperation::GetInstance(channel).DecryptFile(
        in, out, [finish, channel](GFError err, DataObjectPtr data) {
          finish(AnalyseDecrypt<false>(channel, err, data));
        });
    return;
  }
  FileCryptoOperation::GetInstance(channel).DecryptVerifyFile(
      in, out, [finish, channel](GFError err, DataObjectPtr data) {
        finish(AnalyseDecryptVerify<false>(channel, err, data));
      });
}

void SignStagedFile(const QString& in, const QString& out,
                    const QString& signer, QObject* context,
                    OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  const auto signers = SelectKeys(channel, {signer});
  if (!signers.missing.isEmpty()) {
    finish(MissingKeysOutcome(GpgOperation::kSIGN, channel, signers.missing,
                              true));
    return;
  }
  // Binary, as the desktop signs files: a .sig beside the file.
  FileCryptoOperation::GetInstance(channel).SignFile(
      signers.keys, in, false, out,
      [finish, channel](GFError err, DataObjectPtr data) {
        finish(AnalyseOperaOutcome<GpgSignResult, GpgSignResultAnalyse, false>(
            channel, err, data));
      });
}

void VerifyStagedFile(const QString& data, const QString& signature,
                      QObject* context, OutcomeCallback done) {
  auto finish = Guarded(context, std::move(done));
  const auto channel = UsableDefaultChannelHandle();
  FileCryptoOperation::GetInstance(channel).VerifyFile(
      data, signature, [finish, channel](GFError err, DataObjectPtr result) {
        finish(
            AnalyseOperaOutcome<GpgVerifyResult, GpgVerifyResultAnalyse, false>(
                channel, err, result));
      });
}

}  // namespace GpgFrontend::Mobile
