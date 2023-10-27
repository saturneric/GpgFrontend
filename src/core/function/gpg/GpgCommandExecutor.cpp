/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include "GpgCommandExecutor.h"

#include <boost/format.hpp>
#include <string>

#include "GpgFunctionObject.h"
#include "core/thread/DataObject.h"
#include "core/thread/TaskRunnerGetter.h"
#include "module/Module.h"
#include "spdlog/spdlog.h"
#include "thread/Task.h"

namespace GpgFrontend {

GpgCommandExecutor::ExecuteContext::ExecuteContext(
    std::string cmd, std::vector<std::string> arguments,
    GpgCommandExecutorCallback callback, Module::TaskRunnerPtr task_runner,
    GpgCommandExecutorInteractor int_func)
    : cmd(cmd),
      arguments(arguments),
      cb_func(callback),
      int_func(int_func),
      task_runner(task_runner) {}

GpgCommandExecutor::GpgCommandExecutor(int channel)
    : SingletonFunctionObject<GpgCommandExecutor>(channel) {}

void GpgCommandExecutor::ExecuteSync(ExecuteContext context) {
  Thread::Task *task = build_task_from_exec_ctx(context);

  QEventLoop looper;
  QObject::connect(task, &Thread::Task::SignalTaskEnd, &looper,
                   &QEventLoop::quit);

  Thread::TaskRunnerPtr target_task_runner = nullptr;

  if (context.task_runner != nullptr) {
    target_task_runner = context.task_runner;
  } else {
    target_task_runner =
        GpgFrontend::Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
            Thread::TaskRunnerGetter::kTaskRunnerType_External_Process);
  }

  target_task_runner->PostTask(task);

  // to arvoid dead lock issue we need to check if current thread is the same as
  // target thread. if it is, we can't call exec() because it will block the
  // current thread.
  if (QThread::currentThread() != target_task_runner->GetThread()) {
    // block until task finished
    // this is to keep reference vaild until task finished
    looper.exec();
  }
}

void GpgCommandExecutor::ExecuteConcurrentlyAsync(ExecuteContexts contexts) {
  for (auto &context : contexts) {
    auto &cmd = context.cmd;
    SPDLOG_INFO("gpg concurrently called cmd {}", cmd);

    Thread::Task *task = build_task_from_exec_ctx(context);

    if (context.task_runner != nullptr)
      context.task_runner->PostTask(task);
    else
      GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(
              Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
          ->PostTask(task);
  }
}

void GpgCommandExecutor::ExecuteConcurrentlySync(
    const ExecuteContexts contexts) {
  QEventLoop looper;
  int remainingTasks = contexts.size();

  for (auto &context : contexts) {
    auto &cmd = context.cmd;
    SPDLOG_INFO("gpg concurrently called cmd {}", cmd);

    Thread::Task *task = build_task_from_exec_ctx(context);

    QObject::connect(task, &Thread::Task::SignalTaskEnd, [&]() {
      --remainingTasks;
      if (remainingTasks <= 0) {
        looper.quit();
      }
    });

    if (context.task_runner != nullptr)
      context.task_runner->PostTask(task);
    else
      GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(
              Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
          ->PostTask(task);
  }

  looper.exec();
}

Thread::Task *GpgCommandExecutor::build_task_from_exec_ctx(
    const ExecuteContext &context) {
  auto &cmd = context.cmd;
  auto &arguments = context.arguments;
  auto &interact_function = context.int_func;
  auto &cmd_executor_callback = context.cb_func;

  const std::string joined_argument = std::accumulate(
      std::begin(arguments), std::end(arguments), std::string(),
      [](const std::string &a, const std::string &b) -> std::string {
        return a + (a.length() > 0 ? " " : "") + b;
      });

  SPDLOG_DEBUG("building task: called cmd {} arguments size: {}", cmd,
               arguments.size());

  Thread::Task::TaskCallback result_callback =
      [cmd, joined_argument](int rtn, Thread::DataObjectPtr data_object) {
        SPDLOG_DEBUG(
            "data object args count of cmd executor result callback: {}",
            data_object->GetObjectSize());
        if (!data_object->Check<int, std::string, std::string,
                                GpgCommandExecutorCallback>())
          throw std::runtime_error("invalid data object size");

        auto exit_code = Thread::ExtractParams<int>(data_object, 0);
        auto process_stdout =
            Thread::ExtractParams<std::string>(data_object, 1);
        auto process_stderr =
            Thread::ExtractParams<std::string>(data_object, 2);
        auto callback =
            Thread::ExtractParams<GpgCommandExecutorCallback>(data_object, 3);

        // call callback
        SPDLOG_DEBUG(
            "calling custom callback from caller of cmd {} {}, "
            "exit_code: {}",
            cmd, joined_argument, exit_code);
        callback(exit_code, process_stdout, process_stderr);
      };

  Thread::Task::TaskRunnable runner =
      [joined_argument](Thread::DataObjectPtr data_object) -> int {
    SPDLOG_DEBUG("process runner called, data object size: {}",
                 data_object->GetObjectSize());

    if (!data_object->Check<std::string, std::vector<std::string>,
                            GpgCommandExecutorInteractor,
                            GpgCommandExecutorCallback>())
      throw std::runtime_error("invalid data object size");

    // get arguments
    auto cmd = Thread::ExtractParams<std::string>(data_object, 0);
    auto arguments =
        Thread::ExtractParams<std::vector<std::string>>(data_object, 1);
    auto interact_func =
        Thread::ExtractParams<GpgCommandExecutorInteractor>(data_object, 2);
    auto callback =
        Thread::ExtractParams<GpgCommandExecutorCallback>(data_object, 3);

    auto *cmd_process = new QProcess();
    cmd_process->moveToThread(QThread::currentThread());
    cmd_process->setProcessChannelMode(QProcess::MergedChannels);
    cmd_process->setProgram(QString::fromStdString(cmd));
    QStringList q_arguments;
    for (const auto &argument : arguments)
      q_arguments.append(QString::fromStdString(argument));
    cmd_process->setArguments(q_arguments);

    QObject::connect(
        cmd_process, &QProcess::started, [cmd, joined_argument]() -> void {
          SPDLOG_DEBUG(
              "\n== Process Execute Started ==\nCommand: {}\nArguments: "
              "{}\n========================",
              cmd, joined_argument);
        });
    QObject::connect(
        cmd_process, &QProcess::readyReadStandardOutput,
        [interact_func, cmd_process]() { interact_func(cmd_process); });
    QObject::connect(
        cmd_process, &QProcess::errorOccurred,
        [=](QProcess::ProcessError error) {
          SPDLOG_ERROR("caught error while executing command: {} {}, error: {}",
                       cmd, joined_argument, error);
        });

    SPDLOG_DEBUG(
        "\n== Process Execute Ready ==\nCommand: {}\nArguments: "
        "{}\n========================",
        cmd, joined_argument);

    cmd_process->start();
    cmd_process->waitForFinished();

    std::string process_stdout =
                    cmd_process->readAllStandardOutput().toStdString(),
                process_stderr =
                    cmd_process->readAllStandardError().toStdString();
    int exit_code = cmd_process->exitCode();

    SPDLOG_DEBUG(
        "\n==== Process Execution Summary ====\n"
        "Command: {}\n"
        "Arguments: {}\n"
        "Exit Code: {}\n"
        "---- Standard Output ----\n"
        "{}\n"
        "---- Standard Error ----\n"
        "{}\n"
        "===============================",
        cmd, joined_argument, exit_code, process_stdout, process_stderr);

    cmd_process->close();
    cmd_process->deleteLater();

    data_object->Swap({exit_code, process_stdout, process_stderr, callback});
    return 0;
  };

  return new Thread::Task(
      std::move(runner),
      (boost::format("GpgCommamdExecutor(%1%){%2%}") % cmd % joined_argument)
          .str(),
      Thread::TransferParams(cmd, arguments, interact_function,
                             cmd_executor_callback),
      std::move(result_callback));
}

}  // namespace GpgFrontend