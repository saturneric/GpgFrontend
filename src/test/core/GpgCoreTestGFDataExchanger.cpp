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

#include <array>
#include <thread>

#include "core/model/GFDataExchanger.h"

namespace GpgFrontend::Test {

namespace {

auto WriteAll(const QSharedPointer<GFDataExchanger>& ex, const QByteArray& data)
    -> void {
  ex->Write(reinterpret_cast<const std::byte*>(data.constData()), data.size());
  ex->CloseWrite();
}

}  // namespace

// The bug this exists to prevent: a stream whose length is not a whole multiple
// of the reader's buffer lost its tail, because hitting end-of-stream part-way
// through a read reported nothing read at all. Every archive this carries ends
// on a partial block, so it truncated all of them.
TEST(GFDataExchangerTest, APartialFinalReadIsNotThrownAway) {
  auto ex = CreateStandardGFDataExchanger();
  const QByteArray payload("0123456789");

  std::thread writer([&]() { WriteAll(ex, payload); });

  std::array<std::byte, 4096> buffer{};
  QByteArray received;
  while (true) {
    const auto read = ex->Read(buffer.data(), buffer.size());
    if (read <= 0) break;
    received.append(reinterpret_cast<const char*>(buffer.data()),
                    static_cast<int>(read));
  }
  writer.join();

  EXPECT_EQ(received, payload);
}

TEST(GFDataExchangerTest, ReadingAFullBufferReturnsItAll) {
  auto ex = CreateStandardGFDataExchanger();
  const QByteArray payload(64, 'x');

  std::thread writer([&]() { WriteAll(ex, payload); });

  std::array<std::byte, 64> buffer{};
  const auto read = ex->Read(buffer.data(), buffer.size());
  writer.join();

  EXPECT_EQ(read, 64);
}

TEST(GFDataExchangerTest, AClosedEmptyStreamIsEndOfFile) {
  auto ex = CreateStandardGFDataExchanger();
  ex->CloseWrite();

  std::array<std::byte, 16> buffer{};
  EXPECT_EQ(ex->Read(buffer.data(), buffer.size()), 0);
}

TEST(GFDataExchangerTest, ReadingNothingAsksForNothing) {
  auto ex = CreateStandardGFDataExchanger();
  std::array<std::byte, 1> buffer{};

  // would otherwise block forever on a stream nobody is writing to
  EXPECT_EQ(ex->Read(buffer.data(), 0), 0);
}

}  // namespace GpgFrontend::Test
