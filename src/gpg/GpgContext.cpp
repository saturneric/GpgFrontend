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
GpgContext::GpgContext(const GpgContextInitArgs &args) : args_(args) {
  static bool _first = true;

  if (_first) {
    /* Initialize the locale environment. */
    LOG(INFO) << "locale" << setlocale(LC_CTYPE, nullptr);
    info_.GpgMEVersion = gpgme_check_version(nullptr);
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
    info_.AppPath = args.gpg_path;
    auto err = gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                         info_.AppPath.c_str(),
                                         info_.DatabasePath.c_str());
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  }

  auto engine_info = gpgme_ctx_get_engine_info(*this);
  // Check ENV before running
  bool check_passed = false, find_openpgp = false, find_gpgconf = false,
       find_cms = false;

  while (engine_info != nullptr) {
    if (!strcmp(engine_info->version, "1.0.0")) {
      engine_info = engine_info->next;
      continue;
    }

    DLOG(INFO) << gpgme_get_protocol_name(engine_info->protocol)
               << std::string(engine_info->file_name == nullptr
                                  ? "null"
                                  : engine_info->file_name)
               << std::string(engine_info->home_dir == nullptr
                                  ? "null"
                                  : engine_info->home_dir);

    switch (engine_info->protocol) {
      case GPGME_PROTOCOL_OpenPGP:
        find_openpgp = true;
        info_.AppPath = engine_info->file_name;
        info_.GnupgVersion = engine_info->version;
        break;
      case GPGME_PROTOCOL_CMS:
        find_cms = true;
        info_.CMSPath = engine_info->file_name;
        break;
      case GPGME_PROTOCOL_GPGCONF:
        find_gpgconf = true;
        info_.GpgConfPath = engine_info->file_name;
        break;
      case GPGME_PROTOCOL_ASSUAN:
        break;
      case GPGME_PROTOCOL_G13:
        break;
      case GPGME_PROTOCOL_UISERVER:
        break;
      case GPGME_PROTOCOL_SPAWN:
        break;
      case GPGME_PROTOCOL_DEFAULT:
        break;
      case GPGME_PROTOCOL_UNKNOWN:
        break;
    }
    engine_info = engine_info->next;
  }

  // conditional check
  if ((info_.GnupgVersion >= "2.0.0" && find_gpgconf && find_openpgp &&
       find_cms) ||
      (info_.GnupgVersion > "1.0.0" && find_gpgconf))
    check_passed = true;

  if (!check_passed) {
    this->good_ = false;
    LOG(ERROR) << "Env check failed";
    return;
  } else {
    DLOG(INFO) << "gnupg version" << info_.GnupgVersion;
    init_ctx();
    good_ = true;
  }
}

void GpgContext::init_ctx() {
  // Set Independent Database
  if (info_.GnupgVersion <= "2.0.0" && args_.independent_database) {
    info_.DatabasePath = args_.db_path;
    DLOG(INFO) << "custom key db path" << info_.DatabasePath;
    auto err = gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                         info_.AppPath.c_str(),
                                         info_.DatabasePath.c_str());
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    info_.DatabasePath = "default";
  }

  if (args_.ascii) {
    /** Setting the output type must be done at the beginning */
    /** think this means ascii-armor --> ? */
    gpgme_set_armor(*this, 1);
  }

  // Speed up loading process
  gpgme_set_offline(*this, 1);

  if (info_.GnupgVersion >= "2.0.0") {
    check_gpg_error(gpgme_set_keylist_mode(
        *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                   GPGME_KEYLIST_MODE_SIGS | GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                   GPGME_KEYLIST_MODE_WITH_TOFU));
  } else {
    check_gpg_error(gpgme_set_keylist_mode(
        *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_SIGS |
                   GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                   GPGME_KEYLIST_MODE_WITH_TOFU));
  }

  // for unit test
  if (args_.test_mode) {
    LOG(INFO) << "test mode";
    if (info_.GnupgVersion >= "2.1.0") SetPassphraseCb(test_passphrase_cb);
    gpgme_set_status_cb(*this, test_status_cb, nullptr);
  }
}

bool GpgContext::good() const { return good_; }

void GpgContext::SetPassphraseCb(gpgme_passphrase_cb_t cb) const {
  if (info_.GnupgVersion >= "2.1.0") {
    if (gpgme_get_pinentry_mode(*this) != GPGME_PINENTRY_MODE_LOOPBACK) {
      gpgme_set_pinentry_mode(*this, GPGME_PINENTRY_MODE_LOOPBACK);
    }
    gpgme_set_passphrase_cb(*this, cb, nullptr);
  } else {
    LOG(ERROR) << "Not supported for gnupg version" << info_.GnupgVersion;
  }
}

gpgme_error_t GpgContext::test_passphrase_cb(void *opaque, const char *uid_hint,
                                             const char *passphrase_info,
                                             int last_was_bad, int fd) {
  size_t res;
  std::string pass = "abcdefg\n";
  auto pass_len = pass.size();

  size_t off = 0;

  do {
    res = gpgme_io_write(fd, &pass[off], pass_len - off);
    if (res > 0) off += res;
  } while (res > 0 && off != pass_len);

  return off == pass_len ? 0 : gpgme_error_from_errno(errno);
  return 0;
}

gpgme_error_t GpgContext::test_status_cb(void *hook, const char *keyword,
                                         const char *args) {
  LOG(INFO) << "keyword" << keyword;
  return 0;
}

}  // namespace GpgFrontend