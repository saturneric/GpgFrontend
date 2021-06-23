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

#ifndef __KEYSERVERIMPORTDIALOG_H__
#define __KEYSERVERIMPORTDIALOG_H__

#include "gpg/GpgContext.h"
#include "KeyImportDetailDialog.h"
#include "ui/widgets/KeyList.h"


class KeyServerImportDialog : public QDialog {
Q_OBJECT

public:
    KeyServerImportDialog(GpgME::GpgContext *ctx, KeyList *keyList, bool automatic,
                          QWidget *parent);

    KeyServerImportDialog(GpgME::GpgContext *ctx, QWidget *parent);

    void slotImport(const QStringList& keyIds);

    void slotImport(const QStringList& keyIds, const QUrl& keyserverUrl);

    void slotImportKey(const QVector<GpgKey>& keys);

private slots:

    void slotImport();

    void slotSearchFinished();

    void slotImportFinished();

    void slotSearch();

private:
    void createKeysTable();

    void setMessage(const QString &text, bool error);

    void importKeys(QByteArray inBuffer);

    void setLoading(bool status);

    QPushButton *createButton(const QString &text, const char *member);

    QComboBox *createComboBox();

    bool mAutomatic;

    QString appPath;
    QSettings settings;

    GpgME::GpgContext *mCtx;
    KeyList *mKeyList{};
    QLineEdit *searchLineEdit{};
    QComboBox *keyServerComboBox{};
    QProgressBar *waitingBar;
    QLabel *searchLabel{};
    QLabel *keyServerLabel{};
    QLabel *message{};
    QLabel *icon{};
    QPushButton *closeButton{};
    QPushButton *importButton{};
    QPushButton *searchButton{};
    QTableWidget *keysTable{};
    QNetworkAccessManager *qnam{};

};

#endif // __KEYSERVERIMPORTDIALOG_H__
