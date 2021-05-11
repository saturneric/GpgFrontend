/*
 *      textpage.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "editorpage.h"

EditorPage::EditorPage(const QString &filePath, QWidget *parent) : QWidget(parent),
                                                       fullFilePath(filePath)
{
    // Set the Textedit properties
    textPage   = new QTextEdit();
    textPage->setAcceptRichText(false);

    // Set the layout style
    mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->addWidget(textPage);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);

    setAttribute(Qt::WA_DeleteOnClose);
    textPage->setFocus();

    //connect(textPage, SIGNAL(textChanged()), this, SLOT(formatGpgHeader()));
}

const QString& EditorPage::getFilePath() const
{
    return fullFilePath;
}

QTextEdit* EditorPage::getTextPage()
{
    return textPage;
}

void EditorPage::setFilePath(const QString &filePath)
{
    fullFilePath = filePath;
}

void EditorPage::showNotificationWidget(QWidget *widget, const char *className)
{
    widget->setProperty(className,true);
    mainLayout->addWidget(widget);
}

void EditorPage::closeNoteByClass(const char *className)
{
    QList<QWidget *> widgets = findChildren<QWidget *>();
    foreach(QWidget * widget, widgets)
    {
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

    if(start < 0 || startSig < 0 || endSig < 0 || signMarked) {
        return;
    }

    signMarked = true;

    // Set the fontstyle for the header
    QTextCharFormat signFormat;
    signFormat.setForeground(QBrush(QColor::fromRgb(80,80,80)));
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
