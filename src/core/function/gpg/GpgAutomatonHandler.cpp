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

#include "GpgAutomatonHandler.h"

#include <chrono>
#include <thread>

#include "core/function/gpg/GpgContext.h"
#include "core/model/GFEngineSupportIf.h"
#include "core/model/GpgData.h"
#include "core/model/GpgKey.h"

namespace GpgFrontend {

namespace {
// If GnuPG re-issues the exact same prompt this many times in a row, our reply
// is not advancing the session (livelock); give up. Legitimate edit-key flows
// never repeat a single prompt anywhere near this often.
constexpr int kMaxConsecutiveSamePrompts = 16;

// Hard cap on the total number of prompts handled in one interaction, as a
// backstop against pathological loops that vary their prompts.
constexpr int kMaxTotalPrompts = 256;

// How often the watchdog polls the async operation for completion.
constexpr int kInteractPollIntervalMs = 20;

// After cancelling a timed-out operation, how long we keep draining gpgme to
// let the cancellation settle so the shared context stays reusable.
constexpr int kInteractCancelDrainMs = 1000;
}  // namespace

GpgAutomatonHandler::GpgAutomatonHandler(int channel)
    : SingletonFunctionObject<GpgAutomatonHandler>(channel) {}

auto InteratorCbFunc(void* handle, const char* status, const char* args, int fd)
    -> gpgme_error_t {
  auto* handel = static_cast<AutomatonHandelStruct*>(handle);
  const auto status_s = QString::fromUtf8(status);
  const auto args_s = QString::fromUtf8(args);

  LOG_D() << "gpg interation status: " << status_s << "args: " << args_s;

  if (status_s == "KEY_CONSIDERED") {
    auto tokens = QString(args).split(' ');

    // for multi key considered info, skip
    // for empty key considered info, also skip
    if (handel->KeyConsiderIdMatched() || handel->KeyFpr().isEmpty()) {
      return GPG_ERR_NO_ERROR;
    }

    if (tokens.empty() || tokens[0] != handel->KeyFpr()) {
      LOG_W() << "handle struct key fpr: " << handel->KeyFpr()
              << "mismatch token: " << tokens[0] << ", exit...";
      handel->SetSuccess(false);
      return -1;
    }

    handel->SetKeyConsiderIdMatched(true);
    return GPG_ERR_NO_ERROR;
  }

  if (status_s == "KEYEXPIRED") return GPG_ERR_NO_ERROR;

  if (status_s == "CARDCTRL") {
    auto tokens = QString(args).split(' ');

    if (handel->SerialNumber().isEmpty()) return GPG_ERR_NO_ERROR;

    if (tokens.empty() || tokens[0] != handel->SerialNumber()) {
      LOG_W() << "handle struct serial number: " << handel->SerialNumber()
              << "mismatch token: " << tokens[0] << ", exit...";
      handel->SetSuccess(false);
      return -1;
    }

    return GPG_ERR_NO_ERROR;
  }

  if (status_s == "GOT_IT" || status_s.isEmpty()) {
    FLOG_D("gpg reply is GOT_IT, continue...");
    return GPG_ERR_NO_ERROR;
  }

  LOG_D() << "current state" << handel->CurrentStatus()
          << "gpg status: " << status_s << ", args: " << args_s;

  handel->SetPromptStatus(status_s, args_s);

  // Guard against a livelock: if our reply never satisfies GnuPG it keeps
  // re-prompting and gpgme_op_interact would never return. Abort cleanly so the
  // caller gets a failure instead of a hung process.
  if (handel->Stuck()) {
    LOG_W() << "gpg interaction appears stuck on prompt:" << status_s
            << ", args:" << args_s << "- aborting to avoid hang";
    handel->SetSuccess(false);
    return GPG_ERR_GENERAL;
  }

  AutomatonState next_state = handel->NextState(status_s, args_s);
  if (next_state == GpgAutomatonHandler::kAS_ERROR) {
    FLOG_D("handel next state caught error, abort...");
    handel->SetSuccess(false);
    return -1;
  }
  LOG_D() << "next state:" << next_state;

  // set state and preform action
  handel->SetStatus(next_state);
  GpgAutomatonHandler::Command cmd = handel->Action();

  LOG_D() << "next action, cmd:" << cmd;

  if (!cmd.isEmpty()) {
    auto btye_array = cmd.toUtf8();
    gpgme_io_write(fd, btye_array, btye_array.size());
    gpgme_io_write(fd, "\n", 1);
  } else if (status_s == "GET_LINE" &&
             (args_s == "keyedit.prompt" || args_s == "cardedit.prompt")) {
    // gpg is parked at the top-level edit menu and our state machine produced
    // no command. Returning GPG_ERR_FALSE would let gpgme inject a default
    // empty line, which just re-displays the menu and can livelock until
    // Stuck() trips. At this menu "quit" is always valid, so drive gpg out
    // cleanly instead -- it prevents the interaction from stalling here in the
    // first place.
    LOG_W() << "no command for" << args_s
            << "- sending quit to leave edit menu";
    gpgme_io_write(fd, "quit\n", 5);
  } else if (status_s.startsWith("GET_")) {
    // Some other prompt we have no answer for: fall back to gpgme's default
    // handling rather than guessing an (possibly invalid) response.
    return GPG_ERR_FALSE;
  }

  return GPG_ERR_NO_ERROR;
}

auto DoInteractImpl(OpenPGPContext& ctx_, const GpgKeyPtr& key, bool card_edit,
                    const QString& fpr,
                    AutomatonNextStateHandler next_state_handler,
                    AutomatonActionHandler action_handler, int flags,
                    int timeout_ms) -> std::tuple<GpgError, bool> {
  auto& g_ctx = GpgCtx(ctx_);
  gpgme_key_t p_key = key == nullptr ? nullptr : static_cast<gpgme_key_t>(*key);

  // Card edits may legitimately block for a long time (hardware / the user
  // touching the card), so we keep the original synchronous, unbounded call on
  // the shared default context.
  if (card_edit) {
    AutomatonHandelStruct handle(card_edit, fpr);
    handle.SetHandler(std::move(next_state_handler), std::move(action_handler));

    GpgData data_out;
    auto err =
        gpgme_op_interact(g_ctx.DefaultContext(), p_key, flags, InteratorCbFunc,
                          static_cast<void*>(&handle), data_out);
    return {err, handle.Success()};
  }

  // Key edits run on their OWN disposable context, never the shared default
  // one. The reason: a wedged gpgme_op_interact cannot always be cancelled.
  // Synchronous gpgme_cancel (see below) reaps the common case, but if even its
  // channel teardown fails to wake a sufficiently stuck gpg, we must abandon
  // the whole context. If that happened on the shared context, every later
  // operation on this channel would be poisoned. By isolating the interaction
  // we can discard the context on an un-cancellable timeout; the shared one
  // stays pristine.
  gpgme_ctx_t ctx = g_ctx.CreateDisposableContext();
  if (ctx == nullptr) return {GPG_ERR_GENERAL, false};

  // Run the interaction asynchronously and drive it ourselves so a gpgme
  // operation that blocks internally (the callback never re-fires) is still
  // bounded by a wall-clock deadline instead of hanging the caller.
  //
  // The handle and output buffer are heap-owned: on the unhappy path we abandon
  // (leak) them together with the context rather than free memory the gpgme
  // worker might still touch.
  auto handle = std::make_unique<AutomatonHandelStruct>(card_edit, fpr);
  handle->SetHandler(std::move(next_state_handler), std::move(action_handler));
  auto data_out = std::make_unique<GpgData>();

  auto err =
      gpgme_op_interact_start(ctx, p_key, flags, InteratorCbFunc,
                              static_cast<void*>(handle.get()), *data_out);
  if (err != GPG_ERR_NO_ERROR) {
    gpgme_release(ctx);
    return {err, false};
  }

  // Measure real wall-clock elapsed, not poll iterations: a single gpgme_wait
  // (or a callback it invokes) can take a while, so counting iterations would
  // undercount and let the deadline slip well past timeout_ms.
  const auto start = std::chrono::steady_clock::now();
  const auto deadline = start + std::chrono::milliseconds(timeout_ms);
  for (;;) {
    gpgme_error_t status = GPG_ERR_NO_ERROR;
    if (gpgme_wait(ctx, &status, 0) == ctx) {
      // Operation finished; status carries its result.
      gpgme_release(ctx);
      return {status, handle->Success()};
    }

    if (std::chrono::steady_clock::now() >= deadline) break;

    std::this_thread::sleep_for(
        std::chrono::milliseconds(kInteractPollIntervalMs));
  }

  const auto waited_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
  LOG_W() << "gpg key interaction timed out after" << waited_ms
          << "ms, cancelling...";

  // Use the SYNCHRONOUS gpgme_cancel here, not gpgme_cancel_async. Async cancel
  // only takes effect "the next time I/O occurs in the context" -- but a wedged
  // gpg is blocked at a prompt producing no I/O, so async cancel can never
  // fire. Synchronous cancel takes effect immediately by tearing down gpg's I/O
  // channels; closing the command/stdin pipe sends EOF to gpg, waking a process
  // blocked in read() so it exits and frees its keyring lock. We are not inside
  // gpgme_wait at this point (the poll loop above has exited), which the manual
  // requires for gpgme_cancel on the global event loop.
  gpgme_cancel(ctx);

  // Drain the cancellation so the disposable context can be released cleanly.
  // After a synchronous cancel this normally settles on the first poll.
  const auto drain_deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(kInteractCancelDrainMs);
  for (;;) {
    gpgme_error_t status = GPG_ERR_NO_ERROR;
    if (gpgme_wait(ctx, &status, 0) == ctx) {
      // Cancellation settled; the context is idle again, free everything.
      gpgme_release(ctx);
      return {GPG_ERR_TIMEOUT, false};
    }

    if (std::chrono::steady_clock::now() >= drain_deadline) break;

    std::this_thread::sleep_for(
        std::chrono::milliseconds(kInteractPollIntervalMs));
  }

  // The operation refused to cancel within the drain window: gpg is wedged and
  // still holding its keyring lock, which would block every later edit on this
  // channel (gpg serialises edit sessions at the process level, below gpgme).
  // Release the disposable context to tear down the engine and kill the stuck
  // gpg child, freeing that lock. This is safe precisely because the context is
  // disposable -- the shared default context is never touched.
  //
  // The handle/data buffers are leaked (ownership released, memory kept alive)
  // rather than freed, in case gpgme_release() drives the callback one last
  // time during teardown: a small one-off leak beats a use-after-free.
  LOG_E() << "gpg interaction did not cancel within" << kInteractCancelDrainMs
          << "ms; releasing disposable context to kill the stuck gpg child";
  [[maybe_unused]] auto* leaked_handle = handle.release();
  [[maybe_unused]] auto* leaked_data = data_out.release();
  gpgme_release(ctx);
  return {GPG_ERR_TIMEOUT, false};
}

auto GpgAutomatonHandler::DoInteract(
    const GpgKeyPtr& key, AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler, int flags, int timeout_ms)
    -> std::tuple<GpgError, bool> {
  if (!GPG_CTX_MIN_SUPPORT()) return {GPG_ERR_NOT_SUPPORTED, false};

  assert(key != nullptr);
  if (key == nullptr) return {GPG_ERR_USER_1, false};

  return DoInteractImpl(ctx_, key, false, key->Fingerprint(),
                        std::move(next_state_handler),
                        std::move(action_handler), flags, timeout_ms);
}

auto GpgAutomatonHandler::DoCardInteract(
    const QString& serial_number, AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler) -> std::tuple<GpgError, bool> {
  if (!GPG_CTX_MIN_SUPPORT()) return {GPG_ERR_NOT_SUPPORTED, false};

  // timeout_ms is ignored on the card path (card edits run unbounded); pass the
  // default just to satisfy the signature.
  return DoInteractImpl(ctx_, nullptr, true, serial_number,
                        std::move(next_state_handler),
                        std::move(action_handler), GPGME_INTERACT_CARD,
                        kDefaultInteractTimeoutMs);
}

auto GpgAutomatonHandler::AutomatonHandelStruct::NextState(QString gpg_status,
                                                           QString args)
    -> AutomatonState {
  return next_state_handler_(current_state_, std::move(gpg_status),
                             std::move(args));
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetHandler(
    AutomatonNextStateHandler next_state_handler,
    AutomatonActionHandler action_handler) {
  next_state_handler_ = std::move(next_state_handler);
  action_handler_ = std::move(action_handler);
}

auto GpgAutomatonHandler::AutomatonHandelStruct::CurrentStatus() const
    -> AutomatonState {
  return current_state_;
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetStatus(
    AutomatonState next_state) {
  current_state_ = next_state;
}

auto GpgAutomatonHandler::AutomatonHandelStruct::Action() -> Command {
  return action_handler_(*this, current_state_);
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetSuccess(bool success) {
  success_ = success;
}

auto GpgAutomatonHandler::AutomatonHandelStruct::Success() const -> bool {
  return success_;
}
auto GpgAutomatonHandler::AutomatonHandelStruct::KeyFpr() const -> QString {
  return card_edit_ ? "" : id_;
}

GpgAutomatonHandler::AutomatonHandelStruct::AutomatonHandelStruct(
    bool card_edit, QString id)
    : card_edit_(card_edit), id_(std::move(id)) {}

auto GpgAutomatonHandler::AutomatonHandelStruct::PromptStatus() const
    -> std::tuple<QString, QString> {
  return {prompt_status_, prompt_args_};
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetPromptStatus(QString status,
                                                                 QString args) {
  if (status == prompt_status_ && args == prompt_args_) {
    ++prompt_repeat_count_;
  } else {
    prompt_repeat_count_ = 0;
  }
  ++prompt_total_count_;

  prompt_status_ = std::move(status);
  prompt_args_ = std::move(args);
}

auto GpgAutomatonHandler::AutomatonHandelStruct::Stuck() const -> bool {
  return prompt_repeat_count_ >= kMaxConsecutiveSamePrompts ||
         prompt_total_count_ >= kMaxTotalPrompts;
}

auto GpgAutomatonHandler::AutomatonHandelStruct::SerialNumber() const
    -> QString {
  return card_edit_ ? id_ : "";
}

void GpgAutomatonHandler::AutomatonHandelStruct::SetKeyConsiderIdMatched(
    bool matched) {
  key_consider_id_matched_ = matched;
}

auto GpgAutomatonHandler::AutomatonHandelStruct::KeyConsiderIdMatched() const
    -> bool {
  return key_consider_id_matched_;
}
}  // namespace GpgFrontend