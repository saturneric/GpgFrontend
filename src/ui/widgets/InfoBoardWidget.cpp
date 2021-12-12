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
    : QWidget(parent), Ui_InfoBoard() {
  setupUi(this);

  actionButtonLayout->addStretch();
  actionLabel->setText(_("Actions Menu"));
  copyButton->setText(_("Copy"));
  saveButton->setText(_("Save"));
  clearButton->setText(_("Clear"));

  connect(SignalStation::GetInstance(), &SignalStation::signalRefreshInfoBoard,
          this, &InfoBoardWidget::slotRefresh);
}

void InfoBoardWidget::setInfoBoard(const QString& text,
                                   InfoBoardStatus verifyLabelStatus) {
  QString color;
  infoBoard->clear();
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
  infoBoard->append(text);

  infoBoard->setAutoFillBackground(true);
  QPalette status = infoBoard->palette();
  status.setColor(QPalette::Text, color);
  infoBoard->setPalette(status);

  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  // info board font size
  auto info_font_size = 10;
  try {
    info_font_size = settings.lookup("window.info_font_size");
    if (info_font_size < 9 || info_font_size > 18) info_font_size = 10;
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("info_font_size");
  }
  infoBoard->setFont(QFont("Times", info_font_size));
}

void InfoBoardWidget::slotRefresh(const QString& text, InfoBoardStatus status) {
  infoBoard->clear();
  setInfoBoard(text, status);
  infoBoard->verticalScrollBar()->setValue(0);
}

void InfoBoardWidget::associateTextEdit(QTextEdit* edit) {
  if (mTextPage != nullptr)
    disconnect(mTextPage, SIGNAL(textChanged()), this, SLOT(slotReset()));
  this->mTextPage = edit;
  connect(edit, SIGNAL(textChanged()), this, SLOT(slotReset()));
}

void InfoBoardWidget::associateTabWidget(QTabWidget* tab) {
  if (mTextPage != nullptr)
    disconnect(mTextPage, SIGNAL(textChanged()), this, SLOT(slotReset()));
  if (mTabWidget != nullptr) {
    disconnect(mTabWidget, SIGNAL(tabBarClicked(int)), this, SLOT(slotReset()));
    connect(mTabWidget, SIGNAL(tabCloseRequested(int)), this,
            SLOT(slotReset()));
  }

  mTextPage = nullptr;
  mTabWidget = tab;
  connect(tab, SIGNAL(tabBarClicked(int)), this, SLOT(slotReset()));
  connect(tab, SIGNAL(tabCloseRequested(int)), this, SLOT(slotReset()));
}

void InfoBoardWidget::addOptionalAction(const QString& name,
                                        const std::function<void()>& action) {
  LOG(INFO) << "add option" << name.toStdString();
  auto actionButton = new QPushButton(name);
  auto layout = new QHBoxLayout();
  layout->setContentsMargins(5, 0, 5, 0);
  infoBoard->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  // set margin from surroundings
  layout->addWidget(actionButton);
  actionButtonLayout->addLayout(layout);
  connect(actionButton, &QPushButton::clicked, this, [=]() { action(); });
}

/**
 * Delete All item in actionButtonLayout
 */
void InfoBoardWidget::resetOptionActionsMenu() {
  // skip stretch
  deleteWidgetsInLayout(actionButtonLayout, 1);
}

void InfoBoardWidget::slotReset() {
  this->infoBoard->clear();
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

}  // namespace GpgFrontend::UI
