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

#include "AsyncUtils.h"

#include <mutex>
#include <unordered_set>

#include "core/GFCoreRust.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/DataObject.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend {

namespace {
// Per-channel cancellation flags shared between the UI thread (which requests a
// cancel) and the GPG runner thread (which checks before starting a queued
// operation). Keyed by channel so that cancelling an operation on one channel
// never disturbs an operation on another. A channel is present in the set only
// while a cancel is pending for it.
std::mutex g_gpg_cancel_mutex;
std::unordered_set<int> g_gpg_cancelled_channels;

void SetChannelCancelled(int channel, bool cancelled) {
  std::lock_guard<std::mutex> lock(g_gpg_cancel_mutex);
  if (cancelled) {
    g_gpg_cancelled_channels.insert(channel);
  } else {
    g_gpg_cancelled_channels.erase(channel);
  }
}

auto IsChannelCancelled(int channel) -> bool {
  std::lock_guard<std::mutex> lock(g_gpg_cancel_mutex);
  return g_gpg_cancelled_channels.count(channel) != 0;
}
}  // namespace

auto RunGpgOperaAsync(int channel, const GpgOperaRunnable& runnable,
                      const GpgOperationCallback& callback,
                      const QString& operation,
                      const QContainer<EngineSupportIf>& support_ifs)
    -> Thread::Task::TaskHandler {
  if (!GpgContextSupportIf(channel, support_ifs)) {
    LOG_W() << "operation: " << operation << "is not supported.";
    callback(GPG_ERR_NOT_SUPPORTED, TransferParams());
    return Thread::Task::TaskHandler(nullptr);
  }

  auto handler =
      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
          ->RegisterTask(
              operation,
              [=](const DataObjectPtr& data_object) -> int {
                auto custom_data_object = TransferParams();
                // Skip operations that are still queued when a cancel has been
                // requested (e.g. remaining files in a batch after the user
                // cancels the one in progress).
                auto err = IsChannelCancelled(channel)
                               ? static_cast<GpgError>(GPG_ERR_CANCELED)
                               : runnable(custom_data_object);
                data_object->Swap({err, custom_data_object});
                return 0;
              },
              [=](int rtn, const DataObjectPtr& data_object) {
                if (rtn < 0) {
                  callback(GPG_ERR_USER_1,
                           ExtractParams<DataObjectPtr>(data_object, 1));
                } else {
                  callback(ExtractParams<GpgError>(data_object, 0),
                           ExtractParams<DataObjectPtr>(data_object, 1));
                }
              },
              TransferParams());
  handler.Start();
  return handler;
}

auto RunGpgOperaSync(int channel, const GpgOperaRunnable& runnable,
                     const QString& operation,
                     const QContainer<EngineSupportIf>& support_ifs)
    -> std::tuple<GpgError, DataObjectPtr> {
  if (!GpgContextSupportIf(channel, support_ifs)) {
    LOG_W() << "operation: " << operation << "is not supported.";
    return {GPG_ERR_NOT_SUPPORTED, TransferParams()};
  }

  auto data_object = TransferParams();
  auto err = runnable(data_object);
  return {err, data_object};
}

auto RunIOOperaAsync(const OperaRunnable& runnable,
                     const OperationCallback& callback,
                     const QString& operation) -> Thread::Task::TaskHandler {
  auto handler =
      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_IO)
          ->RegisterTask(
              operation,
              [=](const DataObjectPtr& data_object) -> int {
                auto custom_data_object = TransferParams();
                auto err = runnable(custom_data_object);

                data_object->Swap({err, custom_data_object});
                return 0;
              },
              [=](int rtn, const DataObjectPtr& data_object) {
                if (rtn != 0) {
                  callback(-1, ExtractParams<DataObjectPtr>(data_object, 1));
                } else {
                  callback(ExtractParams<GFError>(data_object, 0),
                           ExtractParams<DataObjectPtr>(data_object, 1));
                }
              },
              TransferParams());
  handler.Start();
  return handler;
}

auto RunOperaAsync(const OperaRunnable& runnable,
                   const OperationCallback& callback, const QString& operation)
    -> Thread::Task::TaskHandler {
  auto handler =
      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
          ->RegisterTask(
              operation,
              [=](const DataObjectPtr& data_object) -> int {
                auto custom_data_object = TransferParams();
                auto err = runnable(custom_data_object);

                data_object->Swap({err, custom_data_object});
                return static_cast<int>(err);
              },
              [=](int rtn, const DataObjectPtr& data_object) {
                if (rtn != 0) {
                  callback(-1, ExtractParams<DataObjectPtr>(data_object, 1));
                } else {
                  callback(ExtractParams<GFError>(data_object, 0),
                           ExtractParams<DataObjectPtr>(data_object, 1));
                }
              },
              TransferParams());
  handler.Start();
  return handler;
}

void RequestCancelGpgOperation(int channel) {
  SetChannelCancelled(channel, true);

#ifdef HAS_RUST_SUPPORT
  // Abort an in-flight rPGP streaming operation on this channel at its next
  // chunk read.
  Rust::gfr_set_operation_cancelled(channel, true);
#endif

  // Abort an in-flight GnuPG (gpgme) operation on this channel's context. Only
  // GnuPG channels use a GpgContext; for rPGP channels the cast yields nullptr
  // and the Rust cancel flag above already covers cancellation.
  if (auto* gpg_ctx =
          dynamic_cast<GpgContext*>(&OpenPGPContext::GetInstance(channel))) {
    gpg_ctx->CancelCurrentOperation();
  }
}

void ResetGpgOperationCancelState(int channel) {
  SetChannelCancelled(channel, false);
#ifdef HAS_RUST_SUPPORT
  Rust::gfr_set_operation_cancelled(channel, false);
#endif
}

auto IsGpgOperationCancelRequested(int channel) -> bool {
  return IsChannelCancelled(channel);
}
}  // namespace GpgFrontend