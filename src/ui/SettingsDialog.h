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

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include "ui/GpgFrontendUI.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

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

  void slotGetServiceToken();

 signals:

  void signalRestartNeeded(bool needed);
};

class SendMailTab : public QWidget {
  Q_OBJECT

 public:
  explicit SendMailTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private slots:

  void slotCheckConnection();

  void slotCheckBoxSetEnableDisable(int state);

 private:
  QString appPath;
  QSettings settings;

  QCheckBox* enableCheckBox;

  QLineEdit* smtpAddress;
  QLineEdit* username;
  QLineEdit* password;
  QSpinBox* portSpin;
  QComboBox* connectionTypeComboBox;
  QLineEdit* defaultSender;

  QPushButton* checkConnectionButton;

 signals:

  void signalRestartNeeded(bool needed);
};

class AppearanceTab : public QWidget {
  Q_OBJECT

 public:
  // void setSettings();
  explicit AppearanceTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private:
  QString appPath;
  QSettings settings;

  QButtonGroup* iconStyleGroup;
  QRadioButton* iconSizeSmall;
  QRadioButton* iconSizeMedium;
  QRadioButton* iconSizeLarge;
  QButtonGroup* iconSizeGroup;
  QRadioButton* iconTextButton;
  QRadioButton* iconIconsButton;
  QRadioButton* iconAllButton;
  QSpinBox* infoBoardFontSizeSpin;
  QCheckBox* windowSizeCheckBox;

 signals:

  void signalRestartNeeded(bool needed);
};

class KeyserverTab : public QWidget {
  Q_OBJECT

 public:
  explicit KeyserverTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private:
  QString appPath;
  QSettings settings;
  QComboBox* comboBox;
  QLineEdit* newKeyServerEdit;
  QTableWidget* keyServerTable;
  QStringList keyServerStrList;

 private slots:

  void addKeyServer();

  void refreshTable();

 signals:

  void signalRestartNeeded(bool needed);
};

class AdvancedTab : public QWidget {
  Q_OBJECT

 public:
  explicit AdvancedTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private:
  QString appPath;
  QSettings settings;

  QCheckBox* steganoCheckBox;
  QCheckBox* autoPubkeyExchangeCheckBox;

 signals:

  void signalRestartNeeded(bool needed);
};

class GpgPathsTab : public QWidget {
  Q_OBJECT
 public:
  explicit GpgPathsTab(QWidget* parent = nullptr);

  void applySettings();

 private:
  static QString getRelativePath(const QString& dir1, const QString& dir2);

  QString appPath;
  QSettings settings;

  QString defKeydbPath; /** The default keydb path used by gpg4usb */
  QString accKeydbPath; /** The currently used keydb path */
  QLabel* keydbLabel;

  void setSettings();

 private slots:

  QString chooseKeydbDir();

  void setKeydbPathToDefault();
};

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  GeneralTab* generalTab;
  SendMailTab* sendMailTab;
  AppearanceTab* appearanceTab;
  KeyserverTab* keyserverTab;
  AdvancedTab* advancedTab;
  GpgPathsTab* gpgPathsTab;

  static QHash<QString, QString> listLanguages();

 public slots:

  void slotAccept();

 signals:

  void signalRestartNeeded(bool needed);

 private:
  QTabWidget* tabWidget;
  QDialogButtonBox* buttonBox;
  bool restartNeeded{};

  bool getRestartNeeded() const;

 private slots:

  void slotSetRestartNeeded(bool needed);
};

}  // namespace GpgFrontend::UI

#endif  // __SETTINGSDIALOG_H__
