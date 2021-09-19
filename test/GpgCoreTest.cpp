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
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "gpg/GpgConstants.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExportor.h"
#include "gpg/function/GpgKeyOpera.h"

TEST(GpgKeyTest, GpgCoreTest) {}
class GpgCoreTest : public ::testing::Test {
 protected:
  std::vector<GpgFrontend::StdBypeArrayPtr> secret_keys_;

  boost::filesystem::path parent_path =
      boost::dll::program_location().parent_path();

  boost::filesystem::path config_path = parent_path / "conf" / "core.cfg";

  GpgCoreTest() = default;

  virtual ~GpgCoreTest() = default;

  virtual void SetUp() {
    DLOG(INFO) << "SetUp called";
    using namespace libconfig;
    Config cfg;
    ASSERT_NO_THROW(cfg.readFile(config_path.c_str()));

    const Setting& root = cfg.getRoot();
    ASSERT_TRUE(root.exists("independent_database"));
    bool independent_database = true;
    ASSERT_TRUE(root.lookupValue("independent_database", independent_database));

    DLOG(INFO) << "SetUp independent_database";
    if (independent_database) {
      ASSERT_TRUE(root.exists("independent_db_path"));
      std::string relative_db_path;

      ASSERT_TRUE(root.lookupValue("independent_db_path", relative_db_path));

      auto db_path = parent_path / relative_db_path;
      if (!boost::filesystem::exists(db_path)) {
        boost::filesystem::create_directory(db_path);
      }

      DLOG(INFO) << "GpgFrontend::GpgContext::CreateInstance";
      GpgFrontend::GpgContext::CreateInstance(
          1, std::make_unique<GpgFrontend::GpgContext>(true, db_path.c_str()));
      DLOG(INFO) << "db_path " << db_path;
    }

    ASSERT_TRUE(root.exists("data_path"));
    std::string relative_data_path;
    ASSERT_TRUE(root.lookupValue("data_path", relative_data_path));
    auto data_path = parent_path / relative_data_path;
    ASSERT_TRUE(boost::filesystem::exists(data_path));

    if (root.exists("load_keys.private_keys")) {
      LOG(INFO) << "loading private keys";
      auto& private_keys = root.lookup("load_keys.private_keys");
      for (auto it = private_keys.begin(); it != private_keys.end(); it++) {
        ASSERT_TRUE(it->exists("filename"));
        std::string filename;
        it->lookupValue("filename", filename);
        auto data_file_path = data_path / filename;
        DLOG(INFO) << "private file path" << data_file_path.string();
        std::string data =
            GpgFrontend::read_all_data_in_file(data_file_path.string());
        secret_keys_.push_back(std::make_unique<std::string>(data));
      }
      LOG(INFO) << "loaded private keys";
    }

    GpgFrontend::GpgContext::GetInstance(1).SetPassphraseCb(
        GpgFrontend::GpgContext::test_passphrase_cb);
    for (auto& secret_key : secret_keys_) {
      GpgFrontend::GpgKeyImportExportor::GetInstance(1).ImportKey(
          std::move(secret_key));
    }
  }

  virtual void TearDown() {}

 private:
  void dealing_private_keys() {}

  void configure_independent_database() {}
};

TEST_F(GpgCoreTest, CoreInitTest) {
  auto& ctx = GpgFrontend::GpgContext::GetInstance(1);
  DLOG(INFO) << "CoreInitTest ctx DatabasePath " << ctx.GetInfo().DatabasePath;
  auto& ctx_default = GpgFrontend::GpgContext::GetInstance();
  DLOG(INFO) << "CoreInitTest ctx_default DatabasePath "
             << ctx_default.GetInfo().DatabasePath;
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(1).GetKey(
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(1).GetKey(
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(1).GetKey(
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(1).GetKey(
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
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(1).GetKey(
      "9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.good());
  auto keys = GpgFrontend::GpgKeyGetter::GetInstance(1).FetchKey();
  ASSERT_GE(keys->size(), 1);

  ASSERT_TRUE(find(keys->begin(), keys->end(), key) != keys->end());
}

TEST_F(GpgCoreTest, GpgKeyDeleteTest) {
  // GpgFrontend::GpgKeyOpera::GetInstance().DeleteKeys(
  //     std::move(std::make_unique<std::vector<std::string>>(
  //         1, "9490795B78F8AFE9F93BD09281704859182661FB")));
}
