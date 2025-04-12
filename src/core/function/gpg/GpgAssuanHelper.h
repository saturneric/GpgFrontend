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

#include <assuan.h>

#include "core/function/gpg/GpgContext.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GpgAssuanHelper
    : public SingletonFunctionObject<GpgAssuanHelper> {
 public:
  struct AssuanCallbackContext;

  using DataCallback =
      std::function<gpg_error_t(QSharedPointer<AssuanCallbackContext>)>;
  using InqueryCallback =
      std::function<gpg_error_t(QSharedPointer<AssuanCallbackContext>)>;
  using StatusCallback =
      std::function<gpg_error_t(QSharedPointer<AssuanCallbackContext>)>;

  struct AssuanCallbackContext {
    GpgAssuanHelper* self;
    GpgComponentType component_type;
    assuan_context_t ctx;

    QByteArray buffer;
    QString status;
    QString inquery;

    DataCallback data_cb;
    InqueryCallback inquery_cb;
    StatusCallback status_cb;

    [[nodiscard]] auto SendData(const QByteArray& b) const -> gpg_error_t;
  };

  explicit GpgAssuanHelper(int channel);
  ~GpgAssuanHelper();

  auto ConnectToSocket(GpgComponentType) -> bool;

  auto SendCommand(GpgComponentType type, const QString& command,
                   DataCallback data_cb, InqueryCallback inquery_cb,
                   StatusCallback status_cb) -> bool;

  auto SendStatusCommand(GpgComponentType type, const QString& command)
      -> std::tuple<bool, QStringList>;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
  QMap<GpgComponentType, assuan_context_t> assuan_ctx_;

  void launch_component(GpgComponentType type);

  static auto component_type_to_q_string(GpgComponentType type) -> QString;

  static auto simple_data_callback(void* opaque, const void* buffer,
                                   size_t length) -> gpgme_error_t;

  static auto default_data_callback(void* opaque, const void* buffer,
                                    size_t length) -> gpgme_error_t;

  static auto default_status_callback(void* opaque,
                                      const char* status) -> gpgme_error_t;

  static auto default_inquery_callback(void* opaque,
                                       const char* inquery) -> gpgme_error_t;

  QByteArray temp_data_;
  QString temp_status_;
  QString gpgconf_path_;
};

};  // namespace GpgFrontend