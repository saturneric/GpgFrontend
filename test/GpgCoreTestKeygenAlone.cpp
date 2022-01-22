/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgFrontendTest.h"
#include "gpg/GpgGenKeyInfo.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyOpera.h"

TEST_F(GpgCoreTest, GenerateKeyTestAlone) {
  auto& key_opera = GpgFrontend::GpgKeyOpera::GetInstance(gpg_alone_channel);
  auto keygen_info = std::make_unique<GpgFrontend::GenKeyInfo>(false, true);
  keygen_info->setName("foobar");
  keygen_info->setEmail("bar@gpgfrontend.pub");
  keygen_info->setComment("hello");
  keygen_info->setAlgo("rsa");
  keygen_info->setNonExpired(true);
  keygen_info->setNonPassPhrase(true);

  GpgFrontend::GpgGenKeyResult result = nullptr;
  auto err = GpgFrontend::check_gpg_error_2_err_code(
      key_opera.GenerateKey(keygen_info, result));
  ASSERT_EQ(err, GPG_ERR_NO_ERROR);

  auto fpr = result->fpr;
  ASSERT_FALSE(fpr == nullptr);

  auto key =
      GpgFrontend::GpgKeyGetter::GetInstance(gpg_alone_channel).GetKey(fpr);
  ASSERT_TRUE(key.good());
  key_opera.DeleteKey(fpr);
}

TEST_F(GpgCoreTest, GenerateKeyTestAlone_1) {
  auto& key_opera = GpgFrontend::GpgKeyOpera::GetInstance(gpg_alone_channel);
  auto keygen_info = std::make_unique<GpgFrontend::GenKeyInfo>(false, true);
  keygen_info->setName("foobar");
  keygen_info->setEmail("bar@gpgfrontend.pub");
  keygen_info->setComment("hello gpgfrontend");
  keygen_info->setAlgo("rsa");
  keygen_info->setNonExpired(false);
  keygen_info->setPassPhrase("abcdefg");
  keygen_info->setExpired(boost::posix_time::second_clock::local_time() +
                          boost::posix_time::hours(24));
  keygen_info->setNonPassPhrase(false);

  GpgFrontend::GpgGenKeyResult result = nullptr;
  auto err = GpgFrontend::check_gpg_error_2_err_code(
      key_opera.GenerateKey(keygen_info, result));
  ASSERT_EQ(err, GPG_ERR_NO_ERROR);

  auto fpr = result->fpr;
  ASSERT_FALSE(fpr == nullptr);

  auto key =
      GpgFrontend::GpgKeyGetter::GetInstance(gpg_alone_channel).GetKey(fpr);
  ASSERT_TRUE(key.good());
  key_opera.DeleteKey(fpr);
}

TEST_F(GpgCoreTest, GenerateKeyTestAlone_2) {
  auto& key_opera = GpgFrontend::GpgKeyOpera::GetInstance(gpg_alone_channel);
  auto keygen_info = std::make_unique<GpgFrontend::GenKeyInfo>(false, true);
  keygen_info->setName("foobar");
  keygen_info->setEmail("bar@gpgfrontend.pub");
  keygen_info->setComment("hi");
  keygen_info->setAlgo("rsa");
  keygen_info->setKeySize(3072);
  keygen_info->setNonExpired(true);
  keygen_info->setNonPassPhrase(false);
  keygen_info->setPassPhrase("abcdefg");

  GpgFrontend::GpgGenKeyResult result = nullptr;
  auto err = GpgFrontend::check_gpg_error_2_err_code(
      key_opera.GenerateKey(keygen_info, result));
  ASSERT_EQ(err, GPG_ERR_NO_ERROR);

  auto fpr = result->fpr;
  ASSERT_FALSE(fpr == nullptr);

  auto key =
      GpgFrontend::GpgKeyGetter::GetInstance(gpg_alone_channel).GetKey(fpr);
  ASSERT_TRUE(key.good());
  key_opera.DeleteKey(fpr);
}

TEST_F(GpgCoreTest, GenerateKeyTestAlone_3) {
  auto& key_opera = GpgFrontend::GpgKeyOpera::GetInstance(gpg_alone_channel);
  auto keygen_info = std::make_unique<GpgFrontend::GenKeyInfo>(false, true);
  keygen_info->setName("foo");
  keygen_info->setEmail("bar@gpgfrontend.pub");
  keygen_info->setComment("hello");
  keygen_info->setAlgo("rsa");
  keygen_info->setKeySize(4096);
  keygen_info->setNonExpired(true);
  keygen_info->setNonPassPhrase(false);
  keygen_info->setPassPhrase("abcdefg");

  GpgFrontend::GpgGenKeyResult result = nullptr;
  auto err = GpgFrontend::check_gpg_error_2_err_code(
      key_opera.GenerateKey(keygen_info, result));
  ASSERT_EQ(err, GPG_ERR_NO_ERROR);

  auto fpr = result->fpr;
  ASSERT_FALSE(fpr == nullptr);

  auto key =
      GpgFrontend::GpgKeyGetter::GetInstance(gpg_alone_channel).GetKey(fpr);
  ASSERT_TRUE(key.good());
  key_opera.DeleteKey(fpr);
}

TEST_F(GpgCoreTest, GenerateKeyTestAlone_4) {
  auto& key_opera = GpgFrontend::GpgKeyOpera::GetInstance(gpg_alone_channel);
  auto keygen_info = std::make_unique<GpgFrontend::GenKeyInfo>(false, true);
  keygen_info->setName("foobar");
  keygen_info->setEmail("bar@gpgfrontend.pub");
  keygen_info->setComment("hello");
  keygen_info->setAlgo("dsa");
  keygen_info->setNonExpired(true);
  keygen_info->setNonPassPhrase(false);
  keygen_info->setPassPhrase("abcdefg");

  GpgFrontend::GpgGenKeyResult result = nullptr;
  auto err = GpgFrontend::check_gpg_error_2_err_code(
      key_opera.GenerateKey(keygen_info, result));
  ASSERT_EQ(err, GPG_ERR_NO_ERROR);

  auto fpr = result->fpr;
  ASSERT_FALSE(fpr == nullptr);

  auto key =
      GpgFrontend::GpgKeyGetter::GetInstance(gpg_alone_channel).GetKey(fpr);
  ASSERT_TRUE(key.good());
  key_opera.DeleteKey(fpr);
}