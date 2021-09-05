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

#ifndef __SGPGMEPP_CONTEXT_H__
#define __SGPGMEPP_CONTEXT_H__

#include "GpgFunctionObject.h"
#include "GpgInfo.h"
#include "GpgModel.h"

namespace GpgFrontend {

/**
 * Custom Encapsulation of GpgME APIs
 */
class GpgContext : public SingletonFunctionObject<GpgContext> {

public:
  GpgContext();

  ~GpgContext() override = default;

  [[nodiscard]] bool good() const;

  bool exportKeys(const QVector<GpgKey> &keys, QByteArray &outBuffer);

  const GpgInfo &GetInfo() const { return info; }

  void clearPasswordCache();

  /**
   * @details If text contains PGP-message, put a linebreak before the message,
   * so that gpgme can decrypt correctly
   *
   * @param in Pointer to the QBytearray to check.
   */
  static void preventNoDataErr(QByteArray *in);

  static std::string getGpgmeVersion();

  operator gpgme_ctx_t() const { return _ctx_ref.get(); }

private:
  GpgInfo info;

  using CtxRefHandler =
      std::unique_ptr<struct gpgme_context, std::function<void(gpgme_ctx_t)>>;
  CtxRefHandler _ctx_ref = nullptr;

  bool good_ = true;

  QByteArray mPasswordCache;

  static gpgme_error_t passphraseCb(void *hook, const char *uid_hint,
                                    const char *passphrase_info,
                                    int last_was_bad, int fd);

  gpgme_error_t passphrase(const char *uid_hint, const char *passphrase_info,
                           int last_was_bad, int fd);
};
} // namespace GpgFrontend

#endif // __SGPGMEPP_CONTEXT_H__
