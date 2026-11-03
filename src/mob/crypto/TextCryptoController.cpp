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

#include "crypto/TextCryptoController.h"

#include <QPointer>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgOperaOutcome.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/MemoryUtils.h"
#include "crypto/KeySelection.h"
#include "crypto/ResultPresenter.h"
#include "plat/common/PlatformPrivacy.h"

namespace GpgFrontend::Mobile {

namespace {


}  // namespace

TextCryptoController::TextCryptoController(QObject* parent)
    : QObject(parent) {}

TextCryptoController::~TextCryptoController() = default;

auto TextCryptoController::Busy() const -> bool { return busy_; }

auto TextCryptoController::Output() const -> QString { return output_; }

auto TextCryptoController::Result() const -> QVariantMap { return result_; }

auto TextCryptoController::Sensitive() const -> bool {
  return secure_ != nullptr;
}

void TextCryptoController::encrypt(const QString& text,
                                   const QStringList& recipients,
                                   const QString& signer) {
  if (!begin()) return;
  const auto channel = UsableDefaultChannelHandle();
  auto& opera = MessageCryptoOperation::GetInstance(channel);
  QPointer<TextCryptoController> self(this);

  if (recipients.isEmpty()) {
    opera.EncryptSymmetric(
        GFBuffer(text), true,
        [self, channel](GFError err, DataObjectPtr data) {
          if (self == nullptr) return;
          self->finish(
              AnalyseOperaOutcome<GpgEncryptResult, GpgEncryptResultAnalyse>(
                  channel, err, data));
        });
    return;
  }

  const auto chosen = SelectKeys(channel, recipients);
  if (!chosen.missing.isEmpty()) {
    finish(MissingKeysOutcome(GpgOperation::kENCRYPT, channel, chosen.missing));
    return;
  }
  const auto keys = chosen.keys;
  if (signer.isEmpty()) {
    opera.Encrypt(
        keys, GFBuffer(text), true,
        [self, channel, keys](GFError err, DataObjectPtr data) {
          if (self == nullptr) return;
          auto outcome =
              AnalyseOperaOutcome<GpgEncryptResult, GpgEncryptResultAnalyse>(
                  channel, err, data);
          AttachRecipients(outcome, data, keys);
          self->finish(outcome);
        });
    return;
  }

  const auto signers = SelectKeys(channel, {signer});
  if (!signers.missing.isEmpty()) {
    finish(MissingKeysOutcome(GpgOperation::kSIGN, channel, signers.missing,
                              true));
    return;
  }
  opera.EncryptSign(
      keys, signers.keys, GFBuffer(text), true,
      [self, channel, keys](GFError err, DataObjectPtr data) {
        if (self == nullptr) return;
        auto outcome =
            AnalyseOperaOutcome<GpgEncryptResult, GpgEncryptResultAnalyse,
                                GpgSignResult, GpgSignResultAnalyse>(channel,
                                                                     err, data);
        AttachRecipients(outcome, data, keys);
        self->finish(outcome);
      });
}

void TextCryptoController::decrypt(const QString& text) {
  if (!begin()) return;
  const auto channel = UsableDefaultChannelHandle();
  QPointer<TextCryptoController> self(this);
  MessageCryptoOperation::GetInstance(channel).DecryptVerify(
      GFBuffer(text), [self, channel](GFError err, DataObjectPtr data) {
        if (self == nullptr) return;
        self->finish(AnalyseDecryptVerify<true>(channel, err, data), true);
      });
}

void TextCryptoController::sign(const QString& text, const QString& signer) {
  if (!begin()) return;
  const auto channel = UsableDefaultChannelHandle();
  const auto signers = SelectKeys(channel, {signer});
  if (!signers.missing.isEmpty()) {
    finish(MissingKeysOutcome(GpgOperation::kSIGN, channel, signers.missing,
                              true));
    return;
  }
  QPointer<TextCryptoController> self(this);
  MessageCryptoOperation::GetInstance(channel).Sign(
      signers.keys, GFBuffer(text), GpgSignMode::kCLEAR, true,
      [self, channel](GFError err, DataObjectPtr data) {
        if (self == nullptr) return;
        self->finish(AnalyseOperaOutcome<GpgSignResult, GpgSignResultAnalyse>(
            channel, err, data));
      });
}

void TextCryptoController::verify(const QString& text) {
  if (!begin()) return;
  const auto channel = UsableDefaultChannelHandle();
  QPointer<TextCryptoController> self(this);
  MessageCryptoOperation::GetInstance(channel).Verify(
      GFBuffer(text), GFBuffer(),
      [self, channel](GFError err, DataObjectPtr data) {
        if (self == nullptr) return;
        self->finish(
            AnalyseOperaOutcome<GpgVerifyResult, GpgVerifyResultAnalyse>(
                channel, err, data));
      });
}

auto TextCryptoController::begin() -> bool {
  if (busy_) return false;
  busy_ = true;
  emit SignalBusyChanged();
  return true;
}

void TextCryptoController::clear() {
  if (busy_) return;
  WipeString(output_);
  output_.clear();
  result_.clear();
  secure_.reset();
  emit SignalOutputChanged();
}

void TextCryptoController::finish(const GpgOperaOutcome& outcome,
                                  bool sensitive) {
  WipeString(output_);
  output_ = outcome.output && !outcome.output->Empty()
                ? QString::fromUtf8(outcome.output->ConvertToQByteArray())
                : QString();
  result_ = PresentResult(outcome.info);
  // Once plaintext has been on screen it stays protected until clear(): the
  // page writes each result into its one editor, and a later sign, verify or
  // encrypt there -- or Undo -- leaves the plaintext in view or in reach.
  if (sensitive && !output_.isEmpty() && secure_ == nullptr) {
    secure_ = std::make_unique<SecureWindowScope>(PrimaryWindow());
  }
  busy_ = false;
  emit SignalOutputChanged();
  emit SignalBusyChanged();
  emit SignalFinished(outcome.info.status >= 0);
}

}  // namespace GpgFrontend::Mobile
