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
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

static const char *test_private_key_data = R"(
-----BEGIN PGP PRIVATE KEY BLOCK-----

lQOYBGf5Yb0BCADY9VaMNtAKVt4Woy45PbBmTtSgr8gUZ32zOCypKv/Cnoc0o2xR
nyHSelh/GMImHPt7BFAFplK68+b7pRpWuWC+ljUi+MJliGDTJX2FFJpFTlhqROHO
6982f2OnTyDaCG+XCDtKktK6U29eK3Y+e08WN8o52fYNzYOQc7pfn428gt4Nx9s9
CJDUvdeZFbLj7UfN/0tCe+loiOHF13ZVpyIWiIFi06ANsBPut6CZRxlVJTgNNPZf
PJ813m83PId4w3ExvJ+ndeRDWE5MXqi8NmoRJGNtcS0/D2bnTkgqT89rWSfEkEuz
jOLUR3tzerh6xohwe0U86QuPRsiJFhL0NmMRABEBAAEAB/wMkk7FCioM51KwIh18
EKCHlRrKAtWHpSWJ/H/N/6FZUCFKggu1QRDPJuq41qDtX3GNA8d1RFl33sksHLmF
e/FoqcCDecGd64Scx4fZ7cMwr+T8p2gkOtOwzznhiHrBV4rLyBzTaeWPCWWjIcaU
wUVoZqwvpPsWeqmcdbA/eTnXyewsU48Bpd2mFGhtPl6ZrHtoj4gW7h4/sU1GwACn
GXzyeVzWU/HJJRLVSysnxOyU2wUX9NJSoFhz3j73hw+5dqovOph/IA7FEgDh5s+i
7MbvF9f7m+3o4FsNk964PS+6L8/5vfR6gHYNP8Kfy14Iab5UcLj0jjLTaZxQr/hB
yNqfBADlHg9+Bq7aJ9he5MCr1Hjj90bkuO4XVgGl4vzIFmNf9YXVoOGG7EY38Av0
XO9iKFFLG+uLDA9iWAps1GhefWM9Cfy1SRbZrPQAcWiX4cv8BLukYYDUucp1Gb0a
k1CUR3MSdZ5znv28lH5jZyX5jGhJjuXDea4Y9ZHEiTfHavre+wQA8moPCNTQEgX6
gPxo/gYuCgrEWy5L+t6vNjhQl5bs7XG3YjKKLHjS2zHe16/74C1K+t1n3vmbhQ0a
1mobf+AUwkTAgcvp0U/KzvUJccbfMsSHR3Wl4xnDAtzooEkKWzy82XEx8FFv3cQp
xbvF+isFYRSt6tcj7yemIsDaFeRd+GMEAOvpX8lwyVjGk2U4O2OFvbsJmyQ5MNN3
hZn3e9qUoeVvYvWEwszpqz1p8yHu/Y9SmMU/srZpFq1FARyAYZ0Y+HEPsY6qIzex
NZ4lL0Y/ADK2mOAnvd0EHU2jzizWmANzwnl2D5O1n2SbPRzPYQXb3OPjhFONSdUI
RqrieywWW5hYT8a0HEFEU0tUZXN0KCk8dGVzdGVyQGJrdHVzLmNvbT6JAVEEEwEI
ADsWIQRQlfHaU+FFX6pRe11cTYBUbrblLwUCZ/lhvQIbLwULCQgHAgIiAgYVCgkI
CwIEFgIDAQIeBwIXgAAKCRBcTYBUbrblLwvDCADY43HmTA4UwypSj6lOkC+5tOp9
wll4ph9qAO3lbuWeqLX1uV6XFH4NMvFO6uDQvNaRQFFja8HM3s3uzo0d0ltDNkNx
WroWeUTb1lyX3cPHoApo6jKbQ8jSJkk4+5Jd1/tL34teYJRgZqitTpdOWVc348/a
H3nzsWNgb6fyq2nvkBERV+Er2I2ZG33JKuAw5Qr0eO4CpktKqHGzXpR7R07LdZwF
uWQdeF3NPBl1KgBLJgZYUy2GmDMShaBL4sw1f4z8A21/7W8ld3Ma1eQSNStoQOxI
P8/Isa2Lcw9z0Xec1INfEmYQpGfTnRgJhfNlNHQZYgjIK20sETGvTTjJyPX3
=vR4I
-----END PGP PRIVATE KEY BLOCK-----
)";

TEST_F(GpgCoreTest, CoreAddADSKTestA) {
  auto info = GpgKeyImportExporter::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));
  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("5C4D80546EB6E52F");
  ASSERT_TRUE(key.IsGood());
  ASSERT_TRUE(key.IsPrivateKey());
  ASSERT_TRUE(key.IsHasMasterKey());

  auto key_b = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(key_b.IsGood());
  ASSERT_TRUE(key_b.IsPrivateKey());
  ASSERT_TRUE(key_b.IsHasMasterKey());

  auto key_b_subkeys = key_b.GetSubKeys();
  ASSERT_EQ(key_b_subkeys->size(), 2);

  auto [err, data_object] = GpgKeyOpera::GetInstance().AddADSKSync(key, key_b_subkeys->last());

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());

  GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKey("5C4D80546EB6E52F");
  ASSERT_TRUE(key.IsGood());
  ASSERT_TRUE(key.IsPrivateKey());
  ASSERT_TRUE(key.IsHasMasterKey());

  auto key_subkeys = key.GetSubKeys();
  ASSERT_EQ(key_subkeys->size(), 2);
  ASSERT_EQ(key_subkeys->last().GetID(), "F89C95A05088CC93");
  ASSERT_EQ(key_subkeys->last().IsADSK(), true);

  GpgKeyOpera::GetInstance().DeleteKey(key.GetId());
}
};