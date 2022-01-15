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

#ifndef __VERIFYDETAILSDIALOG_H__
#define __VERIFYDETAILSDIALOG_H__

#include "ui/GpgFrontendUI.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/VerifyKeyDetailBox.h"

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class VerifyDetailsDialog : public QDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Verify Details Dialog object
   *
   * @param parent
   * @param error
   * @param result
   */
  explicit VerifyDetailsDialog(QWidget* parent, GpgError error,
                               GpgVerifyResult result);

 private slots:

  /**
   * @brief
   *
   */
  void slot_refresh();

 private:
  KeyList* key_list_;               ///<
  QHBoxLayout* main_layout_;        ///<
  QWidget* m_vbox_{};               ///<
  QByteArray* input_data_{};        ///<
  QByteArray* input_signature_{};   ///<
  QDialogButtonBox* button_box_{};  ///<
  GpgVerifyResult m_result_;        ///<
  gpgme_error_t error_;             ///<
};

}  // namespace GpgFrontend::UI

#endif  // __VERIFYDETAILSDIALOG_H__
