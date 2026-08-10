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

#include "GpgAssuanHelper.h"

#include "core/function/gpg/GpgContext.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgAssuanHelper::GpgAssuanHelper(int channel)
    : GpgFrontend::SingletonFunctionObject<GpgAssuanHelper>(channel),
      gpgconf_path_(Module::RetrieveRTValueTypedOrDefault<>(
          "core", "gpgme.ctx.gpgconf_path", QString{})) {}

GpgAssuanHelper::~GpgAssuanHelper() = default;

auto GpgAssuanHelper::record_connect_failure(GpgComponentType type,
                                             GpgError err) -> GpgError {
  // Long enough that a broken environment stops costing anything, short enough
  // that an agent restarting underneath us is picked up without the user having
  // to reach for the maintenance buttons.
  constexpr std::chrono::milliseconds kInitialBackoff{1000};
  constexpr std::chrono::milliseconds kMaxBackoff{30000};

  auto& failure = connect_failures_[type];
  failure.err = err;
  failure.backoff = failure.backoff.count() == 0
                        ? kInitialBackoff
                        : std::min(failure.backoff * 2, kMaxBackoff);
  failure.retry_after = std::chrono::steady_clock::now() + failure.backoff;

  LOG_W() << "cannot reach component: " << component_type_to_q_string(type)
          << "err:" << CheckGpgError(err) << "- suppressing retries for"
          << failure.backoff.count() << "ms";

  return err;
}

auto GpgAssuanHelper::ConnectToSocket(GpgComponentType type) -> GpgError {
  if (ctx_map_.contains(type)) return GPG_ERR_NO_ERROR;

  // Replay a recent failure rather than paying for it again. Without this every
  // caller re-ran the whole probe -- including a blocking gpgconf spawn -- and
  // a single unreachable agent froze the interface for as long as the callers
  // kept coming.
  if (const auto it = connect_failures_.constFind(type);
      it != connect_failures_.constEnd() &&
      std::chrono::steady_clock::now() < it->retry_after) {
    return it->err;
  }

  auto socket_path = GpgCtx(ctx_).ComponentDirectory(type);
  if (socket_path.isEmpty()) {
    LOG_W() << "socket path of component: " << component_type_to_q_string(type)
            << " is empty";
    return record_connect_failure(type, GPG_ERR_ENOPKG);
  }

  QFileInfo info(socket_path);
  if (!info.exists()) {
    LOG_W() << "socket path is not exists: " << socket_path;

    LOG_W() << "launching component: " << component_type_to_q_string(type)
            << " by gpgconf, sockets: " << socket_path;
    launch_component(type);

    // QFileInfo caches its stat on first use, so without this the check below
    // would still see the socket as missing no matter how well the launch went.
    info.refresh();

    if (!info.exists()) {
      LOG_W() << "socket path is still not exists: " << socket_path
              << "abort...";
      return record_connect_failure(type, GPG_ERR_ENOTSOCK);
    }
  }

  gpgme_ctx_t ctx;
  auto err = gpgme_new(&ctx);
  if (err != GPG_ERR_NO_ERROR) {
    LOG_E() << "create assuan context failed, err:" << CheckGpgError(err);
    return record_connect_failure(type, err);
  }

  auto p_ctx = QSharedPointer<struct gpgme_context>(
      ctx, [](gpgme_ctx_t p) { gpgme_release(p); });

  err = gpgme_ctx_set_engine_info(p_ctx.get(), GPGME_PROTOCOL_ASSUAN,
                                  info.absoluteFilePath().toUtf8(), "");
  if (err != GPG_ERR_NO_ERROR) {
    LOG_W() << "failed to set gpgme assuan engine info:"
            << info.absoluteFilePath() << "err:" << CheckGpgError(err);
    return record_connect_failure(type, err);
  }

  err = gpgme_set_protocol(p_ctx.get(), GPGME_PROTOCOL_ASSUAN);
  if (err != GPG_ERR_NO_ERROR) {
    LOG_E() << "set gpgme protocol failed, err:" << CheckGpgError(err);
    return record_connect_failure(type, err);
  }

  LOG_D() << "connected to socket by assuan protocol: "
          << info.absoluteFilePath() << "channel:" << GetChannel();

  gpgme_error_t op_err;
  err = gpgme_op_assuan_transact_ext(p_ctx.get(), "GETINFO pid",
                                     simple_data_callback, nullptr, nullptr,
                                     nullptr, nullptr, nullptr, &op_err);
  if (err != GPG_ERR_NO_ERROR) {
    LOG_W() << "failed to test assuan connection, err:" << CheckGpgError(err)
            << "op_err: " << CheckGpgError(op_err);
    return record_connect_failure(type, err);
  }

  // Reached it: drop any accumulated backoff so the next outage starts counting
  // from scratch rather than inheriting an old escalation.
  connect_failures_.remove(type);

  ctx_map_[type] = p_ctx;

  // Now that the connection is live and cached, teach the agent where to put
  // pinentry. Must run after the ctx_map_ insert so the OPTION commands reuse
  // this same connection instead of recursing into ConnectToSocket.
  if (type == GpgComponentType::kGPG_AGENT) {
    update_pinentry_environment(type);
  }

  return err;
}

void GpgAssuanHelper::update_pinentry_environment(GpgComponentType type) {
  struct EnvOption {
    const char* assuan_key;
    const char* env_name;
  };

  // Mirrors gpg's send_pinentry_environment: whatever is set gets forwarded.
  // `display` makes the agent pick a graphical pinentry; `ttyname` is the
  // curses fallback for a headless session.
  static constexpr std::array<EnvOption, 6> kEnvOptions = {{
      {"ttyname", "GPG_TTY"},
      {"ttytype", "TERM"},
      {"display", "DISPLAY"},
      {"xauthority", "XAUTHORITY"},
      {"lc-ctype", "LC_CTYPE"},
      {"lc-messages", "LC_MESSAGES"},
  }};

  for (const auto& option : kEnvOptions) {
    auto value = qEnvironmentVariable(option.env_name);
    if (value.isEmpty()) continue;

    auto [err, _] = SendStatusCommand(
        type, QString("OPTION %1=%2").arg(option.assuan_key, value));
    if (err != GPG_ERR_NO_ERROR) {
      LOG_D() << "agent rejected pinentry OPTION" << option.assuan_key
              << "err:" << CheckGpgError(err);
    }
  }

  // A Wayland pinentry needs its socket path, which has no dedicated OPTION and
  // must be pushed into the agent's environment explicitly.
  auto wayland_display = qEnvironmentVariable("WAYLAND_DISPLAY");
  if (!wayland_display.isEmpty()) {
    SendStatusCommand(
        type, QString("OPTION putenv=WAYLAND_DISPLAY=%1").arg(wayland_display));
  }
}

auto GpgAssuanHelper::SendCommand(GpgComponentType type, const QString& command,
                                  DataCallback data_cb,
                                  InqueryCallback inquery_cb,
                                  StatusCallback status_cb) -> GpgError {
  return send_command(type, command, data_cb, inquery_cb, status_cb, 0);
}

auto GpgAssuanHelper::send_command(GpgComponentType type,
                                   const QString& command,
                                   const DataCallback& data_cb,
                                   const InqueryCallback& inquery_cb,
                                   const StatusCallback& status_cb, int depth)
    -> GpgError {
  if (!ctx_map_.contains(type)) {
    LOG_W() << "haven't connect to: " << component_type_to_q_string(type)
            << ", trying to make a connection";

    auto err = CheckGpgError(ConnectToSocket(type));
    if (err != GPG_ERR_NO_ERROR) return err;
  }

  auto context = SecureCreateSharedObject<AssuanCallbackContext>();
  context->self = this;
  context->data_cb = data_cb;
  context->status_cb = status_cb;
  context->inquery_cb = inquery_cb;

  LOG_D() << "sending assuan command: " << command;

  GpgError op_err;
  auto err = gpgme_op_assuan_transact_ext(
      ctx_map_[type].get(), command.toUtf8(), default_data_callback, &context,
      default_inquery_callback, &context, default_status_callback, &context,
      &op_err);

  if (err != GPG_ERR_NO_ERROR || op_err != GPG_ERR_NO_ERROR) {
    LOG_W() << "failed to send assuan command, err:" << CheckGpgError(err)
            << "op err: " << CheckGpgError(op_err);

    // Broken pipe: the cached connection died, so drop it and try once more on
    // a fresh one. Bounded, because an agent that keeps dropping the pipe used
    // to recurse here until the stack ran out.
    constexpr int kMaxReconnects = 1;
    if (CheckGpgError(op_err) == 32877 && depth < kMaxReconnects) {
      ctx_map_.remove(type);
      return send_command(type, command, data_cb, inquery_cb, status_cb,
                          depth + 1);
    }

    // `err` only reports whether the command round-tripped; the agent/scdaemon
    // reports the actual command failure (e.g. a card rejecting an unsupported
    // algorithm) in `op_err`, with `err` left clean. Returning `err` here would
    // swallow that and make a failed operation look successful, so surface
    // whichever error is set, preferring the transport error.
    return err != GPG_ERR_NO_ERROR ? err : op_err;
  }

  return err;
}

auto GpgAssuanHelper::SendStatusCommand(GpgComponentType type,
                                        const QString& command)
    -> std::tuple<GpgError, QStringList> {
  GpgAssuanHelper::DataCallback d_cb =
      [&](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "data callback of command " << command << ": " << ctx->buffer;

    return 0;
  };

  GpgAssuanHelper::InqueryCallback i_cb =
      [=](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "inquery callback of command: " << command << ": "
            << ctx->inquery_name << "args: " << ctx->inquery_args;
    return 0;
  };

  QStringList lines;
  GpgAssuanHelper::StatusCallback s_cb =
      [&](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "status callback of command: " << command << ":  " << ctx->status
            << "args: " << ctx->status_args;
    lines.append(QStringList{ctx->status, ctx->status_args}.join(' '));
    return 0;
  };

  auto ret = SendCommand(type, command, d_cb, i_cb, s_cb);
  return {ret, lines};
}

auto GpgAssuanHelper::SendDataCommand(GpgComponentType type,
                                      const QString& command)
    -> std::tuple<GpgError, QStringList> {
  QStringList lines;
  GpgAssuanHelper::DataCallback d_cb =
      [&](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "data callback of command " << command << ": " << ctx->buffer;
    lines.push_back(QString::fromUtf8(ctx->buffer));
    return 0;
  };

  GpgAssuanHelper::InqueryCallback i_cb =
      [=](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "inquery callback of command: " << command << ": "
            << ctx->inquery_name << "args: " << ctx->inquery_args;
    return 0;
  };

  GpgAssuanHelper::StatusCallback s_cb =
      [&](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "status callback of command: " << command << ":  "
            << ctx->status;
    return 0;
  };

  auto ret = SendCommand(type, command, d_cb, i_cb, s_cb);
  return {ret, lines};
}

auto GpgAssuanHelper::default_data_callback(void* opaque, const void* buffer,
                                            size_t length) -> gpgme_error_t {
  auto ctx = *static_cast<QSharedPointer<AssuanCallbackContext>*>(opaque);
  ctx->buffer.clear();
  ctx->buffer.append(static_cast<const char*>(buffer),
                     static_cast<qsizetype>(length));
  if (ctx->data_cb) ctx->data_cb(ctx);
  return GPG_ERR_NO_ERROR;
}

auto GpgAssuanHelper::default_status_callback(void* opaque, const char* status,
                                              const char* args)
    -> gpgme_error_t {
  auto ctx = *static_cast<QSharedPointer<AssuanCallbackContext>*>(opaque);
  ctx->status = QString::fromUtf8(status);
  ctx->status_args = QString::fromUtf8(args);
  if (ctx->status_cb) ctx->status_cb(ctx);
  return GPG_ERR_NO_ERROR;
}

// Note: Returning data is currently not implemented in GPGME.
auto GpgAssuanHelper::default_inquery_callback(void* opaque, const char* name,
                                               const char* args,
                                               gpgme_data_t* /*r_data*/)
    -> gpgme_error_t {
  auto ctx = *static_cast<QSharedPointer<AssuanCallbackContext>*>(opaque);
  ctx->inquery_name = QString::fromUtf8(name);
  ctx->inquery_args = QString::fromUtf8(args);
  if (ctx->status_cb) ctx->inquery_cb(ctx);
  return GPG_ERR_NO_ERROR;
}

void GpgAssuanHelper::launch_component(GpgComponentType type) {
  if (gpgconf_path_.isEmpty()) {
    LOG_W() << "gpgconf_path is not collected by initializing";
    return;
  }

  auto gpgconf_path = QFileInfo(gpgconf_path_).absoluteFilePath();
  LOG_D() << "assuan helper channel: " << GetChannel()
          << "gpgconf path: " << gpgconf_path;

  auto args = QStringList{};

  // Without this gpgconf launches a component for the *default* home directory
  // while the socket we are waiting on belongs to this channel's key database,
  // so the launch could never produce the socket being looked for. Every other
  // gpgconf call in the codebase passes it.
  const auto home_path = GpgCtx(ctx_).EngineHomePath();
  if (!home_path.isEmpty()) {
    args.append({"--homedir", QDir::toNativeSeparators(home_path)});
  }

  args.append({"--launch", component_type_to_q_string(type)});

  QProcess process;
  process.setProgram(gpgconf_path);
  process.setArguments(args);
  process.start();

  // Bounded explicitly: the default is 30s, and this runs on the caller's
  // thread, which is the GUI thread for most of the callers.
  constexpr int kLaunchTimeoutMs = 5000;
  if (!process.waitForFinished(kLaunchTimeoutMs)) {
    LOG_E() << "failed to execute gpgconf" << process.arguments();
    process.kill();
    process.waitForFinished(1000);
    return;
  }
}

auto GpgAssuanHelper::component_type_to_q_string(GpgComponentType type)
    -> QString {
  switch (type) {
    case GpgComponentType::kGPG_AGENT:
    case GpgComponentType::kGPG_AGENT_SSH:
      return "gpg-agent";
    case GpgComponentType::kDIRMNGR:
      return "dirmngr";
    case GpgComponentType::kKEYBOXD:
      return "keyboxd";
    default:
      return "all";
  }
}
auto GpgAssuanHelper::simple_data_callback(void* opaque, const void* buffer,
                                           size_t length) -> gpgme_error_t {
  return 0;
}

void GpgAssuanHelper::ResetAllConnections() {
  ctx_map_.clear();

  // Also forget the suppressed failures: this is the path the maintenance
  // actions take, and "restart the components" has to mean the next call really
  // tries again instead of replaying a cached error.
  connect_failures_.clear();
}
}  // namespace GpgFrontend