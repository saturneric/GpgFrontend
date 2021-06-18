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

#include "ui/AboutDialog.h"

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
    auto *pixmap = new QPixmap(":gpgfrontend-logo.png");
    auto *text = new QString("<center><h2>" + qApp->applicationName() + "</h2></center>"
            + "<center><b>" + qApp->applicationVersion() + "</b></center>"
            + "<center>" + GIT_VERSION + "</center>"
            + tr("<br><center>GPGFrontend is a modern, easy-to-use, compact, <br>"
            "cross-platform, and installation-free gpg front-end tool.<br>"
            "It visualizes most of the common operations of gpg commands.<br>"
            "It's licensed under the GPL v3<br><br>"
            "<b>Developer:</b><br>"
            "Saturneric<br><br>"
            "If you have any questions or suggestions have a look<br/>"
            "at my <a href=\"https://bktus.com/%e8%81%94%e7%b3%bb%e4%b8%8e%e9%aa%8c%e8%af%81\">"
            "contact page</a> or send a mail to my<br/> mailing list at"
            " <a href=\"mailto:eric@bktus.com\">eric@bktus.com</a>.") +
            tr("<br><br> Built with Qt ") + qVersion()
            + tr(" and GPGME ") + GpgME::GpgContext::getGpgmeVersion() +
            tr("<br>Built at ") + BUILD_TIMESTAMP + "</center>");

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

