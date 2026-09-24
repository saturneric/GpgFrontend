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

#include "CodecStage.h"

#include <QPointer>
#include <QTimer>

#include "sdk/GFSDKHostCommands.hpp"
#include "ui/command/CommandRegistry.h"

namespace GpgFrontend::UI {

namespace {

using gf::cmd::host::CodecOutcome;

/**
 * @brief One stage in flight. Lives on the thread that started it.
 *
 * Every step runs there: a registry completion, which may arrive on any
 * thread, is posted back to this object first. An attempt number, not the
 * call id, tells a current answer from a stale one, because a completion may
 * be queued before the Invoke() that caused it has even returned its ticket.
 */
class Run : public QObject {
 public:
  Run(QStringList ids, GFBuffer input, bool required, int timeout_ms,
      CodecStage::Done done)
      : ids_(std::move(ids)), input_(std::move(input)), required_(required),
        timeout_ms_(timeout_ms), done_(std::move(done)) {}

  void Next() {
    if (index_ >= ids_.size()) {
      if (required_) {
        return Finish(Failed(QStringLiteral("the encoder could not be used")));
      }
      return Finish({});
    }
    const auto id = ids_[index_++];
    const auto attempt = ++attempt_;

    gf::cmd::EncodeState st;
    const auto args = gf::cmd::EncodeMap(
        gf::cmd::host::CodecArgs{MakeHostBlob(input_)}, st);

    QPointer<Run> self(this);
    const auto ticket = CommandRegistry::Instance().Invoke(
        id, args, std::move(st.blobs), CommandCaller{{}, 0, "host"}, {},
        [self, attempt, id](gf::cmd::RawResult r) {
          if (self.isNull()) return;
          QMetaObject::invokeMethod(
              self.data(),
              [self, attempt, id, r = std::move(r)]() mutable {
                if (!self.isNull()) self->OnAnswer(attempt, id, std::move(r));
              },
              Qt::QueuedConnection);
        });
    if (ticket.status != GF_CMD_OK) {
      LOG_W() << "codec" << id << "could not be called:" << ticket.status;
      return Next();
    }
    call_id_ = ticket.call_id;
    QTimer::singleShot(timeout_ms_, this, [this, attempt, id]() {
      if (attempt != attempt_ || finished_) return;
      LOG_W() << "codec" << id << "took longer than" << timeout_ms_
              << "ms; carrying on without it";
      // After Cancel() returns the call never completes; an answer already
      // queued is stale by its attempt number.
      CommandRegistry::Instance().Cancel(call_id_, {});
      ++attempt_;
      Next();
    });
  }

 private:
  static auto Failed(QString why) -> CodecStageResult {
    CodecStageResult r;
    r.kind = CodecStageResult::Kind::kFailed;
    r.error = std::move(why);
    return r;
  }

  void OnAnswer(quint64 attempt, const QString& id, gf::cmd::RawResult r) {
    if (attempt != attempt_ || finished_) return;
    ++attempt_;  // this call is answered; its timer is now stale

    gf::cmd::host::CodecResult result;
    QString error;
    if (r.status != GF_CMD_OK) {
      LOG_W() << "codec" << id << "is unavailable:" << r.status << r.error;
      return Next();
    }
    if (!gf::cmd::DecodeMap(r.result, r.blobs, result, &error)) {
      LOG_W() << "codec" << id << "answered out of shape:" << error;
      return Next();
    }

    switch (result.outcome) {
      case CodecOutcome::kHandled: {
        if (!result.output.has_value()) {
          LOG_W() << "codec" << id << "claimed the input with no output";
          return Finish(WithSource(
              Failed(QStringLiteral("the codec returned nothing")), id,
              result.cards));
        }
        CodecStageResult out;
        out.kind = CodecStageResult::Kind::kHandled;
        out.output = BlobToGFBuffer(*result.output);
        return Finish(WithSource(out, id, result.cards));
      }
      case CodecOutcome::kFailed:
        return Finish(WithSource(
            Failed(result.error.isEmpty()
                       ? QStringLiteral("the codec could not handle the input")
                       : result.error),
            id, result.cards));
      case CodecOutcome::kNotHandled:
      default:
        return Next();
    }
  }

  static auto WithSource(CodecStageResult r, const QString& id,
                         const QString& cards) -> CodecStageResult {
    r.provider = id;
    r.cards = cards;
    return r;
  }

  void Finish(CodecStageResult result) {
    if (finished_) return;
    finished_ = true;
    auto done = std::move(done_);
    deleteLater();
    if (done) done(std::move(result));
  }

  QStringList ids_;
  GFBuffer input_;
  bool required_;
  int timeout_ms_;
  CodecStage::Done done_;
  qsizetype index_ = 0;
  quint64 attempt_ = 0;
  quint64 call_id_ = 0;
  bool finished_ = false;
};

/// Start @p run from the event loop, so `done` never runs inside the call
/// that asked for it.
void Start(Run* run) {
  QMetaObject::invokeMethod(run, [run]() { run->Next(); },
                            Qt::QueuedConnection);
}

}  // namespace

void CodecStage::RunDecoders(GFBuffer input, Done done, int timeout_ms) {
  auto ids = CommandRegistry::Instance().ProvidersWithFlag(
      gf::cmd::kInputDecoder);
  Start(new Run(std::move(ids), std::move(input), false, timeout_ms,
                std::move(done)));
}

void CodecStage::RunEncoder(const QString& id, GFBuffer input, Done done,
                            int timeout_ms) {
  const auto encoders = CommandRegistry::Instance().ProvidersWithFlag(
      gf::cmd::kOutputEncoder);
  auto ids = encoders.contains(id) ? QStringList{id} : QStringList{};
  Start(new Run(std::move(ids), std::move(input), true, timeout_ms,
                std::move(done)));
}

}  // namespace GpgFrontend::UI
