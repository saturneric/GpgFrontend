/*
 *      helppage.cpp
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

#include "helppage.h"

HelpPage::HelpPage(const QString path, QWidget *parent) :
    QWidget(parent)
{

    browser = new QTextBrowser();
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->addWidget(browser);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);
    //setAttribute(Qt::WA_DeleteOnClose);
    //browser->setSource(QUrl::fromLocalFile(path));

    connect(browser, SIGNAL(anchorClicked(QUrl)), this, SLOT(slotOpenUrl(QUrl)));
    browser->setOpenLinks(false);
    browser->setSource(localizedHelp(QUrl(path)));
    browser->setFocus();

}

void HelpPage::slotOpenUrl(QUrl url) {
     browser->setSource(localizedHelp(url));
};

/**
 * @brief HelpPage::localizedHelp
 * check if the requested file is also available with the locale,
 * e.g. return index.de.html if index.html was requested but the
 * locale is de and index.de.html is available
 * @param url
 * @return
 */
QUrl HelpPage::localizedHelp(QUrl url) {
    QString path = url.toLocalFile();
    QString filename = path.mid(path.lastIndexOf("/") + 1 );
    QString filepath = path.left(path.lastIndexOf("/") + 1 );
    QStringList fileparts = filename.split(".");

    //QSettings settings;
    QString lang = QSettings().value("int/lang", QLocale::system().name()).toString();
    if (lang.isEmpty()) {
        lang = QLocale::system().name();
    }

    fileparts.insert(1,lang);
    QString langfile = filepath + fileparts.join(".");

    if(QFile(QUrl(langfile).toLocalFile()).exists()) {
        return langfile;
    } else {
        return path;
    }

}

QTextBrowser* HelpPage::getBrowser() {
    return browser;
}
