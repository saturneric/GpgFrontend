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

#ifndef FINDWIDGET_H
#define FINDWIDGET_H

#include "EditorPage.h"

/**
 * @brief Class for handling the find widget shown at buttom of a textedit-page
 */
class FindWidget : public QWidget {
Q_OBJECT

public:
    /**
     * @brief
     *
     * @param parent The parent widget
     */
    explicit FindWidget(QWidget *parent, QTextEdit *edit);

private:
    void keyPressEvent(QKeyEvent *e) override;

    /**
     * @details Set background of findEdit to red, if no match is found (Documents textcursor position equals -1),
     *          otherwise set it to white.
     */
    void setBackground();

    QTextEdit *mTextpage; /** Textedit associated to the notification */
    QLineEdit *findEdit; /** Label holding the text shown in verifyNotification */
    [[maybe_unused]] QTextCharFormat cursorFormat;

private slots:

    void slotFindNext();

    void slotFindPrevious();

    void slotFind();

    void slotClose();
};

#endif // FINDWIDGET_H
