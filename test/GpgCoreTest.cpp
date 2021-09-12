/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include <gpg-error.h>
#include <gtest/gtest.h>
#include <boost/date_time/gregorian/parsers.hpp>
#include <boost/dll.hpp>
#include <memory>

#include "gpg/GpgConstants.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExportor.h"
#include "gpg/function/GpgKeyOpera.h"

TEST(GpgKeyTest, GpgCoreTest) {}
class GpgCoreTest : public ::testing::Test {
 protected:
  GpgFrontend::StdBypeArrayPtr secret_key_ = std::make_unique<std::string>(
      "-----BEGIN PGP PRIVATE KEY BLOCK-----\n"
      "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"
      "T2R+xoMRGcJJphKWpVjZ"
      "=u+uG"
      "\n-----END PGP PRIVATE KEY BLOCK-----\n");

  GpgCoreTest() = default;

  virtual ~GpgCoreTest() = default;

  virtual void SetUp() {
    auto config_path =
        boost::dll::program_location().parent_path() / "conf" / "core.cfg";

    using namespace libconfig;
    Config cfg;
    ASSERT_NO_THROW(cfg.readFile(config_path.c_str()));

    const Setting& root = cfg.getRoot();
    ASSERT_TRUE(root.exists("independent_database"));
    bool independent_database = true;
    ASSERT_TRUE(root.lookupValue("independent_database", independent_database));
    if (independent_database)

      GpgFrontend::GpgContext::GetInstance().SetPassphraseCb(
          GpgFrontend::GpgContext::test_passphrase_cb);
    GpgFrontend::GpgKeyImportExportor::GetInstance().ImportKey(
        std::move(this->secret_key_));
  }

  virtual void TearDown() {}

 private:
  void dealing_private_keys() {}

  void configure_independent_database() {}
};

TEST_F(GpgCoreTest, CoreInitTest) {
  auto& ctx = GpgFrontend::GpgContext::GetInstance();
  ASSERT_TRUE(ctx.good());
}

TEST_F(GpgCoreTest, GpgDataTest) {
  auto data_buff = std::string(
      "cqEh8fyKWtmiXrW2zzlszJVGJrpXDDpzgP7ZELGxhfZYFi8rMrSVKDwrpFZBSWMG");

  GpgFrontend::GpgData data(data_buff.data(), data_buff.size());

  auto out_buffer = data.Read2Buffer();
  LOG(INFO) << "in_buffer size " << data_buff.size();
  LOG(INFO) << "out_buffer size " << out_buffer->size();
  ASSERT_EQ(out_buffer->size(), 64);
}

TEST_F(GpgCoreTest, GpgKeyTest) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(
      "9490795B78F8AFE9F93BD09281704859182661FB");
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(
      "9490795B78F8AFE9F93BD09281704859182661FB");
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(
      "9490795B78F8AFE9F93BD09281704859182661FB");
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(
      "9490795B78F8AFE9F93BD09281704859182661FB");
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(
      "9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.good());
  auto keys = GpgFrontend::GpgKeyGetter::GetInstance().FetchKey();
  ASSERT_GE(keys->size(), 1);

  ASSERT_TRUE(find(keys->begin(), keys->end(), key) != keys->end());
}

TEST_F(GpgCoreTest, GpgKeyDeleteTest) {
  // GpgFrontend::GpgKeyOpera::GetInstance().DeleteKeys(
  //     std::move(std::make_unique<std::vector<std::string>>(
  //         1, "9490795B78F8AFE9F93BD09281704859182661FB")));
}
