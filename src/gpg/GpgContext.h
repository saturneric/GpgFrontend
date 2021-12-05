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

#ifndef __SGPGMEPP_CONTEXT_H__
#define __SGPGMEPP_CONTEXT_H__

#include "GpgConstants.h"

#include "GpgFunctionObject.h"
#include "GpgInfo.h"
#include "GpgModel.h"

namespace GpgFrontend {

/**
 * Custom Encapsulation of GpgME APIs
 */
class GpgContext : public SingletonFunctionObject<GpgContext> {
 public:
  explicit GpgContext(bool independent_database = false,
             std::string path = std::string(), int channel = 0);

  ~GpgContext() override = default;

  [[nodiscard]] bool good() const;

  [[nodiscard]] const GpgInfo& GetInfo() const { return info; }

  static std::string getGpgmeVersion();

  operator gpgme_ctx_t() const { return _ctx_ref.get(); }

 private:
  GpgInfo info;

  struct _ctx_ref_deletor {
    void operator()(gpgme_ctx_t _ctx) {
      if (_ctx != nullptr) gpgme_release(_ctx);
    }
  };

  using CtxRefHandler = std::unique_ptr<struct gpgme_context, _ctx_ref_deletor>;
  CtxRefHandler _ctx_ref = nullptr;

  bool good_ = true;

 public:
  static gpgme_error_t test_passphrase_cb(void* opaque, const char* uid_hint,
                                          const char* passphrase_info,
                                          int last_was_bad, int fd) {
    LOG(INFO) << "test_passphrase_cb Called";
    size_t res;
    char pass[] = "abcdefg\n";
    size_t pass_len = strlen(pass);
    size_t off = 0;

    (void)opaque;
    (void)uid_hint;
    (void)passphrase_info;
    (void)last_was_bad;

    do {
      res = gpgme_io_write(fd, &pass[off], pass_len - off);
      if (res > 0) off += res;
    } while (res > 0 && off != pass_len);

    return off == pass_len ? 0 : gpgme_error_from_errno(errno);
  }

  void SetPassphraseCb(decltype(test_passphrase_cb) func) const;
};
}  // namespace GpgFrontend

#endif  // __SGPGMEPP_CONTEXT_H__
