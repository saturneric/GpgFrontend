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

#include "gpg/GpgContext.h"

#include <gpg-error.h>
#include <gpgme.h>

#include <functional>
#include <string>
#include <utility>

#include "GpgConstants.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace GpgFrontend {

/**
 * Constructor
 *  Set up gpgme-context, set paths to app-run path
 */
GpgContext::GpgContext(const GpgContextInitArgs& args) {
  static bool _first = true;

  if (_first) {
    /* Initialize the locale environment. */
    LOG(INFO) << "locale" << setlocale(LC_CTYPE, nullptr);
    gpgme_check_version(nullptr);
    gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
    gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif
    _first = false;
  }

  gpgme_ctx_t _p_ctx;
  check_gpg_error(gpgme_new(&_p_ctx));
  _ctx_ref = CtxRefHandler(_p_ctx);

  if (args.gpg_alone) {
    info.AppPath = args.gpg_path;
    auto err = gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                         info.AppPath.c_str(),
                                         info.DatabasePath.c_str());
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  }

  auto engine_info = gpgme_ctx_get_engine_info(*this);

  // Check ENV before running
  bool check_pass = false, find_openpgp = false, find_gpgconf = false,
       find_assuan = false, find_cms = false;
  while (engine_info != nullptr) {
    if (engine_info->protocol == GPGME_PROTOCOL_GPGCONF &&
        strcmp(engine_info->version, "1.0.0") != 0)
      find_gpgconf = true;
    if (engine_info->protocol == GPGME_PROTOCOL_OpenPGP &&
        strcmp(engine_info->version, "1.0.0") != 0) {
      find_openpgp = true;
      info.AppPath = engine_info->file_name;
      info.DatabasePath = "default", info.GnupgVersion = engine_info->version;
      DLOG(INFO) << "OpenPGP"
                 << std::string(engine_info->file_name == nullptr
                                    ? "null"
                                    : engine_info->file_name)
                 << std::string(engine_info->home_dir == nullptr
                                    ? "null"
                                    : engine_info->home_dir);
    }

    if (engine_info->protocol == GPGME_PROTOCOL_CMS &&
        strcmp(engine_info->version, "1.0.0") != 0)
      find_cms = true;
    if (engine_info->protocol == GPGME_PROTOCOL_ASSUAN) find_assuan = true;
    engine_info = engine_info->next;
  }

  if (find_gpgconf && find_openpgp && find_cms && find_assuan)
    check_pass = true;

  if (!check_pass) {
    this->good_ = false;
    return;
  } else {
    LOG(INFO) << "GnuPG version" << info.GnupgVersion;

    // Set Independent Database
    if (args.independent_database) {
      info.DatabasePath = args.db_path;
      LOG(INFO) << "gpg custom database path" << info.DatabasePath;
      auto err = gpgme_ctx_set_engine_info(
          _ctx_ref.get(), GPGME_PROTOCOL_OpenPGP, info.AppPath.c_str(),
          info.DatabasePath.c_str());
      assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
    }

    /** Setting the output type must be done at the beginning */
    /** think this means ascii-armor --> ? */
    gpgme_set_armor(*this, 1);
    // Speed up loading process
    gpgme_set_offline(*this, 1);

    if (info.GnupgVersion >= "2.0.0") {
      LOG(INFO) << "Using GnuPG 2.x";
      check_gpg_error(gpgme_set_keylist_mode(
          *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                     GPGME_KEYLIST_MODE_SIGS |
                     GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                     GPGME_KEYLIST_MODE_WITH_TOFU));
    } else {
      LOG(INFO) << "Using GnuPG 1.x";
      check_gpg_error(gpgme_set_keylist_mode(
          *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_SIGS |
                     GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                     GPGME_KEYLIST_MODE_WITH_TOFU));
    }
    good_ = true;
  }
}

bool GpgContext::good() const { return good_; }

void GpgContext::SetPassphraseCb(decltype(test_passphrase_cb) cb) const {
  gpgme_set_passphrase_cb(*this, cb, nullptr);
}

std::string GpgContext::getGpgmeVersion() {
  return {gpgme_check_version(nullptr)};
}

}  // namespace GpgFrontend