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

#include "MainWindow.h"

int main(int argc, char *argv[]) {

    Q_INIT_RESOURCE(gpg4usb);

    QApplication app(argc, argv);

    // get application path
    QString appPath = qApp->applicationDirPath();

    QApplication::setApplicationVersion("1.0.0");
    QApplication::setApplicationName("GPGFrontend");

    // dont show icons in menus
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

    // unicode in source
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

    // set environment variables
    // TODO:
    //   - unsetenv on windows?
    //   - wputenv or wputenv_s on windows? http://msdn.microsoft.com/en-us/library/d6dtz42k(v=vs.80).aspx
#ifndef _WIN32
    // do not use GPG_AGENTS like seahorse, because they may save
    // a password an pc's not owned by user
    unsetenv("GPG_AGENT_INFO");
#endif

//        qDebug() << getenv("GNUPGHOME");

#ifndef GPG4USB_NON_PORTABLE
    // take care of gpg not creating directorys on harddisk
    putenv(QString("GNUPGHOME=" + appPath + "/keydb").toUtf8().data());

    // this may help with newer gpgme versions on windows
    //putenv(QString("GPGME_GPGPATH=" + appPath.replace("/", "\\") + "\\bin\\gpg.exe").toUtf8().data());

    // QSettings uses org-name for automatically setting path...
    QApplication::setOrganizationName("conf");

    // specify default path & format for QSettings
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, appPath);
#else
    // in non portable conf should go to ~/.conf/gpg4usb
    app.setOrganizationName("gpg4usb");
    qDebug() << "gpg4usb non portable build";
#endif

    /*QLocale ql(lang);
    foreach(QLocale l , QLocale::matchingLocales(ql.language(), ql.script(), ql.country())) {
        qDebug() << "l: " <<  l.bcp47Name();
    }*/

    // css
    QFile file(qApp->applicationDirPath() + "/css/default.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
    file.close();

    /**
     * internationalisation. loop to restart mainwindow
     * with changed translation when settings change.
     */
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    QTranslator translator, translator2;
    int return_from_event_loop_code;

    do {
        QApplication::removeTranslator(&translator);
        QApplication::removeTranslator(&translator2);

        QString lang = settings.value("int/lang", QLocale::system().name()).toString();
        if (lang.isEmpty()) {
            lang = QLocale::system().name();
        }

        translator.load("ts/gpg4usb_" + lang, appPath);
        QApplication::installTranslator(&translator);

        // set qt translations
        translator2.load("ts/qt_" + lang, appPath);
        QApplication::installTranslator(&translator2);

        MainWindow window;
        return_from_event_loop_code = QApplication::exec();

    } while (return_from_event_loop_code == RESTART_CODE);

    return return_from_event_loop_code;
}



