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

#include "core/function/gpg/GpgContext.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GpgAssuanHelper
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
    gpgme_ctx_t ctx;

    QByteArray buffer;
    QString status;
    QString status_args;
    QString inquery_name;
    QString inquery_args;

    DataCallback data_cb;
    InqueryCallback inquery_cb;
    StatusCallback status_cb;
  };

  /**
   * @brief Construct a new Gpg Assuan Helper object
   *
   * @param channel
   */
  explicit GpgAssuanHelper(int channel);

  /**
   * @brief Destroy the Gpg Assuan Helper object
   *
   */
  ~GpgAssuanHelper();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto ConnectToSocket(GpgComponentType) -> GpgError;

  /**
   * @brief
   *
   * @param type
   * @param command
   * @param data_cb
   * @param inquery_cb
   * @param status_cb
   * @return true
   * @return false
   */
  auto SendCommand(GpgComponentType type, const QString& command,
                   DataCallback data_cb, InqueryCallback inquery_cb,
                   StatusCallback status_cb) -> GpgError;

  /**
   * @brief
   *
   * @param type
   * @param command
   * @return std::tuple<bool, QStringList>
   */
  auto SendStatusCommand(GpgComponentType type, const QString& command)
      -> std::tuple<GpgError, QStringList>;

  /**
   * @brief
   *
   * @param type
   * @param command
   * @return auto
   */
  auto SendDataCommand(GpgComponentType type, const QString& command)
      -> std::tuple<GpgError, QStringList>;

  /**
   * @brief
   *
   */
  void ResetAllConnections();

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());

  QMap<GpgComponentType, QSharedPointer<struct gpgme_context>> ctx_map_;
  QString gpgconf_path_;

  /**
   * @brief
   *
   * @param type
   */
  void launch_component(GpgComponentType type);

  /**
   * @brief
   *
   * @param type
   * @return QString
   */
  static auto component_type_to_q_string(GpgComponentType type) -> QString;

  /**
   * @brief
   *
   * @param opaque
   * @param buffer
   * @param length
   * @return gpgme_error_t
   */
  static auto simple_data_callback(void* opaque, const void* buffer,
                                   size_t length) -> gpgme_error_t;

  /**
   * @brief
   *
   * @param opaque
   * @param buffer
   * @param length
   * @return gpgme_error_t
   */
  static auto default_data_callback(void* opaque, const void* buffer,
                                    size_t length) -> gpgme_error_t;

  /**
   * @brief
   *
   * @param opaque
   * @param status
   * @return gpgme_error_t
   */
  static auto default_status_callback(void* opaque, const char* status,
                                      const char* args) -> gpgme_error_t;

  /**
   * @brief
   *
   * @param opaque
   * @param inquery
   * @return gpgme_error_t
   */
  static auto default_inquery_callback(void* opaque, const char* name,
                                       const char* args, gpgme_data_t* r_data)
      -> gpgme_error_t;
};

};  // namespace GpgFrontend