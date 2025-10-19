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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgContext.h"
#include "core/module/Module.h"

namespace GpgFrontend {

using GpgCommandExecutorCallback =
    std::function<void(int, QByteArray, QByteArray)>;
using GpgCommandExecutorInterator = std::function<void(QProcess *)>;

/**
 * @brief Extra commands related to GPG
 *
 */
class GF_CORE_EXPORT GpgCommandExecutor
    : public SingletonFunctionObject<GpgCommandExecutor> {
 public:
  struct GF_CORE_EXPORT ExecuteContext {
    QString cmd;
    QStringList arguments;
    GpgCommandExecutorCallback cb_func;
    GpgCommandExecutorInterator int_func;
    Module::TaskRunnerPtr task_runner = nullptr;

    /**
     * @brief Construct a new Execute Context object
     *
     */
    ExecuteContext() = default;

    /**
     * @brief Construct a new Execute Context object
     *
     * @param cmd
     * @param arguments
     * @param callback
     * @param task_runner
     * @param int_func
     */
    ExecuteContext(
        QString cmd, QStringList arguments,
        GpgCommandExecutorCallback callback = [](int, const QString &,
                                                 const QString &) {},
        Module::TaskRunnerPtr task_runner = nullptr,
        GpgCommandExecutorInterator int_func = [](QProcess *) {});

    /**
     * @brief Construct a new Execute Context object
     *
     * @param arguments
     * @param callback
     * @param task_runner
     * @param int_func
     */
    ExecuteContext(
        QStringList arguments,
        GpgCommandExecutorCallback callback = [](int, const QString &,
                                                 const QString &) {},
        Module::TaskRunnerPtr task_runner = nullptr,
        GpgCommandExecutorInterator int_func = [](QProcess *) {});
  };

  using ExecuteContexts = QContainer<ExecuteContext>;

  explicit GpgCommandExecutor(int channel = kGpgFrontendDefaultChannel);

  /**
   * @brief Excuting a command
   *
   * @param arguments Command parameters
   * @param interact_func Command answering function
   */
  static void ExecuteSync(const ExecuteContext &);

  /**
   * @brief
   *
   */
  static void ExecuteConcurrentlyAsync(const ExecuteContexts &);

  /**
   * @brief
   *
   */
  static void ExecuteConcurrentlySync(const ExecuteContexts &);

  /**
   * @brief
   *
   */
  auto GpgExecuteSync(const ExecuteContext &)
      -> std::tuple<int, QByteArray, QByteArray>;

  /**
   * @brief
   *
   */
  auto GpgConfExecuteSync(const ExecuteContext &)
      -> std::tuple<int, QByteArray, QByteArray>;

  /**
   * @brief
   *
   * @param context
   */
  void GpgExecuteAsync(const ExecuteContext &);

  /**
   * @brief
   *
   * @param context
   */
  void GpgConfExecuteAsync(const ExecuteContext &);

 private:
  GpgContext &ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
};

}  // namespace GpgFrontend
