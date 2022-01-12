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

#ifndef GPGFRONTEND_GLOBALSETTINGSTATION_H
#define GPGFRONTEND_GLOBALSETTINGSTATION_H

#include <openssl/x509.h>

#include <nlohmann/json.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "GpgFrontendBuildInstallInfo.h"
#include "ui/GpgFrontendUI.h"

namespace vmime::security::cert {
class defaultCertificateVerifier;
class X509Certificate;
}  // namespace vmime::security::cert

namespace GpgFrontend::UI {

class GlobalSettingStation : public QObject {
  Q_OBJECT
 public:
  static GlobalSettingStation& GetInstance();

  GlobalSettingStation() noexcept;

  ~GlobalSettingStation() noexcept override;

  libconfig::Setting& GetUISettings() noexcept { return ui_cfg.getRoot(); }

  [[nodiscard]] boost::filesystem::path GetAppDir() const { return app_path; }

  [[nodiscard]] boost::filesystem::path GetLogDir() const {
    return app_log_path;
  }

  [[nodiscard]] boost::filesystem::path GetStandaloneDatabaseDir() const {
    auto db_path = app_configure_path / "db";
    if (!boost::filesystem::exists(db_path)) {
      boost::filesystem::create_directory(db_path);
    }
    return db_path;
  }

  [[nodiscard]] boost::filesystem::path GetStandaloneGpgBinDir() const {
    return app_resource_path / "gpg1.4" / "gpg";
  }

  [[nodiscard]] boost::filesystem::path GetLocaleDir() const {
    return app_locale_path;
  }

  [[nodiscard]] boost::filesystem::path GetResourceDir() const {
    return app_resource_path;
  }

  [[nodiscard]] boost::filesystem::path GetCertsDir() const {
    return app_resource_path / "certs";
  }

  [[nodiscard]] std::shared_ptr<
      vmime::security::cert::defaultCertificateVerifier>
  GetCertVerifier() const;

  void AddRootCert(const boost::filesystem::path& path);

  const std::vector<std::shared_ptr<X509>>& GetRootCerts();

  void ResetRootCerts() { root_certs_.clear(); }

  void SyncSettings() noexcept;

  std::string SaveDataObj(const std::string& _key, const nlohmann::json& value);

  std::optional<nlohmann::json> GetDataObject(const std::string& _key);

  std::optional<nlohmann::json> GetDataObjectByRef(const std::string& _ref);

 private:
  // Program Location
  boost::filesystem::path app_path = qApp->applicationDirPath().toStdString();

  // Program Data Location
  boost::filesystem::path app_data_path =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
          .toStdString();

  // Program Data Location
  boost::filesystem::path app_log_path = app_data_path / "logs";

  // object storage path
  boost::filesystem::path app_data_objs_path = app_data_path / "objs";

#ifdef LINUX_INSTALL_BUILD
  // Program Data Location
  boost::filesystem::path app_resource_path =
      boost::filesystem::path(APP_LOCALSTATE_PATH) / "gpgfrontend";
#else
  // Program Data Location
  boost::filesystem::path app_resource_path = RESOURCE_DIR_BOOST_PATH(app_path);
#endif

#ifdef LINUX_INSTALL_BUILD
  // Program Data Location
  boost::filesystem::path app_locale_path = std::string(APP_LOCALE_PATH);
#else
  // Program Data Location
  boost::filesystem::path app_locale_path = app_resource_path / "locales";
#endif

  // Program Configure Location
  boost::filesystem::path app_configure_path =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
          .toStdString();

  boost::filesystem::path app_secure_path = app_configure_path / "secure";

  boost::filesystem::path app_secure_key_path = app_secure_path / "app.key";

  // Configure File Directory Location
  boost::filesystem::path ui_config_dir_path =
      app_configure_path / "UserInterface";

  // UI Configure File Location
  boost::filesystem::path ui_config_path = ui_config_dir_path / "ui.cfg";

  libconfig::Config ui_cfg;

  std::vector<std::shared_ptr<X509>> root_certs_;

  std::random_device rd;

  std::mt19937 mt;

  QByteArray hash_key_;

  static std::unique_ptr<GlobalSettingStation> _instance;

  void init_app_secure_key();

  std::string generate_passphrase(int len);
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_GLOBALSETTINGSTATION_H
