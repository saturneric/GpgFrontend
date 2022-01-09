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

#include "GlobalSettingStation.h"

#include <openssl/bio.h>
#include <openssl/pem.h>

#include <vmime/security/cert/openssl/X509Certificate_OpenSSL.hpp>
#include <vmime/vmime.hpp>

std::unique_ptr<GpgFrontend::UI::GlobalSettingStation>
    GpgFrontend::UI::GlobalSettingStation::_instance = nullptr;

GpgFrontend::UI::GlobalSettingStation&
GpgFrontend::UI::GlobalSettingStation::GetInstance() {
  if (_instance == nullptr) {
    _instance = std::make_unique<GlobalSettingStation>();
  }
  return *_instance;
}

void GpgFrontend::UI::GlobalSettingStation::Sync() noexcept {
  using namespace libconfig;
  try {
    ui_cfg.writeFile(ui_config_path.string().c_str());
    LOG(INFO) << _("Updated ui configuration successfully written to")
              << ui_config_path;

  } catch (const FileIOException& fioex) {
    LOG(ERROR) << _("I/O error while writing ui configuration file")
               << ui_config_path;
  }
}

GpgFrontend::UI::GlobalSettingStation::GlobalSettingStation() noexcept {
  using namespace boost::filesystem;
  using namespace libconfig;

  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);

  LOG(INFO) << _("App Path") << app_path;
  LOG(INFO) << _("App Configure Path") << app_configure_path;
  LOG(INFO) << _("App Data Path") << app_data_path;
  LOG(INFO) << _("App Log Path") << app_log_path;
  LOG(INFO) << _("App Locale Path") << app_locale_path;

  if (!is_directory(app_configure_path)) create_directory(app_configure_path);

  if (!is_directory(app_data_path)) create_directory(app_data_path);

  if (!is_directory(app_log_path)) create_directory(app_log_path);

  if (!is_directory(ui_config_dir_path)) create_directory(ui_config_dir_path);

  if (!exists(ui_config_path)) {
    try {
      this->ui_cfg.writeFile(ui_config_path.string().c_str());
      LOG(INFO) << _("UserInterface configuration successfully written to")
                << ui_config_path;

    } catch (const FileIOException& fioex) {
      LOG(ERROR)
          << _("I/O error while writing UserInterface configuration file")
          << ui_config_path;
    }
  } else {
    try {
      this->ui_cfg.readFile(ui_config_path.string().c_str());
      LOG(INFO) << _("UserInterface configuration successfully read from")
                << ui_config_path;
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

GpgFrontend::UI::GlobalSettingStation::~GlobalSettingStation() noexcept =
    default;
