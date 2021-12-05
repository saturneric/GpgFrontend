/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "GpgFrontendTest.h"
#include "gpg/function/GpgKeyGetter.h"

// Should be used once and once-only
INITIALIZE_EASYLOGGINGPP

TEST_F(GpgCoreTest, CoreInitTest) {
  auto& ctx = GpgFrontend::GpgContext::GetInstance(default_channel);
  auto& ctx_default = GpgFrontend::GpgContext::GetInstance();
  ASSERT_TRUE(ctx.good());
  ASSERT_TRUE(ctx_default.good());
  ASSERT_EQ(ctx_default.GetInfo().DatabasePath, "default");
}

TEST_F(GpgCoreTest, GpgDataTest) {
  auto data_buff = std::string(
      "cqEh8fyKWtmiXrW2zzlszJVGJrpXDDpzgP7ZELGxhfZYFi8rMrSVKDwrpFZBSWMG");

  GpgFrontend::GpgData data(data_buff.data(), data_buff.size());

  auto out_buffer = data.Read2Buffer();
  ASSERT_EQ(out_buffer->size(), 64);
}

TEST_F(GpgCoreTest, GpgKeyTest) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(default_channel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.good());
  ASSERT_TRUE(key.is_private_key());
  ASSERT_TRUE(key.has_master_key());

  ASSERT_FALSE(key.disabled());
  ASSERT_FALSE(key.revoked());

  ASSERT_EQ(key.protocol(), "OpenPGP");

  ASSERT_EQ(key.subKeys()->size(), 2);
  ASSERT_EQ(key.uids()->size(), 1);

  ASSERT_TRUE(key.can_certify());
  ASSERT_TRUE(key.can_encrypt());
  ASSERT_TRUE(key.can_sign());
  ASSERT_FALSE(key.can_authenticate());
  ASSERT_TRUE(key.CanEncrActual());
  ASSERT_TRUE(key.CanEncrActual());
  ASSERT_TRUE(key.CanSignActual());
  ASSERT_FALSE(key.CanAuthActual());

  ASSERT_EQ(key.name(), "GpgFrontendTest");
  ASSERT_TRUE(key.comment().empty());
  ASSERT_EQ(key.email(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(key.id(), "81704859182661FB");
  ASSERT_EQ(key.fpr(), "9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_EQ(key.expires(), boost::gregorian::from_simple_string("2023-09-05"));
  ASSERT_EQ(key.pubkey_algo(), "RSA");
  ASSERT_EQ(key.length(), 3072);
  ASSERT_EQ(key.last_update(),
            boost::gregorian::from_simple_string("1970-01-01"));
  ASSERT_EQ(key.create_time(),
            boost::gregorian::from_simple_string("2021-09-05"));

  ASSERT_EQ(key.owner_trust(), "Unknown");

  using namespace boost::posix_time;
  ASSERT_EQ(key.expired(), key.expires() < second_clock::local_time().date());
}

TEST_F(GpgCoreTest, GpgSubKeyTest) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(default_channel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto sub_keys = key.subKeys();
  ASSERT_EQ(sub_keys->size(), 2);

  auto& sub_key = sub_keys->back();

  ASSERT_FALSE(sub_key.revoked());
  ASSERT_FALSE(sub_key.disabled());
  ASSERT_EQ(sub_key.timestamp(),
            boost::gregorian::from_simple_string("2021-09-05"));

  ASSERT_FALSE(sub_key.is_cardkey());
  ASSERT_TRUE(sub_key.is_private_key());
  ASSERT_EQ(sub_key.id(), "2B36803235B5E25B");
  ASSERT_EQ(sub_key.fpr(), "50D37E8F8EE7340A6794E0592B36803235B5E25B");
  ASSERT_EQ(sub_key.length(), 3072);
  ASSERT_EQ(sub_key.pubkey_algo(), "RSA");
  ASSERT_FALSE(sub_key.can_certify());
  ASSERT_FALSE(sub_key.can_authenticate());
  ASSERT_FALSE(sub_key.can_sign());
  ASSERT_TRUE(sub_key.can_encrypt());
  ASSERT_EQ(key.expires(), boost::gregorian::from_simple_string("2023-09-05"));

  using namespace boost::posix_time;
  ASSERT_EQ(sub_key.expired(),
            sub_key.expires() < second_clock::local_time().date());
}

TEST_F(GpgCoreTest, GpgUIDTest) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(default_channel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto uids = key.uids();
  ASSERT_EQ(uids->size(), 1);
  auto& uid = uids->front();

  ASSERT_EQ(uid.name(), "GpgFrontendTest");
  ASSERT_TRUE(uid.comment().empty());
  ASSERT_EQ(uid.email(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(uid.uid(), "GpgFrontendTest <gpgfrontend@gpgfrontend.pub>");
  ASSERT_FALSE(uid.invalid());
  ASSERT_FALSE(uid.revoked());
}

TEST_F(GpgCoreTest, GpgKeySignatureTest) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(default_channel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto uids = key.uids();
  ASSERT_EQ(uids->size(), 1);
  auto& uid = uids->front();

  auto signatures = uid.signatures();
  ASSERT_EQ(signatures->size(), 1);
  auto& signature = signatures->front();

  ASSERT_EQ(signature.name(), "GpgFrontendTest");
  ASSERT_TRUE(signature.comment().empty());
  ASSERT_EQ(signature.email(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(signature.keyid(), "81704859182661FB");
  ASSERT_EQ(signature.pubkey_algo(), "RSA");

  ASSERT_FALSE(signature.revoked());
  ASSERT_FALSE(signature.invalid());
  ASSERT_EQ(GpgFrontend::check_gpg_error_2_err_code(signature.status()),
            GPG_ERR_NO_ERROR);
  ASSERT_EQ(signature.uid(), "GpgFrontendTest <gpgfrontend@gpgfrontend.pub>");
}

TEST_F(GpgCoreTest, GpgKeyGetterTest) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(default_channel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.good());
  auto keys =
      GpgFrontend::GpgKeyGetter::GetInstance(default_channel).FetchKey();
  ASSERT_GE(keys->size(), secret_keys_.size());
  ASSERT_TRUE(find(keys->begin(), keys->end(), key) != keys->end());
}
