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

#ifndef __VERIFYNOTIFICATION_H__
#define __VERIFYNOTIFICATION_H__

#include "EditorPage.h"
#include "gpg/result_analyse/VerifyResultAnalyse.h"
#include "ui/details/VerifyDetailsDialog.h"

class Ui_InfoBoard;

namespace GpgFrontend::UI {

/**
 * @details Enumeration for the status of Verifylabel
 */
typedef enum {
  INFO_ERROR_OK = 0,
  INFO_ERROR_WARN = 1,
  INFO_ERROR_CRITICAL = 2,
  INFO_ERROR_NEUTRAL = 3,
} InfoBoardStatus;

/**
 * @brief Class for handling the verifylabel shown at buttom of a textedit-page
 */
class InfoBoardWidget : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief
   *
   * @param ctx The GPGme-Context
   * @param parent The parent widget
   */
  explicit InfoBoardWidget(QWidget* parent);

  void associateTextEdit(QTextEdit* edit);

  void associateTabWidget(QTabWidget* tab);

  void addOptionalAction(const QString& name,
                         const std::function<void()>& action);

  void resetOptionActionsMenu();

  /**
   * @details Set the text and background-color of verify notification.
   *
   * @param text The text to be set.
   * @param verifyLabelStatus The status of label to set the specified color.
   */
  void setInfoBoard(const QString& text, InfoBoardStatus verifyLabelStatus);

 public slots:

  void slotReset();

  /**
   * @details Refresh the contents of dialog.
   */
  void slotRefresh(const QString& text, InfoBoardStatus status);

 private slots:

  void slotCopy();

  void slotSave();

 private:
  std::shared_ptr<Ui_InfoBoard> ui;

  QTextEdit* mTextPage{nullptr}; /** TextEdit associated to the notification */
  QTabWidget* mTabWidget{nullptr};

  void deleteWidgetsInLayout(QLayout* layout, int start_index = 0);
};

}  // namespace GpgFrontend::UI

#endif  // __VERIFYNOTIFICATION_H__