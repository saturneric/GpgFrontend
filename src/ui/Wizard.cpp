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

#include "ui/Wizard.h"

Wizard::Wizard(GpgME::GpgContext *ctx, KeyMgmt *keyMgmt, QWidget *parent)
        : QWizard(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini", QSettings::IniFormat) {
    mCtx = ctx;
    mKeyMgmt = keyMgmt;

    setPage(Page_Intro, new IntroPage(this));
    setPage(Page_Choose, new ChoosePage(this));
    setPage(Page_GenKey, new KeyGenPage(mCtx, this));
    setPage(Page_Conclusion, new ConclusionPage(this));
#ifndef Q_WS_MAC
    setWizardStyle(ModernStyle);
#endif
    setWindowTitle(tr("First Start Wizard"));

    // http://www.flickr.com/photos/laureenp/6141822934/
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/keys2.jpg"));
    setPixmap(QWizard::LogoPixmap, QPixmap(":/logo_small.png"));
    setPixmap(QWizard::BannerPixmap, QPixmap(":/banner.png"));

    setStartId(settings.value("wizard/nextPage", -1).toInt());
    settings.remove("wizard/nextPage");

    connect(this, SIGNAL(accepted()), this, SLOT(slotWizardAccepted()));

}

void Wizard::slotWizardAccepted() {
    // Don't show is mapped to show -> negation
    settings.setValue("wizard/showWizard", !field("showWizard").toBool());

    if (field("openHelp").toBool()) {
        emit signalOpenHelp("docu.html#content");
    }
}

IntroPage::IntroPage(QWidget *parent)
        : QWizardPage(parent), appPath(qApp->applicationDirPath()),
          settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini", QSettings::IniFormat) {
    setTitle(tr("Getting Started..."));
    setSubTitle(tr("... with GPGFrontend"));

    auto *topLabel = new QLabel(tr("Welcome to use GPGFrontend for decrypting and signing text or file!") +
                                " <br><br><a href='https://gpgfrontend.pub'>GpgFrontend</a> " +
                                tr("is a Powerful, Easy-to-Use, Compact, Cross-Platform, and Installation-Free OpenPGP Crypto Tool.") +
                                tr("For brief information have a look at the") +
                                " <a href='https://gpgfrontend.pub/index.html#/overview'>" +
                                tr("Overview") + "</a> (" +
                                tr("by clicking the link, the page will open in the web browser") +
                                "). <br>");
    topLabel->setTextFormat(Qt::RichText);
    topLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    topLabel->setOpenExternalLinks(true);
    topLabel->setWordWrap(true);

    // QComboBox for language selection
    auto *langLabel = new QLabel(tr("Choose a Language"));
    langLabel->setWordWrap(true);

    languages = SettingsDialog::listLanguages();
    auto *langSelectBox = new QComboBox();

    for (const auto &l : languages) {
        langSelectBox->addItem(l);
    }
    // selected entry from config
    QString langKey = settings.value("int/lang").toString();
    QString langValue = languages.value(langKey);
    if (langKey != "") {
        langSelectBox->setCurrentIndex(langSelectBox->findText(langValue));
    }

    connect(langSelectBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotLangChange(QString)));

    // set layout and add widgets
    auto *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
    layout->addWidget(langLabel);
    layout->addWidget(langSelectBox);
    setLayout(layout);
}

void IntroPage::slotLangChange(const QString &lang) {
    settings.setValue("int/lang", languages.key(lang));
    settings.setValue("wizard/nextPage", this->wizard()->currentId());
    qApp->exit(RESTART_CODE);
}

int IntroPage::nextId() const {
    return Wizard::Page_Choose;
}

ChoosePage::ChoosePage(QWidget *parent)
        : QWizardPage(parent) {
    setTitle(tr("Choose your action..."));
    setSubTitle(tr("...by clicking on the appropriate link."));

    auto *keygenLabel = new QLabel(tr("If you have never used GPGFrontend before and also don't own a gpg key yet you "
                                      "may possibly want to read how to") +
                                   " <a href=\"https://gpgfrontend.pub/index.html#/manual/generate-key\">"
                                   + tr("Generate Key") + "</a><hr>");
    keygenLabel->setTextFormat(Qt::RichText);
    keygenLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    keygenLabel->setOpenExternalLinks(true);
    keygenLabel->setWordWrap(true);

    auto *encrDecyTextLabel = new QLabel(
            tr("If you want to learn how to encrypt, decrypt, sign and verify text, you can read ")
            + "<a href=\"https://gpgfrontend.pub/index.html#/manual/encrypt-decrypt-text\">"
            + tr("Encrypt & Decrypt Text") + "</a> " + tr("or")
            + " <a href=\"https://gpgfrontend.pub/index.html#/manual/sign-verify-text\">"
            + tr("Sign & Verify Text")
            + "</a><hr>");

    encrDecyTextLabel->setTextFormat(Qt::RichText);
    encrDecyTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    encrDecyTextLabel->setOpenExternalLinks(true);
    encrDecyTextLabel->setWordWrap(true);

    auto *signVerifyTextLabel = new QLabel(tr("If you want to operate file, you can read ")
                                           +
                                           "<a href=\"https://gpgfrontend.pub/index.html#/manual/encrypt-decrypt-file\">"
                                           + tr("Encrypt & Sign File") + "</a> " + tr("or")
                                           +
                                           " <a href=\"https://gpgfrontend.pub/index.html#/manual/sign-verify-file\">"
                                           + tr("Sign & Verify File")
                                           + "</a><hr>");
    signVerifyTextLabel->setTextFormat(Qt::RichText);
    signVerifyTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    signVerifyTextLabel->setOpenExternalLinks(true);
    signVerifyTextLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout();
    layout->addWidget(keygenLabel);
    layout->addWidget(encrDecyTextLabel);
    layout->addWidget(signVerifyTextLabel);
    setLayout(layout);
    nextPage = Wizard::Page_Conclusion;
}

int ChoosePage::nextId() const {
    return nextPage;
}

void ChoosePage::slotJumpPage(const QString &page) {
    QMetaObject qmo = Wizard::staticMetaObject;
    int index = qmo.indexOfEnumerator("WizardPages");
    QMetaEnum m = qmo.enumerator(index);

    nextPage = m.keyToValue(page.toUtf8().data());
    wizard()->next();
}

KeyGenPage::KeyGenPage(GpgME::GpgContext *ctx, QWidget *parent)
        : QWizardPage(parent) {
    mCtx = ctx;
    setTitle(tr("Create a keypair..."));
    setSubTitle(tr("...for decrypting and signing messages"));
    auto *topLabel = new QLabel(tr("You should create a new keypair."
                                   "The pair consists of a public and a private key.<br>"
                                   "Other users can use the public key to encrypt messages for you "
                                   "and verify messages signed by you."
                                   "You can use the private key to decrypt and sign messages.<br>"
                                   "For more information have a look at the offline tutorial (which then is shown in the main window):"));
    topLabel->setWordWrap(true);
    auto *linkLabel = new QLabel("<a href=""docu_keygen.html#content"">" + tr("Offline tutorial") + "</a>");
    //linkLabel->setOpenExternalLinks(true);

    // connect(linkLabel, SIGNAL(linkActivated(QString)), parentWidget()->parentWidget(), SLOT(openHelp(QString)));

    auto *createKeyButtonBox = new QWidget(this);
    auto *createKeyButtonBoxLayout = new QHBoxLayout(createKeyButtonBox);
    auto *createKeyButton = new QPushButton(tr("Create New Key"));
    createKeyButtonBoxLayout->addWidget(createKeyButton);
    createKeyButtonBoxLayout->addStretch(1);
    auto *layout = new QVBoxLayout();
    layout->addWidget(topLabel);
    layout->addWidget(linkLabel);
    layout->addWidget(createKeyButtonBox);
    connect(createKeyButton, SIGNAL(clicked(bool)), this, SLOT(slotGenerateKeyDialog()));

    setLayout(layout);
}

int KeyGenPage::nextId() const {
    return Wizard::Page_Conclusion;
}

void KeyGenPage::slotGenerateKeyDialog() {
    qDebug() << "Try Opening KeyGenDialog";
    auto *keyGenDialog = new KeyGenDialog(mCtx, this);
    keyGenDialog->show();
    wizard()->next();
}

ConclusionPage::ConclusionPage(QWidget *parent)
        : QWizardPage(parent) {
    setTitle(tr("Ready."));
    setSubTitle(tr("Have fun with GPGFrontend!"));

    auto *bottomLabel = new QLabel(tr("You are ready to use GPGFrontend now.<br><br>") +
                                   "<a href=\"https://saturneric.github.io/GpgFrontend/index.html#/overview\">"
                                   + tr("The Online Document") + "</a>"
                                   +
                                   tr(" will get you started with GPGFrontend. It will open in the main window.<br>"));

    bottomLabel->setTextFormat(Qt::RichText);
    bottomLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    bottomLabel->setOpenExternalLinks(true);
    bottomLabel->setWordWrap(true);

    openHelpCheckBox = new QCheckBox(tr("Open offline help."));
    openHelpCheckBox->setChecked(Qt::Checked);

    dontShowWizardCheckBox = new QCheckBox(tr("Dont show the wizard again."));
    dontShowWizardCheckBox->setChecked(Qt::Checked);

    registerField("showWizard", dontShowWizardCheckBox);
    // registerField("openHelp", openHelpCheckBox);

    auto *layout = new QVBoxLayout;
    layout->addWidget(bottomLabel);
    // layout->addWidget(openHelpCheckBox);
    layout->addWidget(dontShowWizardCheckBox);
    setLayout(layout);
    setVisible(true);
}

int ConclusionPage::nextId() const {
    return -1;
}
