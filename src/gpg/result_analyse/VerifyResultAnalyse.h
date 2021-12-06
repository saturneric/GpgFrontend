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

#ifndef GPGFRONTEND_VERIFYRESULTANALYSE_H
#define GPGFRONTEND_VERIFYRESULTANALYSE_H

#include "ResultAnalyse.h"
#include "gpg/model/GpgKeySignature.h"

namespace GpgFrontend {

class VerifyResultAnalyse : public ResultAnalyse {
 public:
  explicit VerifyResultAnalyse(GpgError error, GpgVerifyResult result);

  gpgme_signature_t GetSignatures() const;

  GpgVerifyResult TakeChargeOfResult();

 private:
  void do_analyse();

 private:
  bool print_signer(std::stringstream &stream, gpgme_signature_t sign);

  GpgError error;
  GpgVerifyResult result;
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_VERIFYRESULTANALYSE_H
