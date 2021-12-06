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

#include "MainWindow.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

void MainWindow::slotAbout() { new AboutDialog(0, this); }

void MainWindow::slotCheckUpdate() { new AboutDialog(2, this); }

void MainWindow::slotSetStatusBarText(const QString& text) {
  statusBar()->showMessage(text, 20000);
}

void MainWindow::slotStartWizard() {
  auto* wizard = new Wizard(this);
  wizard->show();
  wizard->setModal(true);
}

void MainWindow::slotImportKeyFromEdit() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) return;
  CommonUtils::GetInstance()->slotImportKeys(
      this, edit->curTextPage()->toPlainText().toStdString());
}

void MainWindow::slotOpenKeyManagement() {
  auto* dialog = new KeyMgmt(this);
  dialog->show();
  dialog->raise();
}

void MainWindow::slotOpenFileTab() { edit->slotNewFileTab(); }

void MainWindow::slotDisableTabActions(int number) {
  bool disable;

  if (number == -1)
    disable = true;
  else
    disable = false;

  if (edit->curFilePage() != nullptr) {
    disable = true;
  }

  printAct->setDisabled(disable);
  saveAct->setDisabled(disable);
  saveAsAct->setDisabled(disable);
  quoteAct->setDisabled(disable);
  cutAct->setDisabled(disable);
  copyAct->setDisabled(disable);
  pasteAct->setDisabled(disable);
  closeTabAct->setDisabled(disable);
  selectAllAct->setDisabled(disable);
  findAct->setDisabled(disable);
  verifyAct->setDisabled(disable);
  signAct->setDisabled(disable);
  encryptAct->setDisabled(disable);
  encryptSignAct->setDisabled(disable);
  decryptAct->setDisabled(disable);
  decryptVerifyAct->setDisabled(disable);

  redoAct->setDisabled(disable);
  undoAct->setDisabled(disable);
  zoomOutAct->setDisabled(disable);
  zoomInAct->setDisabled(disable);
  cleanDoubleLinebreaksAct->setDisabled(disable);
  quoteAct->setDisabled(disable);
  appendSelectedKeysAct->setDisabled(disable);
  importKeyFromEditAct->setDisabled(disable);

  cutPgpHeaderAct->setDisabled(disable);
  addPgpHeaderAct->setDisabled(disable);
}

void MainWindow::slotOpenSettingsDialog() {
  auto dialog = new SettingsDialog(this);

  connect(dialog, &SettingsDialog::finished, this, [&]() -> void {
    LOG(INFO) << "Setting Dialog Finished";

    auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

    int icon_width = settings["window"]["icon_size"]["width"];
    int icon_height = settings["window"]["icon_size"]["height"];

    this->setIconSize(QSize(icon_width, icon_height));
    importButton->setIconSize(QSize(icon_width, icon_height));

    // Iconstyle

    int icon_style = settings["window"]["icon_style"];
    auto button_style = static_cast<Qt::ToolButtonStyle>(icon_style);
    this->setToolButtonStyle(button_style);
    importButton->setToolButtonStyle(button_style);

    // restart mainwindow if necessary
    if (getRestartNeeded()) {
      if (edit->maybeSaveAnyTab()) {
        saveSettings();
        qApp->exit(RESTART_CODE);
      }
    }
#ifdef ADVANCED_SUPPORT
    // steganography hide/show
    if (!settings.value("advanced/steganography").toBool()) {
      this->menuBar()->removeAction(steganoMenu->menuAction());
    } else {
      this->menuBar()->insertAction(viewMenu->menuAction(),
                                    steganoMenu->menuAction());
    }
#endif
  });
}

void MainWindow::slotCleanDoubleLinebreaks() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit->curTextPage()->toPlainText();
  content.replace("\n\n", "\n");
  edit->slotFillTextEditWithText(content);
}

void MainWindow::slotAddPgpHeader() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit->curTextPage()->toPlainText().trimmed();

  content.prepend("\n\n").prepend(GpgConstants::PGP_CRYPT_BEGIN);
  content.append("\n").append(GpgConstants::PGP_CRYPT_END);

  edit->slotFillTextEditWithText(content);
}

void MainWindow::slotCutPgpHeader() {
  if (edit->tabCount() == 0 || edit->slotCurPageTextEdit() == nullptr) {
    return;
  }

  QString content = edit->curTextPage()->toPlainText();
  int start = content.indexOf(GpgConstants::PGP_CRYPT_BEGIN);
  int end = content.indexOf(GpgConstants::PGP_CRYPT_END);

  if (start < 0 || end < 0) {
    return;
  }

  // remove head
  int headEnd = content.indexOf("\n\n", start) + 2;
  content.remove(start, headEnd - start);

  // remove tail
  end = content.indexOf(GpgConstants::PGP_CRYPT_END);
  content.remove(end, QString(GpgConstants::PGP_CRYPT_END).size());

  edit->slotFillTextEditWithText(content.trimmed());
}

void MainWindow::slotSetRestartNeeded(bool needed) {
  this->restartNeeded = needed;
}

bool MainWindow::getRestartNeeded() const { return this->restartNeeded; }

}  // namespace GpgFrontend::UI
