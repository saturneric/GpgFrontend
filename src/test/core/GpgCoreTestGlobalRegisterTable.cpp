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

#include "GFCoreTest.h"
#include "core/module/GlobalRegisterTableTreeModel.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::Test {

using Module::GlobalRegisterTableTreeModel;

namespace {

/**
 * @brief Find the row of a child node by its displayed key.
 */
auto FindChild(const GlobalRegisterTableTreeModel& model,
               const QModelIndex& parent, const QString& name) -> QModelIndex {
  for (auto row = 0; row < model.rowCount(parent); ++row) {
    const auto index = model.index(row, 0, parent);
    if (index.data(Qt::DisplayRole).toString() == name) return index;
  }
  return {};
}

/**
 * @brief Walk a dotted path from the model root.
 */
auto Resolve(const GlobalRegisterTableTreeModel& model, const QString& path)
    -> QModelIndex {
  QModelIndex index;
  for (const auto& segment : path.split('.')) {
    index = FindChild(model, index, segment);
    if (!index.isValid()) return {};
  }
  return index;
}

auto ValueAt(const GlobalRegisterTableTreeModel& model,
             const QModelIndex& index, int column) -> QVariant {
  return model.index(index.row(), column, model.parent(index))
      .data(Qt::DisplayRole);
}

constexpr int kColumnType = 1;
constexpr int kColumnValueType = 2;
constexpr int kColumnValue = 3;
constexpr int kColumnVersion = 4;

}  // namespace

TEST_F(GFCoreTest, GRTTreeModelExposesLeavesAndNamespaces) {
  Module::UpsertRTValue("test.grt.model", "alpha", QString("hello"));

  GlobalRegisterTableTreeModel model(Module::ModuleManager::GetInstance().GRT(),
                                     nullptr);
  model.Refresh();

  const auto ns = Resolve(model, "test.grt.model");
  ASSERT_TRUE(ns.isValid());
  EXPECT_EQ(ValueAt(model, ns, kColumnType).toString(), QString("Namespace"));

  // namespaces carry no value at all, they used to render as "<EMPTY>"
  EXPECT_TRUE(ValueAt(model, ns, kColumnValue).toString().isEmpty());
  EXPECT_TRUE(ValueAt(model, ns, kColumnValueType).toString().isEmpty());

  const auto leaf = Resolve(model, "test.grt.model.alpha");
  ASSERT_TRUE(leaf.isValid());
  EXPECT_EQ(ValueAt(model, leaf, kColumnType).toString(), QString("Leaf"));
  EXPECT_EQ(ValueAt(model, leaf, kColumnValue).toString(), QString("hello"));
  EXPECT_EQ(leaf.data(Module::kGRTPathRole).toString(),
            QString("test.grt.model.alpha"));
}

TEST_F(GFCoreTest, GRTTreeModelHumanisesValueTypes) {
  Module::UpsertRTValue("test.grt.types", "text", QString("v"));
  Module::UpsertRTValue("test.grt.types", "number", 42);
  Module::UpsertRTValue("test.grt.types", "flag", true);
  Module::UpsertRTValue("test.grt.types", "real", 1.5);

  GlobalRegisterTableTreeModel model(Module::ModuleManager::GetInstance().GRT(),
                                     nullptr);
  model.Refresh();

  const auto type_of = [&](const QString& key) -> QString {
    const auto index = Resolve(model, "test.grt.types." + key);
    EXPECT_TRUE(index.isValid());
    return ValueAt(model, index, kColumnValueType).toString();
  };

  EXPECT_EQ(type_of("text"), QString("String"));
  EXPECT_EQ(type_of("number"), QString("Integer"));
  EXPECT_EQ(type_of("flag"), QString("Boolean"));
  EXPECT_EQ(type_of("real"), QString("Number"));
}

TEST_F(GFCoreTest, GRTTreeModelRefreshPicksUpNewValuesAndVersions) {
  Module::UpsertRTValue("test.grt.refresh", "counter", 1);

  GlobalRegisterTableTreeModel model(Module::ModuleManager::GetInstance().GRT(),
                                     nullptr);
  model.Refresh();

  auto leaf = Resolve(model, "test.grt.refresh.counter");
  ASSERT_TRUE(leaf.isValid());
  EXPECT_EQ(ValueAt(model, leaf, kColumnValue).toInt(), 1);
  const auto first_version = ValueAt(model, leaf, kColumnVersion).toInt();

  // a value published after the snapshot is only visible after a refresh
  Module::UpsertRTValue("test.grt.refresh", "counter", 2);
  Module::UpsertRTValue("test.grt.refresh", "fresh", QString("new"));
  model.Refresh();

  leaf = Resolve(model, "test.grt.refresh.counter");
  ASSERT_TRUE(leaf.isValid());
  EXPECT_EQ(ValueAt(model, leaf, kColumnValue).toInt(), 2);
  EXPECT_EQ(ValueAt(model, leaf, kColumnVersion).toInt(), first_version + 1);

  EXPECT_TRUE(Resolve(model, "test.grt.refresh.fresh").isValid());
}

}  // namespace GpgFrontend::Test
