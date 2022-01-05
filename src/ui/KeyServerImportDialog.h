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

#ifndef __KEY_SERVER_IMPORT_DIALOG_H__
#define __KEY_SERVER_IMPORT_DIALOG_H__

#include "KeyImportDetailDialog.h"
#include "gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

class KeyServerImportDialog : public QDialog {
  Q_OBJECT

 public:
  KeyServerImportDialog(bool automatic, QWidget* parent);

  explicit KeyServerImportDialog(QWidget* parent);

  void slotImport(const KeyIdArgsListPtr& keys);

  void slotImport(const QStringList& keyIds, const QUrl& keyserverUrl);

 signals:
  void signalKeyImported();

 private slots:

  void slotImport();

  void slotSearchFinished();

  void slotImportFinished(const QString& keyid);

  void slotSearch();

  void slotSaveWindowState();

 private:
  void createKeysTable();

  void setMessage(const QString& text, bool error);

  void importKeys(ByteArrayPtr in_data);

  void setLoading(bool status);

  QPushButton* createButton(const QString& text, const char* member);

  QComboBox* createComboBox();

  bool mAutomatic = false;

  QLineEdit* searchLineEdit{};
  QComboBox* keyServerComboBox{};
  QProgressBar* waitingBar;
  QLabel* searchLabel{};
  QLabel* keyServerLabel{};
  QLabel* message{};
  QLabel* icon{};
  QPushButton* closeButton{};
  QPushButton* importButton{};
  QPushButton* searchButton{};
  QTableWidget* keysTable{};
  QNetworkAccessManager* qnam{};
};

}  // namespace GpgFrontend::UI

#endif  // __KEY_SERVER_IMPORT_DIALOG_H__
