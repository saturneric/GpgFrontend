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

#include "MainWindow.h"

#include "ui/UserInterfaceUtils.h"
#ifdef RELEASE
#include "ui/function/VersionCheckThread.h"
#endif
#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

MainWindow::MainWindow() {
  this->setMinimumSize(1200, 700);
  this->setWindowTitle(qApp->applicationName());
}

void MainWindow::init() noexcept {
  LOG(INFO) << _("Called");
  try {
    // Check Context Status
    if (!GpgContext::GetInstance().good()) {
      QMessageBox::critical(
          nullptr, _("ENV Loading Failed"),
          _("Gnupg is not installed correctly, please follow the ReadME "
            "instructions to install gnupg and then open GpgFrontend."));
      QCoreApplication::quit();
      exit(0);
    }

    networkAccessManager = new QNetworkAccessManager(this);

    /* get path where app was started */
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    edit = new TextEdit(this);
    setCentralWidget(edit);

    /* the list of Keys available*/
    mKeyList = new KeyList(this);

    mKeyList->slotRefresh();

    infoBoard = new InfoBoardWidget(this, mKeyList);

    /* List of binary Attachments */
    attachmentDockCreated = false;

    /* Variable containing if restart is needed */
    this->slotSetRestartNeeded(false);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();

    connect(edit->tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(slotDisableTabActions(int)));

    mKeyList->addMenuAction(appendSelectedKeysAct);
    mKeyList->addMenuAction(copyMailAddressToClipboardAct);
    mKeyList->addMenuAction(showKeyDetailsAct);
    mKeyList->addSeparator();
    mKeyList->addMenuAction(refreshKeysFromKeyserverAct);
    mKeyList->addMenuAction(uploadKeyToServerAct);

    restoreSettings();

    // open filename if provided as first command line parameter
    QStringList args = qApp->arguments();
    if (args.size() > 1) {
      if (!args[1].startsWith("-")) {
        if (QFile::exists(args[1])) edit->loadFile(args[1]);
      }
    }
    edit->curTextPage()->setFocus();

    auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

    if (!settings.exists("wizard") ||
        settings.lookup("wizard").getType() != libconfig::Setting::TypeGroup)
      settings.add("wizard", libconfig::Setting::TypeGroup);

    auto& wizard = settings["wizard"];

    // Show wizard, if the don't show wizard message box wasn't checked
    // and keylist doesn't contain a private key

    if (!wizard.exists("show_wizard"))
      wizard.add("show_wizard", libconfig::Setting::TypeBoolean) = true;

    bool show_wizard = true;
    wizard.lookupValue("show_wizard", show_wizard);

    LOG(INFO) << "wizard show_wizard" << show_wizard;

    if (show_wizard) {
      slotStartWizard();
    }

    emit loaded();

#ifdef RELEASE
    QString baseUrl =
        "https://api.github.com/repos/saturneric/gpgfrontend/releases/latest";
    QNetworkRequest request;
    request.setUrl(QUrl(baseUrl));
    auto* replay = networkAccessManager->get(request);
    auto version_thread = new VersionCheckThread(replay);

    connect(version_thread, SIGNAL(finished()), version_thread,
            SLOT(deleteLater()));
    connect(version_thread, &VersionCheckThread::upgradeVersion, this,
            &MainWindow::slotVersionUpgrade);

    version_thread->start();
#endif

  } catch (...) {
    LOG(FATAL) << _("Critical error occur while loading GpgFrontend.");
    QMessageBox::critical(nullptr, _("Loading Failed"),
                          _("Critical error occur while loading GpgFrontend."));
    QCoreApplication::quit();
    exit(0);
  }
}

void MainWindow::restoreSettings() {
  LOG(INFO) << _("Called");

  try {
    auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

    if (!settings.exists("window") ||
        settings.lookup("window").getType() != libconfig::Setting::TypeGroup)
      settings.add("window", libconfig::Setting::TypeGroup);

    auto& window = settings["window"];

    if (!window.exists("window_state"))
      window.add("window_state", libconfig::Setting::TypeString) =
          saveState().toBase64().toStdString();

    std::string window_state = settings.lookup("window.window_state");
    // state sets pos & size of dock-widgets
    this->restoreState(
        QByteArray::fromBase64(QByteArray::fromStdString(window_state)));

    if (!window.exists("window_save"))
      window.add("window_save", libconfig::Setting::TypeBoolean) = true;

    bool window_save;
    window.lookupValue("window_save", window_save);

    // Restore window size & location
    if (window_save) {
      if (!window.exists("window_pos"))
        window.add("window_pos", libconfig::Setting::TypeGroup);

      auto& window_pos = window["window_pos"];

      if (!window_pos.exists("x"))
        window_pos.add("x", libconfig::Setting::TypeInt) = 100;

      if (!window_pos.exists("y"))
        window_pos.add("y", libconfig::Setting::TypeInt) = 100;

      int x, y;
      window_pos.lookupValue("x", x);
      window_pos.lookupValue("y", y);

      auto pos = QPoint(x, y);

      if (!window.exists("window_size"))
        window.add("window_size", libconfig::Setting::TypeGroup);

      auto& window_size = window["window_size"];

      if (!window_size.exists("width"))
        window_size.add("width", libconfig::Setting::TypeInt) = 800;

      if (!window_size.exists("height"))
        window_size.add("height", libconfig::Setting::TypeInt) = 450;

      int width, height;
      window_size.lookupValue("width", width);
      window_size.lookupValue("height", height);

      auto size = QSize(width, height);
      this->resize(size);
      this->move(pos);
    } else {
      this->resize(QSize(800, 450));
      this->move(QPoint(100, 100));
    }

    if (!window.exists("icon_size"))
      window.add("icon_size", libconfig::Setting::TypeGroup);

    auto& icon_size = window["icon_size"];

    if (!icon_size.exists("width"))
      icon_size.add("width", libconfig::Setting::TypeInt) = 24;

    if (!icon_size.exists("height"))
      icon_size.add("height", libconfig::Setting::TypeInt) = 24;

    int width = icon_size["width"], height = icon_size["height"];
    LOG(INFO) << "icon_size" << width << height;

    // info board font size
    if (!window.exists("info_font_size"))
      window.add("info_font_size", libconfig::Setting::TypeInt) = 10;

    // icons ize
    this->setIconSize(QSize(width, height));
    importButton->setIconSize(QSize(width, height));

    if (!settings.exists("keyserver") ||
        settings.lookup("keyserver").getType() != libconfig::Setting::TypeGroup)
      settings.add("keyserver", libconfig::Setting::TypeGroup);

    auto& keyserver = settings["keyserver"];

    if (!keyserver.exists("server_list")) {
      keyserver.add("server_list", libconfig::Setting::TypeList);

      auto& server_list = keyserver["server_list"];
      server_list.add(libconfig::Setting::TypeString) = "http://keys.gnupg.net";
      server_list.add(libconfig::Setting::TypeString) =
          "https://keyserver.ubuntu.com";
      server_list.add(libconfig::Setting::TypeString) =
          "http://pool.sks-keyservers.net";
    }

    if (!keyserver.exists("default_server")) {
      keyserver.add("default_server", libconfig::Setting::TypeString) =
          "https://keyserver.ubuntu.com";
    }

    if (!window.exists("icon_style")) {
      window.add("icon_style", libconfig::Setting::TypeInt) =
          Qt::ToolButtonTextUnderIcon;
    }

    int s_icon_style = window.lookup("icon_style");

    // icon_style
    auto icon_style = static_cast<Qt::ToolButtonStyle>(s_icon_style);
    this->setToolButtonStyle(icon_style);
    importButton->setToolButtonStyle(icon_style);

    if (!settings.exists("general") ||
        settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
      settings.add("general", libconfig::Setting::TypeGroup);

    auto& general = settings["general"];

    if (!general.exists("save_key_checked")) {
      general.add("save_key_checked", libconfig::Setting::TypeBoolean) = true;
    }

    bool save_key_checked = true;
    general.lookupValue("save_key_checked", save_key_checked);

    // Checked Keys
    if (save_key_checked) {
      if (!general.exists("save_key_checked_key_ids")) {
        general.add("save_key_checked_key_ids", libconfig::Setting::TypeList);
      }
      auto key_ids_ptr = std::make_unique<KeyIdArgsList>();
      auto& save_key_checked_key_ids = general["save_key_checked_key_ids"];
      const auto key_ids_size =
          general.lookup("save_key_checked_key_ids").getLength();
      for (auto i = 0; i < key_ids_size; i++) {
        std::string key_id = save_key_checked_key_ids[i];
        LOG(INFO) << "get checked key id" << key_id;
        key_ids_ptr->push_back(key_id);
      }
      mKeyList->setChecked(key_ids_ptr);
    }
  } catch (...) {
    LOG(ERROR) << "cannot resolve settings";
  }

  GlobalSettingStation::GetInstance().Sync();
}

void MainWindow::saveSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    // window position and size
    settings["window"]["window_state"] = saveState().toBase64().toStdString();
    settings["window"]["window_pos"]["x"] = pos().x();
    settings["window"]["window_pos"]["y"] = pos().y();

    settings["window"]["window_size"]["width"] = size().width();
    settings["window"]["window_size"]["height"] = size().height();

    bool save_key_checked = settings.lookup("general.save_key_checked");

    // keyid-list of private checked keys
    if (save_key_checked) {
      auto& key_ids = settings.lookup("general.save_key_checked_key_ids");
      const int key_ids_size = key_ids.getLength();
      for (auto i = 0; i < key_ids_size; i++) key_ids.remove(i);
      auto key_ids_need_to_store = mKeyList->getChecked();

      for (size_t i = 0; i < key_ids_need_to_store->size(); i++) {
        std::string key_id = (*key_ids_need_to_store)[i];
        key_ids.add(libconfig::Setting::TypeString) = key_id;
      }

    } else {
      settings["general"].remove("save_key_checked");
    }
  } catch (...) {
    LOG(ERROR) << "cannot save settings";
  };

  GlobalSettingStation::GetInstance().Sync();
}

void MainWindow::closeAttachmentDock() {
  if (!attachmentDockCreated) {
    return;
  }
  attachmentDock->close();
  attachmentDock->deleteLater();
  attachmentDockCreated = false;
}

void MainWindow::closeEvent(QCloseEvent* event) {
  /*
   * ask to save changes, if there are
   * modified documents in any tab
   */
  if (edit->maybeSaveAnyTab()) {
    saveSettings();
    event->accept();
  } else {
    event->ignore();
  }

  // clear password from memory
  //  GpgContext::GetInstance().clearPasswordCache();
}

}  // namespace GpgFrontend::UI
