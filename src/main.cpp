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
#include "GpgFrontendBuildInfo.h"

int main(int argc, char *argv[]) {

    Q_INIT_RESOURCE(gpgfrontend);

    QApplication app(argc, argv);

    // get application path
    QString appPath = qApp->applicationDirPath();

    QApplication::setApplicationVersion(BUILD_VERSION);
    QApplication::setApplicationName(PROJECT_NAME);

    // dont show icons in menus
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

    // unicode in source
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

#ifdef WINDOWS
    // css
    QFile file(RESOURCE_DIR(qApp->applicationDirPath()) + "/css/default.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
    file.close();
#endif

    /**
     * internationalisation. loop to restart mainwindow
     * with changed translation when settings change.
     */
    if(!QDir(RESOURCE_DIR(appPath) + "/conf").exists()) {
        QDir().mkdir(RESOURCE_DIR(appPath) + "/conf");
    }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini", QSettings::IniFormat);
    QTranslator translator, translator2;
    int return_from_event_loop_code;

    qDebug() << "Resource Directory" << RESOURCE_DIR(appPath);

    do {
        QApplication::removeTranslator(&translator);
        QApplication::removeTranslator(&translator2);

        QString lang = settings.value("int/lang", QLocale::system().name()).toString();
        if (lang.isEmpty()) {
            lang = QLocale::system().name();
        }
        qDebug() << "Language set" << lang;
        translator.load(RESOURCE_DIR(appPath) + "/ts/" + "gpgfrontend_" + lang);
        qDebug() << "Translator" << translator.filePath();
        QApplication::installTranslator(&translator);

        // set qt translations
        translator2.load(RESOURCE_DIR(appPath) + "/ts/qt_" + lang);
        qDebug() << "Translator2" << translator2.filePath();
        QApplication::installTranslator(&translator2);

        QApplication::setQuitOnLastWindowClosed(true);


        /**
         * The function `gpgme_check_version' must be called before any other
         *  function in the library, because it initializes the thread support
         *  subsystem in GPGME. (from the info page) */
        gpgme_check_version(nullptr);

        // the locale set here is used for the other setlocale calls which have nullptr
        // -> nullptr means use default, which is configured here
        setlocale(LC_ALL, settings.value("int/lang").toLocale().name().toUtf8().constData());

        /** set locale, because tests do also */
        gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
        //qDebug() << "Locale set to" << LC_CTYPE << " - " << setlocale(LC_CTYPE, nullptr);
        #ifndef _WIN32
        gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
        #endif

        MainWindow window;
        return_from_event_loop_code = QApplication::exec();

    } while (return_from_event_loop_code == RESTART_CODE);

    return return_from_event_loop_code;
}



