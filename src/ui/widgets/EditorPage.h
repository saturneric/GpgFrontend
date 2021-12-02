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

#ifndef __EDITORPAGE_H__
#define __EDITORPAGE_H__

#include "gpg/GpgConstants.h"
#include "ui/GpgFrontendUI.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QString;
class QLabel;
QT_END_NAMESPACE

namespace GpgFrontend::UI {

/**
 * @brief Class for handling a single tab of the tabwidget
 *
 */
class EditorPage : public QWidget {
  Q_OBJECT
 public:
  /**
   * @details Add layout and add plaintextedit
   *
   * @param filePath Path of the file handled in this tab
   * @param parent Pointer to the parent widget
   */
  explicit EditorPage(QString filePath = "", QWidget* parent = nullptr);

  /**
   * @details Get the filepath of the currently activated tab.
   */
  [[nodiscard]] const QString& getFilePath() const;

  /**
   * @details Set filepath of currently activated tab.
   *
   * @param filePath The path to be set
   */
  void setFilePath(const QString& filePath);

  /**
   * @details Return pointer tp the textedit of the currently activated tab.
   */
  QTextEdit* getTextPage();

  /**
   * @details Show additional widget at buttom of currently active tab
   *
   * @param widget The widget to be added
   * @param className The name to handle the added widget
   */
  void showNotificationWidget(QWidget* widget, const char* className);

  /**
   * @details Hide all widgets with the given className
   *
   * @param className The classname of the widgets to hide
   */
  void closeNoteByClass(const char* className);

  void ReadFile();

  [[nodiscard]] bool ReadDone() const { return this->readDone; }

  void PrepareToDestroy();

 private:
  QTextEdit* textPage;     /** The textedit of the tab */
  QVBoxLayout* mainLayout; /** The layout for the tab */
  QString fullFilePath;    /** The path to the file handled in the tab */
  bool signMarked{}; /** true, if the signed header is marked, false if not */
  bool readDone = false;
  QThread* readThread = nullptr;

 private slots:

  /**
   * @details Format the gpg header in another font-style
   */
  void slotFormatGpgHeader();

  void slotInsertText(const QString& text);
};

}  // namespace GpgFrontend::UI

#endif  // __EDITORPAGE_H__
