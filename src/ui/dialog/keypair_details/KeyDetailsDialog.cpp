/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "KeyDetailsDialog.h"

#include "core/function/gpg/GpgAttributeHelper.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/keypair_details/KeyPairDetailTab.h"
#include "ui/dialog/keypair_details/KeyPairOperaTab.h"
#include "ui/dialog/keypair_details/KeyPairPhotosTab.h"
#include "ui/dialog/keypair_details/KeyPairSubkeyTab.h"
#include "ui/dialog/keypair_details/KeyPairUIDTab.h"

namespace {
constexpr int kMinDialogWidth = 680;
constexpr int kMinDialogHeight = 560;
}  // namespace

namespace GpgFrontend::UI {

KeyDetailsDialog::KeyDetailsDialog(int channel, const GpgKeyPtr& key,
                                   QWidget* parent)
    : GeneralDialog(typeid(KeyDetailsDialog).name(), parent),
      current_gpg_context_channel_(channel) {
  tab_widget_ = new QTabWidget();
  tab_widget_->addTab(
      new KeyPairDetailTab(current_gpg_context_channel_, key, tab_widget_),
      tr("KeyPair"));

  auto attrs =
      GpgAttributeHelper(current_gpg_context_channel_).GetAttributes(key->ID());

  bool has_photos = false;
  has_photos = std::any_of(
      attrs.begin(), attrs.end(),
      [](const GpgFrontend::GpgAttrInfo& info) { return info.ext == "jpg"; });

  if (!key->IsRevoked()) {
    tab_widget_->addTab(
        new KeyPairUIDTab(current_gpg_context_channel_, key, tab_widget_),
        tr("UIDs"));

    // Add Photos tab if there are photo attributes
    if (has_photos) {
      tab_widget_->addTab(new KeyPairPhotosTab(current_gpg_context_channel_,
                                               key, attrs, tab_widget_),
                          tr("Photo IDs"));
    }

    tab_widget_->addTab(
        new KeyPairSubkeyTab(current_gpg_context_channel_, key, tab_widget_),
        tr("Keychain"));
    tab_widget_->addTab(
        new KeyPairOperaTab(current_gpg_context_channel_, key, tab_widget_),
        tr("Operations"));
  }

  QString m_key_id = key->ID();
  connect(UISignalStation::GetInstance(), &UISignalStation::SignalKeyRevoked,
          this, [this, m_key_id](const QString& key_id) {
            if (key_id == m_key_id) this->close();
          });

  auto* main_layout = new QVBoxLayout;
  main_layout->addWidget(tab_widget_);

#ifdef Q_OS_MACOS
  setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

  this->setAttribute(Qt::WA_DeleteOnClose, true);
  this->setLayout(main_layout);
  this->setWindowTitle(QString(tr("Key Details") + " (Key DB Index: %1)")
                           .arg(current_gpg_context_channel_));
  this->setModal(true);

  // Commit the size before the native window is mapped. Relying on the size
  // hint alone maps the window smaller than the tab pages need, and the
  // correction lands after the first frame is already on screen. A saved
  // geometry still wins: GeneralDialog restores it right before the map.
  this->ensurePolished();
  main_layout->activate();
  this->resize(sizeHint().expandedTo(QSize(kMinDialogWidth, kMinDialogHeight)));

  this->show();
  this->raise();
  this->activateWindow();
}
}  // namespace GpgFrontend::UI
