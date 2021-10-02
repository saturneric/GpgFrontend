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

#ifndef __ABOUTDIALOG_H__
#define __ABOUTDIALOG_H__

#include "gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"
namespace GpgFrontend::UI {

/**
 * @brief Class containing the main tab of about dialog
 *
 */
class InfoTab : public QWidget {
  Q_OBJECT

 public:
  explicit InfoTab(QWidget* parent = nullptr);
};

/**
 * @brief Class containing the translator tab of about dialog
 *
 */
class TranslatorsTab : public QWidget {
  Q_OBJECT

 public:
  explicit TranslatorsTab(QWidget* parent = nullptr);
};

/**
 * @brief Class containing the main tab of about dialog
 *
 */
class UpdateTab : public QWidget {
  Q_OBJECT

  QLabel* currentVersionLabel;

  QLabel* latestVersionLabel;

  QLabel* upgradeLabel;

  QProgressBar* pb;

  QString currentVersion;

 public:
  explicit UpdateTab(QWidget* parent = nullptr);

  void getLatestVersion();

 private slots:
  void processReplyDataFromUpdateServer(const QByteArray& data);
  ;

 signals:
  void replyFromUpdateServer(QByteArray data);
};

/**
 * @brief Class for handling the about dialog
 *
 */
class AboutDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AboutDialog(int defaultIndex, QWidget* parent);

 protected:
  void showEvent(QShowEvent* ev) override;

 private:
  UpdateTab* updateTab;
};

}  // namespace GpgFrontend::UI

#endif  // __ABOUTDIALOG_H__
