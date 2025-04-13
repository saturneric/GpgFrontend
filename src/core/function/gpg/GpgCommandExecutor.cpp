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
#include "GpgCommandExecutor.h"

#include <qglobal.h>

#include "core/model/DataObject.h"
#include "core/module/Module.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend {

auto BuildTaskFromExecCtx(const GpgCommandExecutor::ExecuteContext &context)
    -> Thread::Task * {
  const auto &cmd = context.cmd;
  const auto &arguments = context.arguments;
  const auto &interact_function = context.int_func;
  const auto &cmd_executor_callback = context.cb_func;

  Thread::Task::TaskCallback result_callback =
      [cmd](int /*rtn*/, const DataObjectPtr &data_object) {
        LOG_D() << "data object args count of cmd executor result callback:"
                << data_object->GetObjectSize();

        if (!data_object->Check<int, QString, GpgCommandExecutorCallback>()) {
          FLOG_W("data object checking failed");
          return;
        }

        auto exit_code = ExtractParams<int>(data_object, 0);
        auto process_stdout = ExtractParams<QString>(data_object, 1);
        auto callback =
            ExtractParams<GpgCommandExecutorCallback>(data_object, 2);

        callback(exit_code, process_stdout, {});
      };

  Thread::Task::TaskRunnable runner =
      [](const DataObjectPtr &data_object) -> int {
    FLOG_D("process runner called, data object size: %lu",
           data_object->GetObjectSize());

    if (!data_object->Check<QString, QStringList, GpgCommandExecutorInterator,
                            GpgCommandExecutorCallback>()) {
      FLOG_W("data object checking failed");
      return -1;
    }

    // get arguments
    auto cmd = ExtractParams<QString>(data_object, 0);
    auto arguments = ExtractParams<QStringList>(data_object, 1);
    auto interact_func =
        ExtractParams<GpgCommandExecutorInterator>(data_object, 2);
    auto callback = ExtractParams<GpgCommandExecutorCallback>(data_object, 3);
    const QString joined_argument = arguments.join(" ");

    // create process
    auto *cmd_process = new QProcess();
    // move to current thread
    //
    cmd_process->moveToThread(QThread::currentThread());
    // set process channel mode
    // this is to make sure we can get all output from stdout and stderr
    cmd_process->setProcessChannelMode(QProcess::MergedChannels);
    cmd_process->setProgram(cmd);

    // set arguments
    QStringList q_arguments;
    for (const auto &argument : arguments) {
      q_arguments.append(argument);
    }
    cmd_process->setArguments(q_arguments);

    QObject::connect(
        cmd_process, &QProcess::started, [cmd, joined_argument]() -> void {
          LOG_D() << "\n== Process Execute Started ==\nCommand: " << cmd
                  << "\nArguments: " << joined_argument
                  << " \n========================";
        });
    QObject::connect(
        cmd_process, &QProcess::readyReadStandardOutput,
        [interact_func, cmd_process]() { interact_func(cmd_process); });
    QObject::connect(cmd_process, &QProcess::errorOccurred,
                     [=](QProcess::ProcessError error) {
                       LOG_W()
                           << "caught error while executing command: " << cmd
                           << joined_argument << ", error:" << error;
                     });

    LOG_D() << "\n== Process Execute Ready ==\nCommand: " << cmd
            << "\nArguments: " << joined_argument
            << "\n========================";

    cmd_process->start();
    cmd_process->waitForFinished();

    QString process_stdout = cmd_process->readAllStandardOutput();
    int exit_code = cmd_process->exitCode();

    LOG_D() << "\n==== Process Execution Summary ====\n"
            << "Command: " << cmd << "\n"
            << "Arguments: " << joined_argument << "\n"
            << "Exit Code: " << exit_code << "\n"
            << "---- Standard Output ----\n"
            << process_stdout << "\n"
            << "===============================";

    cmd_process->close();
    cmd_process->deleteLater();

    data_object->Swap({exit_code, process_stdout, callback});
    return 0;
  };

  return new Thread::Task(
      std::move(runner),
      QString("GpgCommamdExecutor(%1){%2}").arg(cmd).arg(arguments.join(' ')),
      TransferParams(cmd, arguments, interact_function, cmd_executor_callback),
      std::move(result_callback));
}

void GpgCommandExecutor::ExecuteSync(ExecuteContext context) {
  Thread::Task *task = BuildTaskFromExecCtx(context);
  QPointer<Thread::Task> p_t = task;

  auto *looper = new QEventLoop();
  QObject::connect(task, &Thread::Task::SignalTaskEnd, looper,
                   &QEventLoop::quit);

  Thread::TaskRunnerPtr target_task_runner = context.task_runner;

  if (target_task_runner == nullptr) {
    target_task_runner =
        GpgFrontend::Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
            Thread::TaskRunnerGetter::kTaskRunnerType_External_Process);
  }
  target_task_runner->PostTask(task);

  // to arvoid dead lock issue we need to check if current thread is the
  // same as target thread. if it is, we can't call exec() because it will
  // block the current thread.
  FLOG_D() << "blocking until gpg command " << context.cmd << context.arguments
           << " finish...";
  // block until task finished
  // this is to keep reference vaild until task finished
  looper->exec();
  looper->deleteLater();
}

void GpgCommandExecutor::ExecuteConcurrentlyAsync(ExecuteContexts contexts) {
  for (auto &context : contexts) {
    Thread::Task *task = BuildTaskFromExecCtx(context);

    if (context.task_runner != nullptr) {
      context.task_runner->PostTask(task);
    } else {
      GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(
              Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
          ->PostTask(task);
    }
  }
}

void GpgCommandExecutor::ExecuteConcurrentlySync(ExecuteContexts contexts) {
  QEventLoop looper;
  auto remaining_tasks = contexts.size();
  Thread::TaskRunnerPtr target_task_runner = nullptr;

  for (auto &context : contexts) {
    const auto &cmd = context.cmd;
    LOG_D() << "gpg concurrently called cmd: " << cmd;

    Thread::Task *task = BuildTaskFromExecCtx(context);

    QObject::connect(task, &Thread::Task::SignalTaskEnd, [&]() {
      --remaining_tasks;
      LOG_D() << "remaining tasks: " << remaining_tasks;
      if (remaining_tasks <= 0) {
        FLOG_D("no remaining task, quit");
        looper.quit();
      }
    });

    if (context.task_runner != nullptr) {
      target_task_runner = context.task_runner;
    } else {
      target_task_runner =
          GpgFrontend::Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
              Thread::TaskRunnerGetter::kTaskRunnerType_External_Process);
    }

    target_task_runner->PostTask(task);
  }

  FLOG_D("blocking until concurrent gpg commands finish...");
  // block until task finished
  // this is to keep reference vaild until task finished
  looper.exec();
}

GpgCommandExecutor::ExecuteContext::ExecuteContext(
    QString cmd, QStringList arguments, GpgCommandExecutorCallback callback,
    Module::TaskRunnerPtr task_runner, GpgCommandExecutorInterator int_func)
    : cmd(std::move(cmd)),
      arguments(std::move(arguments)),
      cb_func(std::move(callback)),
      int_func(std::move(int_func)),
      task_runner(std::move(task_runner)) {}

GpgCommandExecutor::GpgCommandExecutor(int channel)
    : GpgFrontend::SingletonFunctionObject<GpgCommandExecutor>(channel) {}

void GpgCommandExecutor::GpgExecuteSync(const ExecuteContext &context) {
  const auto gpg_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});

  if (context.cmd.isEmpty() && gpg_path.isEmpty()) {
    LOG_E() << "failed to execute gpg command, gpg binary path is empty.";
    return;
  }

  LOG_D() << "got gpg binary path:" << gpg_path;
  LOG_D() << "context channel:" << GetChannel()
          << "home path: " << ctx_.HomeDirectory();

  ExecuteContext ctx = {
      context.cmd.isEmpty() ? gpg_path : context.cmd,
      context.arguments,
      context.cb_func,
      context.task_runner,
      context.int_func,
  };

  if (!ctx.arguments.contains("--homedir") && !ctx_.HomeDirectory().isEmpty()) {
    ctx.arguments.append("--homedir");
    ctx.arguments.append(ctx_.HomeDirectory());
  }

  return ExecuteSync(ctx);
}
}  // namespace GpgFrontend