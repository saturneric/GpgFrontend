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

#include "GlobalSettingStation.h"

#include <openssl/bio.h>
#include <openssl/pem.h>

#include <vmime/security/cert/openssl/X509Certificate_OpenSSL.hpp>
#include <vmime/vmime.hpp>

std::unique_ptr<GpgFrontend::UI::GlobalSettingStation>
    GpgFrontend::UI::GlobalSettingStation::instance_ = nullptr;

GpgFrontend::UI::GlobalSettingStation&
GpgFrontend::UI::GlobalSettingStation::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = std::make_unique<GlobalSettingStation>();
  }
  return *instance_;
}

void GpgFrontend::UI::GlobalSettingStation::SyncSettings() noexcept {
  using namespace libconfig;
  try {
    ui_cfg_.writeFile(ui_config_path_.string().c_str());
    LOG(INFO) << _("Updated ui configuration successfully written to")
              << ui_config_path_;

  } catch (const FileIOException& fioex) {
    LOG(ERROR) << _("I/O error while writing ui configuration file")
               << ui_config_path_;
  }
}

GpgFrontend::UI::GlobalSettingStation::GlobalSettingStation() noexcept {
  using namespace boost::filesystem;
  using namespace libconfig;

  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);

  LOG(INFO) << _("App Path") << app_path_;
  LOG(INFO) << _("App Configure Path") << app_configure_path_;
  LOG(INFO) << _("App Data Path") << app_data_path_;
  LOG(INFO) << _("App Log Path") << app_log_path_;
  LOG(INFO) << _("App Locale Path") << app_locale_path_;

  if (!is_directory(app_configure_path_)) create_directory(app_configure_path_);

  if (!is_directory(app_data_path_)) create_directory(app_data_path_);

  if (!is_directory(app_log_path_)) create_directory(app_log_path_);

  if (!is_directory(ui_config_dir_path_)) create_directory(ui_config_dir_path_);

  if (!is_directory(app_secure_path_)) create_directory(app_secure_path_);

  if (!exists(app_secure_key_path_)) {
    init_app_secure_key();
  }

  const auto key =
      GpgFrontend::read_all_data_in_file(app_secure_key_path_.string());
  hash_key_ = QCryptographicHash::hash(QByteArray::fromStdString(key),
                                       QCryptographicHash::Sha256);

  if (!exists(app_data_objs_path_)) create_directory(app_data_objs_path_);

  if (!exists(ui_config_path_)) {
    try {
      this->ui_cfg_.writeFile(ui_config_path_.string().c_str());
      LOG(INFO) << _("UserInterface configuration successfully written to")
                << ui_config_path_;

    } catch (const FileIOException& fioex) {
      LOG(ERROR)
          << _("I/O error while writing UserInterface configuration file")
          << ui_config_path_;
    }
  } else {
    try {
      this->ui_cfg_.readFile(ui_config_path_.string().c_str());
      LOG(INFO) << _("UserInterface configuration successfully read from")
                << ui_config_path_;
    } catch (const FileIOException& fioex) {
      LOG(ERROR) << _("I/O error while reading UserInterface configure file");
    } catch (const ParseException& pex) {
      LOG(ERROR) << _("Parse error at ") << pex.getFile() << ":"
                 << pex.getLine() << " - " << pex.getError();
    }
  }
}

void GpgFrontend::UI::GlobalSettingStation::AddRootCert(
    const boost::filesystem::path& path) {
  auto out_buffer = GpgFrontend::read_all_data_in_file(path.string());

  auto mem_bio = std::shared_ptr<BIO>(
      BIO_new_mem_buf(out_buffer.data(), static_cast<int>(out_buffer.size())),
      [](BIO* _p) { BIO_free(_p); });

  auto x509 = std::shared_ptr<X509>(
      PEM_read_bio_X509(mem_bio.get(), nullptr, nullptr, nullptr),
      [](X509* _p) { X509_free(_p); });

  if (!x509) return;

  root_certs_.push_back(x509);
}

vmime::shared_ptr<vmime::security::cert::defaultCertificateVerifier>
GpgFrontend::UI::GlobalSettingStation::GetCertVerifier() const {
  auto p_cv =
      vmime::make_shared<vmime::security::cert::defaultCertificateVerifier>();

  std::vector<vmime::shared_ptr<vmime::security::cert::X509Certificate>>
      _root_certs;
  for (const auto& cert : root_certs_) {
    _root_certs.push_back(
        std::make_shared<vmime::security::cert::X509Certificate_OpenSSL>(
            cert.get()));
  }
  return p_cv;
}

const std::vector<std::shared_ptr<X509>>&
GpgFrontend::UI::GlobalSettingStation::GetRootCerts() {
  return root_certs_;
}

std::string GpgFrontend::UI::GlobalSettingStation::generate_passphrase(
    int len) {
  std::uniform_int_distribution<int> dist(999, 99999);
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_str;
  tmp_str.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_str += alphanum[dist(mt_) % (sizeof(alphanum) - 1)];
  }

  return tmp_str;
}

void GpgFrontend::UI::GlobalSettingStation::init_app_secure_key() {
  GpgFrontend::write_buffer_to_file(app_secure_key_path_.string(),
                                    generate_passphrase(256));
  boost::filesystem::permissions(
      app_secure_key_path_,
      boost::filesystem::owner_read | boost::filesystem::owner_write);
}

std::string GpgFrontend::UI::GlobalSettingStation::SaveDataObj(
    const std::string& _key, const nlohmann::json& value) {
  std::string _hash_obj_key = {};
  if (_key.empty()) {
    _hash_obj_key =
        QCryptographicHash::hash(
            hash_key_ + QByteArray::fromStdString(
                            generate_passphrase(32) +
                            to_iso_extended_string(
                                boost::posix_time::second_clock::local_time())),
            QCryptographicHash::Sha256)
            .toHex()
            .toStdString();
  } else {
    _hash_obj_key =
        QCryptographicHash::hash(hash_key_ + QByteArray::fromStdString(_key),
                                 QCryptographicHash::Sha256)
            .toHex()
            .toStdString();
  }

  const auto obj_path = app_data_objs_path_ / _hash_obj_key;

  QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                            QAESEncryption::Padding::ISO);
  auto encoded =
      encryption.encode(QByteArray::fromStdString(to_string(value)), hash_key_);

  GpgFrontend::write_buffer_to_file(obj_path.string(), encoded.toStdString());

  return _key.empty() ? _hash_obj_key : std::string();
}

std::optional<nlohmann::json>
GpgFrontend::UI::GlobalSettingStation::GetDataObject(const std::string& _key) {
  try {
    auto _hash_obj_key =
        QCryptographicHash::hash(hash_key_ + QByteArray::fromStdString(_key),
                                 QCryptographicHash::Sha256)
            .toHex()
            .toStdString();

    const auto obj_path = app_data_objs_path_ / _hash_obj_key;

    if (!boost::filesystem::exists(obj_path)) {
      return {};
    }

    auto buffer = GpgFrontend::read_all_data_in_file(obj_path.string());
    auto encoded = QByteArray::fromStdString(buffer);

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);

    auto decoded =
        encryption.removePadding(encryption.decode(encoded, hash_key_));

    return nlohmann::json::parse(decoded.toStdString());
  } catch (...) {
    return {};
  }
}
std::optional<nlohmann::json>
GpgFrontend::UI::GlobalSettingStation::GetDataObjectByRef(
    const std::string& _ref) {
  if (_ref.size() != 64) return {};

  try {
    auto _hash_obj_key = _ref;
    const auto obj_path = app_data_objs_path_ / _hash_obj_key;

    if (!boost::filesystem::exists(obj_path)) return {};

    auto buffer = GpgFrontend::read_all_data_in_file(obj_path.string());
    auto encoded = QByteArray::fromStdString(buffer);

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);

    auto decoded =
        encryption.removePadding(encryption.decode(encoded, hash_key_));

    return nlohmann::json::parse(decoded.toStdString());
  } catch (...) {
    return {};
  }
}

GpgFrontend::UI::GlobalSettingStation::~GlobalSettingStation() noexcept =
    default;
