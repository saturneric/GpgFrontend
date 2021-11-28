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

#ifndef GPGFRONTEND_SETTINGSGENERAL_H
#define GPGFRONTEND_SETTINGSGENERAL_H

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {
class KeyList;

class GeneralTab : public QWidget {
  Q_OBJECT

 public:
  explicit GeneralTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private:
  QString appPath;
  QSettings settings;

  QComboBox* serverSelectBox;
  QCheckBox* saveCheckedKeysCheckBox;
  QCheckBox* importConfirmationCheckBox;
  QComboBox* langSelectBox;
  QComboBox* ownKeySelectBox;
  QPushButton* getServiceTokenButton;
  QLabel* serviceTokenLabel;
  QHash<QString, QString> lang;

  std::unordered_map<std::string, std::string> keyIds;
  std::vector<std::string> keyIdsList;
  std::string serviceToken;
  KeyList* mKeyList;

 private slots:

  void slotOwnKeyIdChanged();

  void slotLanguageChanged();

 signals:

  void signalRestartNeeded(bool needed);
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSGENERAL_H
