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

#ifndef WIZARD_H
#define WIZARD_H

#include "SettingsDialog.h"
#include "gpg/GpgConstants.h"
#include "ui/GpgFrontendUI.h"
#include "ui/KeyMgmt.h"
#include "ui/keygen/KeygenDialog.h"

namespace GpgFrontend::UI {

class Wizard : public QWizard {
  Q_OBJECT
  Q_ENUMS(WizardPages)

 public:
  enum WizardPages {
    Page_Intro,
    Page_Choose,
    Page_ImportFromGpg4usb,
    Page_ImportFromGnupg,
    Page_GenKey,
    Page_Conclusion
  };

  Wizard(KeyMgmt* keyMgmt, QWidget* parent = nullptr);

 private:
  KeyMgmt* mKeyMgmt;
  QString appPath;
  QSettings settings;

 private slots:

  void slotWizardAccepted();

 signals:

  void signalOpenHelp(QString page);
};

class IntroPage : public QWizardPage {
  Q_OBJECT

 public:
  explicit IntroPage(QWidget* parent = nullptr);

  QHash<QString, QString> languages;

  [[nodiscard]] int nextId() const override;

 private:
  QString appPath;
  QSettings settings;

 private slots:

  void slotLangChange(const QString& lang);
};

class ChoosePage : public QWizardPage {
  Q_OBJECT

 public:
  explicit ChoosePage(QWidget* parent = nullptr);

 private slots:

  void slotJumpPage(const QString& page);

 private:
  [[nodiscard]] int nextId() const override;

  int nextPage;
};

class KeyGenPage : public QWizardPage {
  Q_OBJECT

 public:
  explicit KeyGenPage(QWidget* parent = nullptr);

  [[nodiscard]] int nextId() const override;

 private slots:

  void slotGenerateKeyDialog();
};

class ConclusionPage : public QWizardPage {
  Q_OBJECT

 public:
  explicit ConclusionPage(QWidget* parent = nullptr);

  [[nodiscard]] int nextId() const override;

 private:
  QCheckBox* dontShowWizardCheckBox;
  QCheckBox* openHelpCheckBox;
};

}  // namespace GpgFrontend::UI

#endif
