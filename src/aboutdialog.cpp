/*
 *      aboutdialog.cpp
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

#include "aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
        : QDialog(parent) {
    this->setWindowTitle(tr("About ") + qApp->applicationName());

    auto *tabWidget = new QTabWidget;
    auto *infoTab = new InfoTab;
    auto *translatorsTab = new TranslatorsTab;

    tabWidget->addTab(infoTab, tr("General"));
    tabWidget->addTab(translatorsTab, tr("Translators"));

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    this->exec();
}

InfoTab::InfoTab(QWidget *parent)
        : QWidget(parent) {
    auto *pixmap = new QPixmap(":gpg4usb-logo.png");
    auto *text = new QString("<center><h2>" + qApp->applicationName() + " "
                             + qApp->applicationVersion() + "</h2></center>"
                             + tr("<center>This application allows simple encryption <br>"
                                  "and decryption of text messages or files.<br>"
                                  "It's licensed under the GPL v3<br><br>"
                                  "<b>Developer:</b><br>"
                                  "Bene, Heimer, Juergen, Nils, Ubbo<br><br>"
                                  "If you have any questions or suggestions have a look<br/>"
                                  "at our <a href=\"http://gpg4usb.cpunk.de/contact.php\">"
                                  "contact page</a> or send a mail to our<br/> mailing list at"
                                  " <a href=\"mailto:gpg4usb@gzehn.de\">gpg4usb@gzehn.de</a>.") +
                             tr("<br><br> Built with Qt ") + qVersion()
                             + tr(" and GPGME ") + GpgME::GpgContext::getGpgmeVersion() + "</center>");

    auto *layout = new QGridLayout();
    auto *pixmapLabel = new QLabel();
    pixmapLabel->setPixmap(*pixmap);
    layout->addWidget(pixmapLabel, 0, 0, 1, -1, Qt::AlignCenter);
    auto *aboutLabel = new QLabel();
    aboutLabel->setText(*text);
    aboutLabel->setOpenExternalLinks(true);
    layout->addWidget(aboutLabel, 1, 0, 1, -1);
    layout->addItem(new QSpacerItem(20, 10, QSizePolicy::Minimum,
                                    QSizePolicy::Fixed), 2, 1, 1, 1);

    setLayout(layout);
}

TranslatorsTab::TranslatorsTab(QWidget *parent)
        : QWidget(parent) {
    QFile translatorsFile;
    translatorsFile.setFileName(qApp->applicationDirPath() + "/TRANSLATORS");
    translatorsFile.open(QIODevice::ReadOnly);
    QByteArray inBuffer = translatorsFile.readAll();

    auto *label = new QLabel(inBuffer);
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(label);

    setLayout(mainLayout);
}

