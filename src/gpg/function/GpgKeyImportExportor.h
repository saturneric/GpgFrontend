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

#ifndef _GPGKEYIMPORTEXPORTOR_H
#define _GPGKEYIMPORTEXPORTOR_H

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

class GpgImportedKey {
public:
  std::string fpr;
  int importStatus;
};

typedef std::list<GpgImportedKey> GpgImportedKeyList;

class GpgImportInformation {
public:
  GpgImportInformation() = default;

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

class GpgKeyImportExportor
    : public SingletonFunctionObject<GpgKeyImportExportor> {
public:
  GpgImportInformation ImportKey(StdBypeArrayPtr inBuffer);

  bool ExportKeys(KeyIdArgsListPtr &uid_list, BypeArrayPtr &out_buffer) const;

  bool ExportKeys(KeyArgsList &keys, BypeArrayPtr &outBuffer) const;

  bool ExportSecretKey(const GpgKey &key, BypeArrayPtr outBuffer) const;

private:
  GpgContext &ctx = GpgContext::GetInstance();
};

} // namespace GpgFrontend

#endif // _GPGKEYIMPORTEXPORTOR_H