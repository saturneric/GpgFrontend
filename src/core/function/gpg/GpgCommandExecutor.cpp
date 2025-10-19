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
  const auto &int_func = context.int_func;
  const auto &cb = context.cb_func;

  Thread::Task::TaskCallback result_callback = [cmd](int /*rtn*/,
                                                     const DataObjectPtr &obj) {
    LOG_D() << "data object args count of cmd executor result callback:"
            << obj->GetObjectSize();

    if (!obj->Check<int, QByteArray, QByteArray,
                    GpgCommandExecutorCallback>()) {
      LOG_W() << "failed gpg command: " << cmd;
      return;
    }

    auto code = ExtractParams<int>(obj, 0);
    auto out = ExtractParams<QByteArray>(obj, 1);
    auto err = ExtractParams<QByteArray>(obj, 2);
    auto cb = ExtractParams<GpgCommandExecutorCallback>(obj, 3);

    cb(code, out, err);
  };

  Thread::Task::TaskRunnable runner =
      [](const DataObjectPtr &data_object) -> int {
    LOG_D() << "process runner called, data object size:"
            << data_object->GetObjectSize();

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
    auto *pcs = new QProcess();
    // move to current thread
    //
    pcs->moveToThread(QThread::currentThread());
    // set process channel mode
    // this is to make sure we can get all output from stdout and stderr
    pcs->setProcessChannelMode(QProcess::SeparateChannels);
    pcs->setProgram(cmd);

    // set arguments
    QStringList q_arguments;
    for (const auto &argument : arguments) {
      q_arguments.append(argument);
    }
    pcs->setArguments(q_arguments);

    QObject::connect(pcs, &QProcess::started, [cmd, joined_argument]() -> void {
      LOG_D() << "\n== Process Execute Started ==\nCommand: " << cmd
              << "\nArguments: " << joined_argument
              << " \n========================";
    });
    QObject::connect(pcs, &QProcess::readyReadStandardOutput,
                     [interact_func, pcs]() { interact_func(pcs); });
    QObject::connect(
        pcs, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
          LOG_W() << "caught error while executing command: " << cmd
                  << joined_argument << ", error:" << error;
        });

    LOG_D() << "\n== Process Execute Ready ==\nCommand: " << cmd
            << "\nArguments: " << joined_argument
            << "\n========================";

    pcs->start();
    pcs->waitForFinished();

    auto out = pcs->readAllStandardOutput();
    auto err = pcs->readAllStandardError();
    auto code = pcs->exitCode();

    LOG_D() << "\n==== Process Execution Summary ====\n"
            << "Command: " << cmd << "\n"
            << "Arguments: " << joined_argument << "\n"
            << "Exit Code: " << code << "\n"
            << "---- Standard Output ----\n"
            << out << "\n"
            << "---- Standard Error ----\n"
            << err << "\n"
            << "===============================";

    pcs->close();
    pcs->deleteLater();

    data_object->Swap({code, out, err, callback});
    return 0;
  };

  return new Thread::Task(
      std::move(runner),
      QString("GpgCommamdExecutor(%1){%2}").arg(cmd).arg(arguments.join(' ')),
      TransferParams(cmd, arguments, int_func, cb), std::move(result_callback));
}

void GpgCommandExecutor::ExecuteSync(const ExecuteContext &context) {
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

void GpgCommandExecutor::ExecuteConcurrentlyAsync(
    const ExecuteContexts &contexts) {
  for (const auto &context : contexts) {
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

void GpgCommandExecutor::ExecuteConcurrentlySync(
    const ExecuteContexts &contexts) {
  QEventLoop looper;
  auto remaining_tasks = contexts.size();
  Thread::TaskRunnerPtr target_task_runner = nullptr;

  for (const auto &context : contexts) {
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

GpgCommandExecutor::ExecuteContext::ExecuteContext(
    QStringList arguments, GpgCommandExecutorCallback callback,
    Module::TaskRunnerPtr task_runner, GpgCommandExecutorInterator int_func)
    : arguments(std::move(arguments)),
      cb_func(std::move(callback)),
      int_func(std::move(int_func)),
      task_runner(std::move(task_runner)) {}

GpgCommandExecutor::GpgCommandExecutor(int channel)
    : GpgFrontend::SingletonFunctionObject<GpgCommandExecutor>(channel) {}

auto PrepareContext(const GpgContext &ctx_, const QString &path,
                    const GpgCommandExecutor::ExecuteContext &context)
    -> std::tuple<bool, GpgCommandExecutor::ExecuteContext> {
  if (context.cmd.isEmpty() && path.isEmpty()) {
    LOG_E() << "failed to execute gpg command, gpg binary path is empty.";
    return {false, {}};
  }

  LOG_D() << "got path:" << path << "context channel:" << ctx_.GetChannel()
          << "home path: " << ctx_.HomeDirectory();

  GpgCommandExecutor::ExecuteContext ctx = {
      context.cmd.isEmpty() ? path : context.cmd,
      context.arguments,
      context.cb_func,
      context.task_runner,
      context.int_func,
  };

  if (!ctx.arguments.contains("--homedir") && !ctx_.HomeDirectory().isEmpty()) {
    ctx.arguments.prepend(QDir::toNativeSeparators((ctx_.HomeDirectory())));
    ctx.arguments.prepend("--homedir");
  }

  return {true, ctx};
}

auto PrepareExecuteSyncContext(
    const GpgContext &ctx_, const QString &path,
    const GpgCommandExecutor::ExecuteContext &context)
    -> std::tuple<int, QByteArray, QByteArray> {
  auto ctx = context;

  int pcs_exit_code;
  QByteArray pcs_stdout;
  QByteArray pcs_stderr;

  // proxy
  ctx.cb_func = [&](int exit_code, const QByteArray &out,
                    const QByteArray &err) {
    pcs_exit_code = exit_code;
    pcs_stdout = out;
    pcs_stderr = err;
  };

  auto [ret, ctx2] = PrepareContext(ctx_, path, ctx);
  if (ret) {
    GpgFrontend::GpgCommandExecutor::ExecuteSync(ctx2);
    return {pcs_exit_code, pcs_stdout, pcs_stderr};
  }
  return {-1, "invalid context", ""};
}

void PrepareExecuteAsyncContext(
    const GpgContext &ctx_, const QString &path,
    const GpgCommandExecutor::ExecuteContext &context) {
  auto [ret, ctx] = PrepareContext(ctx_, path, context);
  GpgFrontend::GpgCommandExecutor::ExecuteConcurrentlyAsync({ctx});
}

auto GpgCommandExecutor::GpgExecuteSync(const ExecuteContext &context)
    -> std::tuple<int, QByteArray, QByteArray> {
  return PrepareExecuteSyncContext(ctx_,
                                   Module::RetrieveRTValueTypedOrDefault<>(
                                       "core", "gpgme.ctx.app_path", QString{}),
                                   context);
}

auto GpgCommandExecutor::GpgConfExecuteSync(const ExecuteContext &context)

    -> std::tuple<int, QByteArray, QByteArray> {
  return PrepareExecuteSyncContext(
      ctx_,
      Module::RetrieveRTValueTypedOrDefault<>("core", "gpgme.ctx.gpgconf_path",
                                              QString{}),
      context);
}

void GpgCommandExecutor::GpgExecuteAsync(const ExecuteContext &context) {
  PrepareExecuteAsyncContext(ctx_,
                             Module::RetrieveRTValueTypedOrDefault<>(
                                 "core", "gpgme.ctx.app_path", QString{}),
                             context);
}

void GpgCommandExecutor::GpgConfExecuteAsync(const ExecuteContext &context) {
  PrepareExecuteAsyncContext(ctx_,
                             Module::RetrieveRTValueTypedOrDefault<>(
                                 "core", "gpgme.ctx.gpgconf_path", QString{}),
                             context);
}
}  // namespace GpgFrontend