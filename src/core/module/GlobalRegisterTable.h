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

#include <any>
#include <functional>
#include <optional>

#include "core/typedef/CoreTypedef.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Module {

using Namespace = QString;
using Key = QString;
using LPCallback = std::function<void(Namespace, Key, int, std::any)>;

/**
 * @brief Hierarchical runtime key-value registry for inter-module communication.
 *
 * Values are stored in a tree indexed by (Namespace, Key) pairs. Modules can
 * publish typed values, retrieve them, subscribe to change notifications, and
 * enumerate child keys. Each publish increments a per-node version counter
 * and emits SignalPublish.
 */
class GlobalRegisterTable : public QObject {
  Q_OBJECT
 public:
  friend class GlobalRegisterTableTreeModel;

  /**
   * @brief Construct an empty register table.
   */
  GlobalRegisterTable();

  ~GlobalRegisterTable() override;

  /**
   * @brief Publish a value under the given namespace and key.
   *
   * Creates intermediate tree nodes as needed. Increments the node version
   * and emits SignalPublish.
   *
   * @param ns namespace string
   * @param key key string within the namespace
   * @param value typed value to store
   * @return true on success
   */
  auto PublishKV(Namespace ns, Key key, std::any value) -> bool;

  /**
   * @brief Retrieve the value stored under the given namespace and key.
   *
   * @param ns namespace string
   * @param key key string
   * @return the stored value if present, or empty if not found
   */
  auto LookupKV(Namespace ns, Key key) -> std::optional<std::any>;

  /**
   * @brief Subscribe to change notifications for a namespace/key pair.
   *
   * @p callback is invoked (via Qt signal) each time the value is published.
   * The subscription is automatically removed when @p obj is destroyed.
   *
   * @param obj QObject whose lifetime bounds the subscription
   * @param ns namespace string
   * @param key key string
   * @param callback function called with (namespace, key, version, value) on each publish
   * @return true if the subscription was registered successfully
   */
  auto ListenPublish(QObject* obj, Namespace ns, Key key, LPCallback callback)
      -> bool;

  /**
   * @brief List all direct child keys under the given namespace/key node.
   *
   * @param n namespace string
   * @param k parent key string
   * @return list of child key strings
   */
  auto ListChildKeys(Namespace n, Key k) -> QContainer<Key>;

 signals:
  /**
   * @brief Emitted whenever a value is published via PublishKV.
   *
   * @param ns namespace of the published value
   * @param key key of the published value
   * @param version new version number of the node
   * @param value the published value
   */
  void SignalPublish(Namespace ns, Key key, int version, std::any value);

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend::Module
