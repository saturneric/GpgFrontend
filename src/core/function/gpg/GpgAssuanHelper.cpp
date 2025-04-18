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

#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgAssuanHelper::GpgAssuanHelper(int channel)
    : GpgFrontend::SingletonFunctionObject<GpgAssuanHelper>(channel),
      gpgconf_path_(Module::RetrieveRTValueTypedOrDefault<>(
          "core", "gpgme.ctx.gpgconf_path", QString{})) {}

GpgAssuanHelper::~GpgAssuanHelper() {
  for (const auto& ctx : assuan_ctx_) {
    assuan_release(ctx);
  }
}

auto GpgAssuanHelper::ConnectToSocket(GpgComponentType type) -> GpgError {
  if (assuan_ctx_.contains(type)) return GPG_ERR_NO_ERROR;

  auto socket_path = ctx_.ComponentDirectory(type);
  if (socket_path.isEmpty()) {
    LOG_W() << "socket path of component: " << component_type_to_q_string(type)
            << " is empty";
    return GPG_ERR_ENOPKG;
  }

  QFileInfo info(socket_path);
  if (!info.exists()) {
    LOG_W() << "socket path is not exists: " << socket_path;

    LOG_W() << "launching component: " << component_type_to_q_string(type)
            << " by gpgconf, sockets: " << socket_path;
    launch_component(type);

    if (!info.exists()) {
      LOG_W() << "socket path is still not exists: " << socket_path
              << "abort...";
      return GPG_ERR_ENOTSOCK;
    }
  }

  assuan_context_t a_ctx;
  assuan_new(&a_ctx);

  auto err = assuan_socket_connect(a_ctx, info.absoluteFilePath().toUtf8(),
                                   ASSUAN_INVALID_PID, 0);
  if (err != GPG_ERR_NO_ERROR) {
    LOG_W() << "failed to connect to socket:" << CheckGpgError(err);
    return err;
  }

  LOG_D() << "connected to socket by assuan protocol: "
          << info.absoluteFilePath() << "channel:" << GetChannel();

  err = assuan_transact(a_ctx, "GETINFO pid", simple_data_callback, nullptr,
                        nullptr, nullptr, nullptr, nullptr);
  if (err != GPG_ERR_NO_ERROR) {
    LOG_W() << "failed to test assuan connection:" << CheckGpgError(err);
    return err;
  }

  assuan_ctx_[type] = a_ctx;
  return err;
}

auto GpgAssuanHelper::SendCommand(GpgComponentType type, const QString& command,
                                  DataCallback data_cb,
                                  InqueryCallback inquery_cb,
                                  StatusCallback status_cb) -> GpgError {
  if (!assuan_ctx_.contains(type)) {
    LOG_W() << "haven't connect to: " << component_type_to_q_string(type)
            << ", trying to make a connection";

    auto err = CheckGpgError(ConnectToSocket(type));
    if (err != GPG_ERR_NO_ERROR) return err;
  }

  auto context = QSharedPointer<AssuanCallbackContext>::create();
  context->self = this;
  context->data_cb = std::move(data_cb);
  context->status_cb = std::move(status_cb);
  context->inquery_cb = std::move(inquery_cb);

  LOG_D() << "sending assuan command: " << command;

  auto err = assuan_transact(
      assuan_ctx_[type], command.toUtf8(), default_data_callback, &context,
      default_inquery_callback, &context, default_status_callback, &context);

  if (err != GPG_ERR_NO_ERROR) {
    LOG_W() << "failed to send assuan command:" << CheckGpgError(err);

    // broken pipe error, try reconnect next time
    if (CheckGpgError(err) == 32877) {
      assuan_ctx_.remove(type);
    }
    return err;
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
            << ctx->inquery;
    return 0;
  };

  QStringList lines;
  GpgAssuanHelper::StatusCallback s_cb =
      [&](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "status callback of command: " << command << ":  "
            << ctx->status;
    lines.append(ctx->status);
    return 0;
  };

  auto ret = SendCommand(type, command, d_cb, i_cb, s_cb);
  return {ret, lines};
}

auto GpgAssuanHelper::SendDataCommand(GpgComponentType type,
                                      const QString& command)
    -> std::tuple<bool, QStringList> {
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
            << ctx->inquery;

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

auto GpgAssuanHelper::default_status_callback(void* opaque, const char* status)
    -> gpgme_error_t {
  auto ctx = *static_cast<QSharedPointer<AssuanCallbackContext>*>(opaque);
  ctx->status = QString::fromUtf8(status);
  if (ctx->status_cb) ctx->status_cb(ctx);
  return GPG_ERR_NO_ERROR;
}

auto GpgAssuanHelper::default_inquery_callback(
    void* opaque, const char* inquery) -> gpgme_error_t {
  auto ctx = *static_cast<QSharedPointer<AssuanCallbackContext>*>(opaque);
  ctx->inquery = QString::fromUtf8(inquery);
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

  QProcess process;
  process.setProgram(gpgconf_path);
  process.setArguments({"--launch", component_type_to_q_string(type)});
  process.start();

  if (!process.waitForFinished()) {
    LOG_E() << "failed to execute gpgconf" << process.arguments();
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
  LOG_D() << "assuan callback data: "
          << QByteArray::fromRawData(static_cast<const char*>(buffer), length);
  return 0;
}

auto GpgAssuanHelper::AssuanCallbackContext::SendData(const QByteArray& b) const
    -> gpg_error_t {
  return assuan_send_data(ctx, b.constData(), b.size());
}
}  // namespace GpgFrontend