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
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/widgets/EditorPage.h"

#include <utility>

EditorPage::EditorPage(QString filePath, QWidget *parent) : QWidget(parent),
                                                            fullFilePath(std::move(filePath)) {
    // Set the Textedit properties
    textPage = new QTextEdit();
    textPage->setAcceptRichText(false);

    // Set the layout style
    mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->addWidget(textPage);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);

    setAttribute(Qt::WA_DeleteOnClose);
    textPage->setFocus();

    // Front in same width
    this->setFont({"Courier"});
}

const QString &EditorPage::getFilePath() const {
    return fullFilePath;
}

QTextEdit *EditorPage::getTextPage() {
    return textPage;
}

void EditorPage::setFilePath(const QString &filePath) {
    fullFilePath = filePath;
}

void EditorPage::showNotificationWidget(QWidget *widget, const char *className) {
    widget->setProperty(className, true);
    mainLayout->addWidget(widget);
}

void EditorPage::closeNoteByClass(const char *className) {
    QList<QWidget *> widgets = findChildren<QWidget *>();
    for (QWidget *widget:  widgets) {
        if (widget->property(className) == true) {
            widget->close();
        }
    }
}

void EditorPage::slotFormatGpgHeader() {

    QString content = textPage->toPlainText();

    // Get positions of the gpg-headers, if they exist
    int start = content.indexOf(GpgConstants::PGP_SIGNED_BEGIN);
    int startSig = content.indexOf(GpgConstants::PGP_SIGNATURE_BEGIN);
    int endSig = content.indexOf(GpgConstants::PGP_SIGNATURE_END);

    if (start < 0 || startSig < 0 || endSig < 0 || signMarked) {
        return;
    }

    signMarked = true;

    // Set the fontstyle for the header
    QTextCharFormat signFormat;
    signFormat.setForeground(QBrush(QColor::fromRgb(80, 80, 80)));
    signFormat.setFontPointSize(9);

    // set font style for the signature
    QTextCursor cursor(textPage->document());
    cursor.setPosition(startSig, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, endSig);
    cursor.setCharFormat(signFormat);

    // set the font style for the header
    int headEnd = content.indexOf("\n\n", start);
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, headEnd);
    cursor.setCharFormat(signFormat);

}
