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
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/MemoryUtils.h"
#include "crypto/KeySelection.h"
#include "crypto/OperationQueue.h"
#include "crypto/ResultPresenter.h"
#include "plat/common/PlatformPrivacy.h"
#include "shell/result_analyse/GpgDecryptResultAnalyse.h"
#include "shell/result_analyse/GpgEncryptResultAnalyse.h"
#include "shell/result_analyse/GpgOperaOutcome.h"
#include "shell/result_analyse/GpgSignResultAnalyse.h"
#include "shell/result_analyse/GpgVerifyResultAnalyse.h"

namespace GpgFrontend::Mobile {

TextCryptoController::TextCryptoController(QObject* parent) : QObject(parent) {
  if (auto* queue = OperationQueue::Instance(); queue != nullptr) {
    connect(queue, &OperationQueue::SignalFinished, this,
            &TextCryptoController::dropped);
  }
}

TextCryptoController::~TextCryptoController() {
  // Before it is gone: what the queue then says must not reach it.
  if (auto* queue = OperationQueue::Instance(); queue != nullptr) {
    disconnect(queue, nullptr, this, nullptr);
    queue->ForgetOwner(this);
  }
}

auto TextCryptoController::Busy() const -> bool { return busy_; }

auto TextCryptoController::Output() const -> QString { return output_; }

auto TextCryptoController::Result() const -> QVariantMap { return result_; }

auto TextCryptoController::OperationId() const -> int { return submitted_id_; }

auto TextCryptoController::FinishedId() const -> int { return finished_id_; }

auto TextCryptoController::ReplacesText() const -> bool {
  return replaces_text_;
}

auto TextCryptoController::Sensitive() const -> bool {
  return secure_ != nullptr;
}

auto TextCryptoController::encrypt(const QString& text,
                                   const QStringList& recipients,
                                   const QString& signer, bool symmetric)
    -> bool {
  if (!symmetric && recipients.isEmpty()) return false;
  OperationParams params;
  params.op = QStringLiteral("encrypt");
  params.symmetric = symmetric;
  if (!symmetric) {
    params.recipients = recipients;
    params.signer = signer;
  }
  const auto to = params.recipients;
  const auto by = params.signer;
  return submit(std::move(params), text,
                [this, text, to, by]() { run_encrypt(text, to, by); });
}

void TextCryptoController::run_encrypt(const QString& text,
                                       const QStringList& recipients,
                                       const QString& signer) {
  const auto channel = UsableDefaultChannelHandle();
  auto& opera = MessageCryptoOperation::GetInstance(channel);
  QPointer<TextCryptoController> self(this);

  if (recipients.isEmpty()) {
    opera.EncryptSymmetric(
        GFBuffer(text), true, [self, channel](GFError err, DataObjectPtr data) {
          if (self == nullptr) return;
          self->finish(AnalyseEncrypt<true>(channel, err, data, {}, false),
                       true);
        });
    return;
  }

  const auto selected = SelectEncryptKeys(channel, recipients, signer);
  if (const auto* missing = std::get_if<GpgOperaOutcome>(&selected)) {
    finish(*missing, true);
    return;
  }
  const auto keys = std::get<EncryptKeys>(selected);
  const auto signs = !keys.signers.isEmpty();
  auto done = [self, channel, keys, signs](GFError err, DataObjectPtr data) {
    if (self == nullptr) return;
    self->finish(
        AnalyseEncrypt<true>(channel, err, data, keys.recipients, signs), true);
  };
  if (signs) {
    opera.EncryptSign(keys.recipients, keys.signers, GFBuffer(text), true,
                      std::move(done));
  } else {
    opera.Encrypt(keys.recipients, GFBuffer(text), true, std::move(done));
  }
}

auto TextCryptoController::decrypt(const QString& text, bool verify) -> bool {
  OperationParams params;
  params.op = QStringLiteral("decrypt");
  params.verify = verify;
  return submit(std::move(params), text, [this, text, verify]() {
    const auto channel = UsableDefaultChannelHandle();
    QPointer<TextCryptoController> self(this);
    if (!verify) {
      MessageCryptoOperation::GetInstance(channel).Decrypt(
          GFBuffer(text), [self, channel](GFError err, DataObjectPtr data) {
            if (self == nullptr) return;
            self->finish(AnalyseDecrypt<true>(channel, err, data), true, true);
          });
      return;
    }
    MessageCryptoOperation::GetInstance(channel).DecryptVerify(
        GFBuffer(text), [self, channel](GFError err, DataObjectPtr data) {
          if (self == nullptr) return;
          self->finish(AnalyseDecryptVerify<true>(channel, err, data), true,
                       true);
        });
  });
}

auto TextCryptoController::sign(const QString& text, const QString& signer)
    -> bool {
  OperationParams params;
  params.op = QStringLiteral("sign");
  params.signer = signer;
  return submit(std::move(params), text, [this, text, signer]() {
    const auto channel = UsableDefaultChannelHandle();
    const auto signers = SelectKeys(channel, {signer});
    if (!signers.missing.isEmpty()) {
      finish(MissingKeysOutcome(GpgOperation::kSIGN, channel, signers.missing,
                                true),
             true);
      return;
    }
    QPointer<TextCryptoController> self(this);
    MessageCryptoOperation::GetInstance(channel).Sign(
        signers.keys, GFBuffer(text), GpgSignMode::kCLEAR, true,
        [self, channel](GFError err, DataObjectPtr data) {
          if (self == nullptr) return;
          self->finish(AnalyseOperaOutcome<GpgSignResult, GpgSignResultAnalyse>(
                           channel, err, data),
                       true);
        });
  });
}

auto TextCryptoController::verify(const QString& text) -> bool {
  OperationParams params;
  params.op = QStringLiteral("verify");
  return submit(std::move(params), text, [this, text]() {
    const auto channel = UsableDefaultChannelHandle();
    QPointer<TextCryptoController> self(this);
    MessageCryptoOperation::GetInstance(channel).Verify(
        GFBuffer(text), GFBuffer(),
        [self, channel](GFError err, DataObjectPtr data) {
          if (self == nullptr) return;
          self->finish(
              AnalyseOperaOutcome<GpgVerifyResult, GpgVerifyResultAnalyse>(
                  channel, err, data),
              false);
        });
  });
}

auto TextCryptoController::submit(OperationParams params, const QString& text,
                                  std::function<void()> work) -> bool {
  auto* queue = OperationQueue::Instance();
  if (busy_ || queue == nullptr) return false;
  busy_ = true;
  emit SignalBusyChanged();
  if (&text != &input_) {
    WipeString(input_);
    input_ = text;
  }

  OperationQueue::Request request;
  request.owner = this;
  request.surface = OperationSurface::kTEXT;
  request.params = std::move(params);
  request.targets = {OperationTarget{-1, {}, TextVersion(text)}};
  request.run = [this, work = std::move(work)](int id) {
    running_id_ = id;
    work();
  };
  // It may run, and even end, before Submit() returns.
  submitted_id_ = queue->Submit(std::move(request));
  if (submitted_id_ == 0 && busy_) {
    busy_ = false;
    emit SignalBusyChanged();
    return false;
  }
  // operationId is known only now: what was said while it ran named the
  // one before.
  emit SignalBusyChanged();
  return true;
}

void TextCryptoController::dropped(int id) {
  if (id != submitted_id_ || id == running_id_ || !busy_) return;
  WipeString(output_);
  output_.clear();
  result_ = {{QStringLiteral("cancelled"), true}};
  replaces_text_ = false;
  finished_id_ = id;
  busy_ = false;
  emit SignalOutputChanged();
  emit SignalBusyChanged();
  emit SignalFinished(false);
}

auto TextCryptoController::holds(int id) const -> bool {
  return id != 0 && id == submitted_id_ && !busy_ && !input_.isEmpty();
}

auto TextCryptoController::retry(int id) -> bool {
  auto* queue = OperationQueue::Instance();
  if (!holds(id) || queue == nullptr) return false;
  const auto* record = queue->Log()->Find(id);
  if (record == nullptr) return false;
  const auto params = record->params;
  // A copy: the operation replaces what is held as it starts.
  const auto text = input_;
  if (params.op == QStringLiteral("encrypt")) {
    return encrypt(text, params.recipients, params.signer, params.symmetric);
  }
  if (params.op == QStringLiteral("decrypt")) {
    return decrypt(text, params.verify);
  }
  if (params.op == QStringLiteral("sign")) return sign(text, params.signer);
  if (params.op == QStringLiteral("verify")) return verify(text);
  return false;
}

void TextCryptoController::clear() {
  for (auto& kept : held_) WipeString(kept);
  held_.clear();
  // The text of an operation still running is its own until it ends.
  if (busy_) return;
  WipeString(output_);
  output_.clear();
  WipeString(input_);
  input_.clear();
  result_.clear();
  replaces_text_ = false;
  secure_.reset();
  emit SignalOutputChanged();
}

auto TextCryptoController::hold(int id) -> bool {
  if (id == 0 || id != finished_id_ || !replaces_text_) return false;
  held_.insert(id, output_);
  return true;
}

auto TextCryptoController::isHeld(int id) const -> bool {
  return held_.contains(id);
}

auto TextCryptoController::take(int id) -> QString { return held_.take(id); }

void TextCryptoController::release(int id) {
  auto it = held_.find(id);
  if (it == held_.end()) return;
  WipeString(*it);
  held_.erase(it);
}

void TextCryptoController::finish(const GpgOperaOutcome& outcome,
                                  bool makes_text, bool sensitive) {
  WipeString(output_);
  output_ = outcome.output && !outcome.output->Empty()
                ? QString::fromUtf8(outcome.output->ConvertToQByteArray())
                : QString();
  result_ = PresentResult(outcome.info);
  // Said by the operation and how it ended, never guessed from the text:
  // the core releases an output only when making it worked
  // (GpgOpResultInfo::OutputReleasable()) -- a message whose signature
  // fails still decrypts -- and an empty message decrypts to nothing, which
  // is its text.
  replaces_text_ = makes_text && outcome.output.has_value() &&
                   !result_.value(QStringLiteral("cancelled")).toBool();
  // Once plaintext has been on screen it stays protected until clear(): the
  // page writes each result into its one editor, and a later sign, verify or
  // encrypt there -- or Undo -- leaves the plaintext in view or in reach.
  if (sensitive && !output_.isEmpty() && secure_ == nullptr) {
    secure_ = std::make_unique<SecureWindowScope>(PrimaryWindow());
  }
  busy_ = false;
  const auto id = running_id_;
  running_id_ = 0;
  finished_id_ = id;
  auto* queue = id != 0 ? OperationQueue::Instance() : nullptr;
  const auto phase =
      EndedPhase(result_.value(QStringLiteral("cancelled")).toBool(),
                 outcome.info.status >= 0);
  // Recorded before it is announced, so a page that reads the record on
  // the signals below finds it ended.
  if (queue != nullptr) {
    if (!output_.isEmpty()) {
      queue->Log()->SetOutputVersion(id, TextVersion(output_));
    }
    queue->Log()->SetFinished(id, phase, {}, result_, {});
  }
  emit SignalOutputChanged();
  emit SignalBusyChanged();
  emit SignalFinished(outcome.info.status >= 0);

  // Last: the next operation in the queue starts here, and one submitted
  // from the signals above goes after this one.
  if (queue != nullptr) queue->Complete(id, phase, result_);
}

}  // namespace GpgFrontend::Mobile
