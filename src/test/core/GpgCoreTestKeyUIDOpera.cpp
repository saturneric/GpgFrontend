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

#include "GpgCoreTest.h"
#include "core/GpgConstants.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/function/gpg/GpgUIDOperator.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/GpgUtils.h"

static const char *test_private_key_data = R"(
-----BEGIN PGP PRIVATE KEY BLOCK-----

lHQEZ0N2exMFK4EEAAoCAwTmBGWDMqzfdx1fkGjuWHP/R6F2ZvvlolcVNJAImWKl
ew0ncmSH1/pOQ7vAud9yPnkDZfhJpcKHl1G4q/Bu5YyiAAEA5sIYAJtXCFrI5upG
Lk8XmdqGCn5c8gKsWVUikrqdKkcOLrQbZmZmZmZmKGZmZmZmKTxmZmZmQGZmZi5m
ZmY+iJkEExMIAEEWIQQwn1gjkF+wDtnB6TLy2N+l8QneRwUCZ0N2ewIbIwUJA8Jm
8wULCQgHAgIiAgYVCgkICwIEFgIDAQIeBwIXgAAKCRDy2N+l8QneR2ptAQCRE6T3
HhNp7CHVk1DkxTIj4i7Mw1Em7Cwvctr8usPhBwD/YECZLMowPLNZO4GFhIH+1Etd
GZ2d0Gbb51DUlPZUO2K0HGdnZ2dnZyhnZ2dnZyk8Z2dnZ2dAZ2dnLmdnZz6ImQQT
EwgAQRYhBDCfWCOQX7AO2cHpMvLY36XxCd5HBQJnRjCtAhsjBQkDwmbzBQsJCAcC
AiICBhUKCQgLAgQWAgMBAh4HAheAAAoJEPLY36XxCd5HaD8BAPvBCdzFSeGXyFXh
4Sn4tVpwlnJOwzC0ECGYSieaNjocAQChWIXVinXAQ5U/oslqR2+Gg1s2o8gTKVbv
ZE6T+6Vvc7QgaGhoaGhoKGhoaGhoaGgpPGhoaGhoQGhoaGguaGhoaD6ImQQTEwgA
QRYhBDCfWCOQX7AO2cHpMvLY36XxCd5HBQJnRjC2AhsjBQkDwmbzBQsJCAcCAiIC
BhUKCQgLAgQWAgMBAh4HAheAAAoJEPLY36XxCd5HqE0A/R9wq+ZobC2Iztoudcg/
eKXp1hs1D9zv7R6KkLdbB4zXAP0ch5qZnrb+U/wIuhq+oOwJknMsD/njB263eUgb
AXNh2rQdeXl5eXkoeXl5eXl5KTx5eXl5eUB5eXl5Lnl5eT6ImQQTEwgAQRYhBDCf
WCOQX7AO2cHpMvLY36XxCd5HBQJnRjDBAhsjBQkDwmbzBQsJCAcCAiICBhUKCQgL
AgQWAgMBAh4HAheAAAoJEPLY36XxCd5HcycA/RwytAvY8ryaR4qUPN8g1NSX3Dv8
SHYYu2cZJuck+lVxAQDxcyljEIZKVqOpNfWRZyqcRvE8kr64PymJTAPYOVdBuA==
=4Hms
-----END PGP PRIVATE KEY BLOCK-----
)";

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreDeleteUIDTestA) {
  auto info = GpgKeyImportExporter::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key->IsGood());

  auto uids = key->UIDs();

  ASSERT_EQ(uids.size(), 4);
  ASSERT_EQ(uids[2].GetUID(), "gggggg(ggggg)<ggggg@ggg.ggg>");

  auto res = GpgUIDOperator::GetInstance().DeleteUID(key, 3);

  ASSERT_TRUE(res);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key != nullptr);

  uids = key->UIDs();

  ASSERT_EQ(uids.size(), 3);
  ASSERT_EQ(uids[2].GetUID(), "hhhhhh(hhhhhhh)<hhhhh@hhhh.hhhh>");

  GpgKeyOpera::GetInstance().DeleteKey(key);
  GpgKeyGetter::GetInstance().FlushKeyCache();
}

TEST_F(GpgCoreTest, CoreRevokeUIDTestA) {
  GpgKeyGetter::GetInstance().FlushKeyCache();
  auto info = GpgKeyImportExporter::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key != nullptr);

  auto uids = key->UIDs();

  ASSERT_EQ(uids.size(), 4);
  ASSERT_EQ(uids[2].GetUID(), "gggggg(ggggg)<ggggg@ggg.ggg>");

  auto res = GpgUIDOperator::GetInstance().RevokeUID(
      key, 3, 4, "H\nEEEEEL\n\n\n\nL   \n0\n");

  ASSERT_TRUE(res);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key != nullptr);

  uids = key->UIDs();

  ASSERT_EQ(uids.size(), 4);
  ASSERT_EQ(uids[2].GetUID(), "gggggg(ggggg)<ggggg@ggg.ggg>");
  ASSERT_TRUE(uids[2].GetRevoked());

  GpgKeyOpera::GetInstance().DeleteKey(key);
  GpgKeyGetter::GetInstance().FlushKeyCache();
}

}  // namespace GpgFrontend::Test