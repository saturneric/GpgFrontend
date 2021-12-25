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

#ifndef _GPGKEYIMPORTEXPORTOR_H
#define _GPGKEYIMPORTEXPORTOR_H

#include <string>

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

class GpgImportedKey {
 public:
  std::string fpr;
  int import_status;
};

typedef std::list<GpgImportedKey> GpgImportedKeyList;

class GpgImportInformation {
 public:
  GpgImportInformation() = default;

  explicit GpgImportInformation(gpgme_import_result_t result) {
    if (result->unchanged) unchanged = result->unchanged;
    if (result->considered) considered = result->considered;
    if (result->no_user_id) no_user_id = result->no_user_id;
    if (result->imported) imported = result->imported;
    if (result->imported_rsa) imported_rsa = result->imported_rsa;
    if (result->unchanged) unchanged = result->unchanged;
    if (result->new_user_ids) new_user_ids = result->new_user_ids;
    if (result->new_sub_keys) new_sub_keys = result->new_sub_keys;
    if (result->new_signatures) new_signatures = result->new_signatures;
    if (result->new_revocations) new_revocations = result->new_revocations;
    if (result->secret_read) secret_read = result->secret_read;
    if (result->secret_imported) secret_imported = result->secret_imported;
    if (result->secret_unchanged) secret_unchanged = result->secret_unchanged;
    if (result->not_imported) not_imported = result->not_imported;
  }

  int considered = 0;
  int no_user_id = 0;
  int imported = 0;
  int imported_rsa = 0;
  int unchanged = 0;
  int new_user_ids = 0;
  int new_sub_keys = 0;
  int new_signatures = 0;
  int new_revocations = 0;
  int secret_read = 0;
  int secret_imported = 0;
  int secret_unchanged = 0;
  int not_imported = 0;
  GpgImportedKeyList importedKeys;
};

class GpgKeyImportExporter
    : public SingletonFunctionObject<GpgKeyImportExporter> {
 public:
  explicit GpgKeyImportExporter(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<GpgKeyImportExporter>(channel) {}

  GpgImportInformation ImportKey(StdBypeArrayPtr inBuffer);

  bool ExportKeys(KeyIdArgsListPtr& uid_list, ByteArrayPtr& out_buffer) const;

  bool ExportKeys(const KeyArgsList& keys, ByteArrayPtr& outBuffer) const;

  bool ExportKey(const GpgKey& key, ByteArrayPtr& out_buffer) const;

  bool ExportKeyOpenSSH(const GpgKey& key, ByteArrayPtr& out_buffer) const;

  bool ExportSecretKey(const GpgKey& key, ByteArrayPtr& outBuffer) const;

  bool ExportSecretKeyShortest(const GpgKey& key,
                               ByteArrayPtr& outBuffer) const;

 private:
  GpgContext& ctx =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
};

}  // namespace GpgFrontend

#endif  // _GPGKEYIMPORTEXPORTOR_H