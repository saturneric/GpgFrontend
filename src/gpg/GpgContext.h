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

struct GpgContextInitArgs {
  // make no sense for gpg2
  bool independent_database = false;
  std::string db_path = {};
  bool gpg_alone = false;
  std::string gpg_path = {};
  bool test_mode = false;
  bool ascii = true;

  GpgContextInitArgs() = default;
};

/**
 * Custom Encapsulation of GpgME APIs
 */
class GpgContext : public SingletonFunctionObject<GpgContext> {
 public:
  explicit GpgContext(const GpgContextInitArgs& args = {});

  explicit GpgContext(int channel)
      : SingletonFunctionObject<GpgContext>(channel) {}

  ~GpgContext() override = default;

  [[nodiscard]] bool good() const;

  [[nodiscard]] const GpgInfo& GetInfo() const { return info_; }

  operator gpgme_ctx_t() const { return _ctx_ref.get(); }

 private:
  GpgInfo info_;
  GpgContextInitArgs args_;

  void init_ctx();

  struct _ctx_ref_deleter {
    void operator()(gpgme_ctx_t _ctx) {
      if (_ctx != nullptr) gpgme_release(_ctx);
    }
  };

  using CtxRefHandler = std::unique_ptr<struct gpgme_context, _ctx_ref_deleter>;
  CtxRefHandler _ctx_ref = nullptr;

  bool good_ = true;

 public:
  static gpgme_error_t test_passphrase_cb(void* opaque, const char* uid_hint,
                                          const char* passphrase_info,
                                          int last_was_bad, int fd);

  static gpgme_error_t test_status_cb(void* hook, const char* keyword,
                                      const char* args);

  void SetPassphraseCb(gpgme_passphrase_cb_t func) const;
};
}  // namespace GpgFrontend

#endif  // __SGPGMEPP_CONTEXT_H__
