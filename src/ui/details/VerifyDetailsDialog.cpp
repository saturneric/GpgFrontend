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

#include "VerifyDetailsDialog.h"

#include <boost/format.hpp>

namespace GpgFrontend::UI {

VerifyDetailsDialog::VerifyDetailsDialog(QWidget* parent, GpgError error,
                                         GpgVerifyResult result)
    : QDialog(parent), mResult(std::move(result)), error(error) {
  this->setWindowTitle(_("Signatures Details"));

  mainLayout = new QHBoxLayout();
  this->setLayout(mainLayout);

  slotRefresh();

  this->exec();
}

void VerifyDetailsDialog::slotRefresh() {
  mVbox = new QWidget();
  auto* mVboxLayout = new QVBoxLayout(mVbox);
  mainLayout->addWidget(mVbox);

  // Button Box for close button
  buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

  mVboxLayout->addWidget(new QLabel(QString::fromStdString(
      std::string(_("Status")) + ": " + gpgme_strerror(error))));

  auto sign = mResult->signatures;

  if (sign == nullptr) {
    mVboxLayout->addWidget(new QLabel(_("No valid input found")));
    mVboxLayout->addWidget(buttonBox);
    return;
  }

  // Get timestamp of signature of current text
  QDateTime timestamp;
  timestamp.setTime_t(sign->timestamp);

  // Set the title widget depending on sign status
  if (gpg_err_code(sign->status) == GPG_ERR_BAD_SIGNATURE) {
    mVboxLayout->addWidget(new QLabel(_("Error Validating signature")));
  } else if (mInputSignature != nullptr) {
    const auto info = (boost::format(_("File was signed on %1%")) %
                       QLocale::system().toString(timestamp).toStdString())
                          .str() +
                      "<br/>" + _("It Contains") + ": " + "<br/><br/>";
    mVboxLayout->addWidget(new QLabel(info.c_str()));
  } else {
    const auto info = (boost::format(_("Signed on %1%")) %
                       QLocale::system().toString(timestamp).toStdString())
                          .str() +
                      "<br/>" + _("It Contains") + ": " + "<br/><br/>";
    mVboxLayout->addWidget(new QLabel(info.c_str()));
  }
  // Add information box for every single key
  while (sign) {
    GpgSignature signature(sign);
    auto* sign_box = new VerifyKeyDetailBox(signature, this);
    sign = sign->next;
    mVboxLayout->addWidget(sign_box);
  }

  mVboxLayout->addWidget(buttonBox);
}

}  // namespace GpgFrontend::UI