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
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

class GF_CORE_EXPORT CacheManager
    : public SingletonFunctionObject<CacheManager> {
 public:
  /**
   * @brief Construct a new Cache Manager object
   *
   * @param channel
   */
  explicit CacheManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Destroy the Cache Manager object
   *
   */
  ~CacheManager() override;

  /**
   * @brief
   *
   * @param key
   * @param value
   */
  void SaveCache(const QString& key, QString value, qint64 ttl = -1);

  /**
   * @brief
   *
   * @param key
   * @param value
   */
  void SaveSecCache(const QString& key, const GFBuffer& value, qint64 ttl = -1);

  /**
   * @brief
   *
   * @param key
   * @param value
   * @param flush
   */
  void SaveDurableCache(const QString& key, const QJsonDocument& value,
                        bool flush = false);

  /**
   * @brief
   *
   * @param key
   * @param value
   * @param flush
   */
  void SaveSecDurableCache(const QString& key, const GFBuffer& value,
                           bool flush = false);

  /**
   * @brief
   *
   * @param key
   * @param value
   */
  auto LoadCache(const QString& key) -> QString;

  /**
   * @brief
   *
   * @param key
   * @param value
   */
  auto LoadSecCache(const QString& key) -> GFBuffer;

  /**
   * @brief
   *
   * @param key
   * @return QJsonDocument
   */
  auto LoadSecDurableCache(const QString& key) -> GFBuffer;

  /**
   * @brief
   *
   * @param key
   * @param default_value
   * @return QJsonDocument
   */
  auto LoadSecDurableCache(const QString& key, const GFBuffer& default_value)
      -> GFBuffer;

  /**
   * @brief
   *
   * @param key
   * @return QJsonDocument
   */
  auto LoadDurableCache(const QString& key) -> QJsonDocument;

  /**
   * @brief
   *
   * @param key
   * @param default_value
   * @return QJsonDocument
   */
  auto LoadDurableCache(const QString& key, QJsonDocument default_value)
      -> QJsonDocument;

  /**
   * @brief
   *
   * @param key
   * @return auto
   */
  void ResetCache(const QString& key);

  /**
   * @brief
   *
   * @param key
   * @return true
   * @return false
   */
  auto ResetDurableCache(const QString& key) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend
