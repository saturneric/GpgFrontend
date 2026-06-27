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

#include "core/model/GFEngineSupportIf.h"
#include "core/thread/Task.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Run a GPG operation asynchronously on the dedicated GPG task runner.
 *
 * Checks @p support_ifs against the context capabilities first; if the
 * operation is unsupported the callback is invoked with GPG_ERR_NOT_SUPPORTED
 * immediately and a null TaskHandler is returned. Otherwise the runnable is
 * executed on the GPG thread and the callback is delivered to the calling
 * thread on completion.
 *
 * @param channel OpenPGP context channel
 * @param runnable GPG operation to execute on the runner thread
 * @param callback function invoked on the calling thread with (GpgError,
 * DataObjectPtr)
 * @param operation human-readable operation name used for task identification
 * @param support_ifs engine capability requirements that must be satisfied
 * @return TaskHandler for the scheduled task, or a null handler if unsupported
 */
auto GF_CORE_EXPORT
RunGpgOperaAsync(int channel, const GpgOperaRunnable& runnable,
                 const GpgOperationCallback& callback, const QString& operation,
                 const QContainer<EngineSupportIf>& support_ifs)
    -> Thread::Task::TaskHandler;

/**
 * @brief Run a GPG operation synchronously on the calling thread.
 *
 * Checks @p support_ifs first; if unsupported returns immediately with
 * GPG_ERR_NOT_SUPPORTED.
 *
 * @param channel OpenPGP context channel
 * @param runnable GPG operation to execute
 * @param operation human-readable operation name (used for logging)
 * @param support_ifs engine capability requirements that must be satisfied
 * @return tuple of (GpgError, DataObjectPtr) with the operation result
 */
auto GF_CORE_EXPORT RunGpgOperaSync(
    int channel, const GpgOperaRunnable& runnable, const QString& operation,
    const QContainer<EngineSupportIf>& support_ifs)
    -> std::tuple<GpgError, DataObjectPtr>;

/**
 * @brief Run an I/O operation asynchronously on the dedicated I/O task runner.
 *
 * @param runnable I/O operation to execute on the runner thread
 * @param callback function invoked on the calling thread with (GFError,
 * DataObjectPtr)
 * @param operation human-readable operation name used for task identification
 * @return TaskHandler for the scheduled task
 */
auto GF_CORE_EXPORT RunIOOperaAsync(const OperaRunnable& runnable,
                                    const OperationCallback& callback,
                                    const QString& operation)
    -> Thread::Task::TaskHandler;

/**
 * @brief Run a general operation asynchronously on the default task runner.
 *
 * @param runnable operation to execute on the runner thread
 * @param callback function invoked on the calling thread with (GFError,
 * DataObjectPtr)
 * @param operation human-readable operation name used for task identification
 * @return TaskHandler for the scheduled task
 */
auto GF_CORE_EXPORT RunOperaAsync(const OperaRunnable& runnable,
                                  const OperationCallback& callback,
                                  const QString& operation)
    -> Thread::Task::TaskHandler;

/**
 * @brief Request cancellation of the GPG operation currently running on
 * @p channel.
 *
 * Engine-agnostic: aborts an in-flight rPGP streaming operation (via the
 * channel's Rust cancel flag) and an in-flight GnuPG operation (via
 * gpgme_cancel_async). The cancelled operation completes through its normal
 * callback path with GPG_ERR_CANCELED. Also marks any operations still queued
 * on the GPG runner for this channel so that they are skipped rather than
 * executed. Cancellation is per-channel: other channels are unaffected. Safe to
 * call when nothing is running.
 *
 * @param channel OpenPGP context channel of the operation to cancel
 */
void GF_CORE_EXPORT RequestCancelGpgOperation(int channel);

/**
 * @brief Clear the cancellation flag for @p channel.
 *
 * Call before starting a new operation (or batch of operations) on a channel so
 * that a previous cancellation request for it does not affect the new work.
 *
 * @param channel OpenPGP context channel whose cancellation state to clear
 */
void GF_CORE_EXPORT ResetGpgOperationCancelState(int channel);

/**
 * @brief Return whether a cancellation has been requested for @p channel and
 * not yet reset.
 *
 * @param channel OpenPGP context channel to query
 * @return true if cancellation is pending for the channel
 */
auto GF_CORE_EXPORT IsGpgOperationCancelRequested(int channel) -> bool;

}  // namespace GpgFrontend
