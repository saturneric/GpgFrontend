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

#include "ui/widgets/InfoBoardWidget.h"

#include "ui/SignalStation.h"
#include "ui/settings/GlobalSettingStation.h"
#include "ui_InfoBoard.h"

namespace GpgFrontend::UI {

InfoBoardWidget::InfoBoardWidget(QWidget* parent)
    : QWidget(parent), ui(std::make_shared<Ui_InfoBoard>()) {
  ui->setupUi(this);

  ui->actionButtonLayout->addStretch();
  ui->copyButton->setText(_("Copy"));
  ui->saveButton->setText(_("Save File"));
  ui->clearButton->setText(_("Clear"));

  connect(ui->copyButton, &QPushButton::clicked, this,
          &InfoBoardWidget::slotCopy);
  connect(ui->saveButton, &QPushButton::clicked, this,
          &InfoBoardWidget::slotSave);
  connect(ui->clearButton, &QPushButton::clicked, this,
          &InfoBoardWidget::slotReset);

  connect(SignalStation::GetInstance(), &SignalStation::signalRefreshInfoBoard,
          this, &InfoBoardWidget::slotRefresh);
}

void InfoBoardWidget::setInfoBoard(const QString& text,
                                   InfoBoardStatus verifyLabelStatus) {
  QString color;
  ui->infoBoard->clear();
  switch (verifyLabelStatus) {
    case INFO_ERROR_OK:
      color = "#008000";
      break;
    case INFO_ERROR_WARN:
      color = "#FF8C00";
      break;
    case INFO_ERROR_CRITICAL:
      color = "#DC143C";
      break;
    default:
      break;
  }
  ui->infoBoard->append(text);

  ui->infoBoard->setAutoFillBackground(true);
  QPalette status = ui->infoBoard->palette();
  status.setColor(QPalette::Text, color);
  ui->infoBoard->setPalette(status);

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  // info board font size
  auto info_font_size = 10;
  try {
    info_font_size = settings.lookup("window.info_font_size");
    if (info_font_size < 9 || info_font_size > 18) info_font_size = 10;
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("info_font_size");
  }
  ui->infoBoard->setFont(QFont("Times", info_font_size));
}

void InfoBoardWidget::slotRefresh(const QString& text, InfoBoardStatus status) {
  ui->infoBoard->clear();
  setInfoBoard(text, status);
  ui->infoBoard->verticalScrollBar()->setValue(0);
}

void InfoBoardWidget::associateTextEdit(QTextEdit* edit) {
  if (mTextPage != nullptr)
    disconnect(mTextPage, SIGNAL(textChanged()), this, SLOT(slotReset()));
  this->mTextPage = edit;
  connect(edit, SIGNAL(textChanged()), this, SLOT(slotReset()));
}

void InfoBoardWidget::associateTabWidget(QTabWidget* tab) {
  mTextPage = nullptr;
  mTabWidget = tab;
  connect(tab, SIGNAL(tabBarClicked(int)), this, SLOT(slotReset()));
  connect(tab, SIGNAL(tabCloseRequested(int)), this, SLOT(slotReset()));
  // reset
  this->slotReset();
}

void InfoBoardWidget::addOptionalAction(const QString& name,
                                        const std::function<void()>& action) {
  LOG(INFO) << "add option" << name.toStdString();
  auto actionButton = new QPushButton(name);
  auto layout = new QHBoxLayout();
  layout->setContentsMargins(5, 0, 5, 0);
  ui->infoBoard->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  // set margin from surroundings
  layout->addWidget(actionButton);
  ui->actionButtonLayout->addLayout(layout);
  connect(actionButton, &QPushButton::clicked, this, [=]() { action(); });
}

/**
 * Delete All item in actionButtonLayout
 */
void InfoBoardWidget::resetOptionActionsMenu() {
  // skip stretch
  deleteWidgetsInLayout(ui->actionButtonLayout, 1);
}

void InfoBoardWidget::slotReset() {
  ui->infoBoard->clear();
  resetOptionActionsMenu();
}

/**
 * Try Delete all widget from target layout
 * @param layout target layout
 */
void InfoBoardWidget::deleteWidgetsInLayout(QLayout* layout, int start_index) {
  LOG(INFO) << "Called";

  QLayoutItem* item;
  while ((item = layout->layout()->takeAt(start_index)) != nullptr) {
    layout->removeItem(item);
    if (item->layout() != nullptr)
      deleteWidgetsInLayout(item->layout());
    else if (item->widget() != nullptr)
      delete item->widget();
    delete item;
  }
}

void InfoBoardWidget::slotCopy() {
  auto* clipboard = QGuiApplication::clipboard();
  clipboard->setText(ui->infoBoard->toPlainText());
}

void InfoBoardWidget::slotSave() {
  auto file_path = QFileDialog::getSaveFileName(
      this, _("Save Information Board's Content"), {}, tr("Text (*.txt)"));
  LOG(INFO) << "file path" << file_path.toStdString();
  if (file_path.isEmpty()) return;

  QFile file(file_path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    file.write(ui->infoBoard->toPlainText().toUtf8());
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The file path is not exists, unprivileged or unreachable."));
  }
  file.close();
}

}  // namespace GpgFrontend::UI
