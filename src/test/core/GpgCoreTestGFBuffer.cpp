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

#include <cstddef>

#include "core/model/GFBuffer.h"

namespace GpgFrontend::Test {

TEST(GFBufferTest, ConstructFromSize) {
  GFBuffer buf(16);
  EXPECT_EQ(buf.Size(), 16);
  EXPECT_FALSE(buf.Empty());
}

TEST(GFBufferTest, ConstructFromQByteArray) {
  QByteArray arr("hello", 5);
  GFBuffer buf(arr);
  EXPECT_EQ(buf.Size(), 5);
  EXPECT_EQ(memcmp(buf.Data(), arr.constData(), 5), 0);
}

TEST(GFBufferTest, ConstructFromQString) {
  QString str = QStringLiteral("hello world");
  GFBuffer buf(str);
  EXPECT_EQ(buf.Size(), str.toUtf8().size());
  EXPECT_EQ(buf.ConvertToQString(), str);
}

TEST(GFBufferTest, ConstructFromCharPointer) {
  const char* data = "abcdefg";
  GFBuffer buf(data);
  EXPECT_EQ(buf.Size(), strlen(data));
  EXPECT_EQ(memcmp(buf.Data(), data, strlen(data)), 0);
}

TEST(GFBufferTest, CopyAndMove) {
  GFBuffer a("12345");
  GFBuffer b(a);  // Copy constructor
  EXPECT_EQ(a, b);
}

TEST(GFBufferTest, AssignmentOperator) {
  GFBuffer a("foo");
  GFBuffer b;
  b = a;
  EXPECT_EQ(b, "foo");
}

TEST(GFBufferTest, EqualityOperators) {
  GFBuffer a("bar");
  GFBuffer b("bar");
  GFBuffer c("baz");
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
  EXPECT_TRUE(a != c);
}

TEST(GFBufferTest, AppendBuffer) {
  GFBuffer a("hello");
  GFBuffer b(", world");
  a.Append(b);
  EXPECT_EQ(a, "hello, world");
}

TEST(GFBufferTest, AppendSelf) {
  GFBuffer a("abc");
  a.Append(a);
  EXPECT_EQ(a, "abcabc");
}

TEST(GFBufferTest, AppendCharPointer) {
  GFBuffer a("abc");
  a.Append("def", 3);
  EXPECT_EQ(a, "abcdef");
}

TEST(GFBufferTest, ResizeIncrease) {
  GFBuffer a("12345");
  a.Resize(10);
  EXPECT_EQ(a.Size(), 10);
  EXPECT_EQ(memcmp(a.Data(), "12345", 5), 0);
}

TEST(GFBufferTest, ResizeDecrease) {
  GFBuffer a("abcdef");
  a.Resize(3);
  EXPECT_EQ(a.Size(), 3);
  EXPECT_EQ(memcmp(a.Data(), "abc", 3), 0);
}

TEST(GFBufferTest, Zeroize) {
  GFBuffer a("secretdata");
  a.Zeroize();
  // After Zeroize, content should be all zero, but size remains
  bool all_zero = true;
  for (size_t i = 0; i < a.Size(); ++i) {
    if (a.Data()[i] != 0) {
      all_zero = false;
      break;
    }
  }
  EXPECT_TRUE(all_zero);
}

TEST(GFBufferTest, LeftMidRight) {
  GFBuffer a("abcdefgh");
  EXPECT_EQ(a.Left(3), "abc");
  EXPECT_EQ(a.Mid(2, 4), "cdef");
  EXPECT_EQ(a.Right(2), "gh");
}

TEST(GFBufferTest, ConvertToQByteArray) {
  std::string data = "testdata";
  GFBuffer buf(data.c_str(), data.size());
  QByteArray arr = buf.ConvertToQByteArray();
  EXPECT_EQ(arr, QByteArray::fromStdString(data));
}

TEST(GFBufferTest, ConvertToQString) {
  QString s = QStringLiteral("你好world");
  GFBuffer buf(s);
  EXPECT_EQ(buf.ConvertToQString(), s);
}

TEST(GFBufferTest, LessOperator) {
  GFBuffer a("aaa");
  GFBuffer b("bbb");
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

// Test self assignment safety
TEST(GFBufferTest, SelfAssignment) {
  GFBuffer a("abc");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
  a = a;
#pragma GCC diagnostic pop
  EXPECT_EQ(a, "abc");
}

// Test move semantics
TEST(GFBufferTest, MoveConstructorAndMoveAssignment) {
  GFBuffer a("abc");
  GFBuffer b(std::move(a));
  EXPECT_TRUE(a.Empty());
  EXPECT_EQ(b, "abc");

  GFBuffer c("def");
  c = std::move(b);
  EXPECT_TRUE(b.Empty());
  EXPECT_EQ(c, "abc");
}

// Test extreme size
TEST(GFBufferTest, LargeBuffer) {
  size_t sz = static_cast<size_t>(1024 * 1024);
  GFBuffer a(sz);
  memset(a.Data(), 0x7f, sz);
  for (size_t i = 0; i < sz; ++i) {
    ASSERT_EQ((unsigned char)a.Data()[i], 0x7f);
  }
}

// Test append empty
TEST(GFBufferTest, AppendEmpty) {
  GFBuffer a("abc");
  GFBuffer empty;
  a.Append(empty);
  EXPECT_EQ(a, "abc");
}

// Test resize to zero and back
TEST(GFBufferTest, ResizeZeroAndBack) {
  GFBuffer a("abc");
  a.Resize(0);
  EXPECT_TRUE(a.Empty());
  a.Resize(5);
  EXPECT_EQ(a.Size(), 5);
}

// Test append to zero-size buffer
TEST(GFBufferTest, AppendToEmptyBuffer) {
  GFBuffer a;
  GFBuffer b("data");
  a.Append(b);
  EXPECT_EQ(a, "data");
}

// Test append null/zero pointer
TEST(GFBufferTest, AppendNullptr) {
  GFBuffer a("abc");
  a.Append(nullptr, 0);
  EXPECT_EQ(a, "abc");
}

// Test destructive zeroize after Resize
TEST(GFBufferTest, ZeroizeAfterResize) {
  GFBuffer a("secret");
  a.Resize(10);
  a.Zeroize();
  bool all_zero = true;
  for (size_t i = 0; i < a.Size(); ++i) {
    if (a.Data()[i] != 0) all_zero = false;
  }
  EXPECT_TRUE(all_zero);
}

// Test double Zeroize (should not crash)
TEST(GFBufferTest, DoubleZeroize) {
  GFBuffer a("secure");
  a.Zeroize();
  a.Zeroize();
  SUCCEED();
}

// Test behavior after move
TEST(GFBufferTest, MoveAfterZeroize) {
  GFBuffer a("hello");
  a.Zeroize();
  GFBuffer b(std::move(a));
  EXPECT_TRUE(a.Empty());
  EXPECT_TRUE(b.Size() == 5);
}

}  // namespace GpgFrontend::Test