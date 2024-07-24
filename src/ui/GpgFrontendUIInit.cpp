/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgFrontendUIInit.h"

#include <QtNetwork>

#include "UIModuleManager.h"
#include "core/GpgConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/module/ModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

QList<QTranslator*> registered_translators;
QList<QByteArray> loaded_qm_datum;

extern void InitUITranslations();

void WaitEnvCheckingProcess() {
  GF_UI_LOG_DEBUG("need to waiting for env checking process");

  // create and show loading window before starting the main window
  auto* waiting_dialog = new QProgressDialog();
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto* waiting_dialog_label = new QLabel(
      QCoreApplication::tr("Loading Gnupg Info...") + "<br /><br />" +
      QCoreApplication::tr(
          "If this process is too slow, please set the key "
          "server address appropriately in the gnupg configuration "
          "file (depending "
          "on the network situation in your country or region)."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalGoodGnupgEnv, waiting_dialog,
                        [=]() {
                          GF_UI_LOG_DEBUG("gpg env loaded successfuly");
                          waiting_dialog->finished(0);
                          waiting_dialog->deleteLater();
                        });

  // new local event looper
  QEventLoop looper;
  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalGoodGnupgEnv, &looper,
                        &QEventLoop::quit);

  QApplication::connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    GF_UI_LOG_DEBUG("cancel clicked on waiting dialog");
    QApplication::quit();
    exit(0);
  });

  auto env_state =
      Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.basic", 0);
  GF_UI_LOG_DEBUG("ui is ready to waiting for env initialized, env_state: {}",
                  env_state);

  // check twice to avoid some unlucky sitations
  if (env_state == 1) {
    GF_UI_LOG_DEBUG("env state turned initialized before the looper start");
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
    return;
  }

  // show the loading window
  waiting_dialog->setModal(true);
  waiting_dialog->setFocus();
  waiting_dialog->show();

  // block the main thread until the gpg context is loaded
  looper.exec();
}

void PreInitGpgFrontendUI() {
  CommonUtils::GetInstance();

  // declare module ui entry mount points
  UIModuleManager::GetInstance().DeclareMountPoint("AboutDialogTabs", "QWidget",
                                                   {});
}

void InitGpgFrontendUI(QApplication* /*app*/) {
  // init locale
  InitUITranslations();

  auto settings = GlobalSettingStation::GetInstance().GetSettings();
  auto theme = settings.value("appearance/theme").toString();

#ifdef WINDOWS
  if (theme.isEmpty()) {
    // support dark mode on windows
    QApplication::setStyle(QStyleFactory::create("Fusion"));
  }
#endif

  auto available_styles = QStyleFactory::keys();
  for (QString& s : available_styles) s = s.toLower();
  if (!theme.isEmpty() && available_styles.contains(theme)) {
    QApplication::setStyle(QStyleFactory::create(theme));
  }

  // register meta types
  qRegisterMetaType<QSharedPointer<GpgPassphraseContext> >(
      "QSharedPointer<GpgPassphraseContext>");

  // init signal station
  UISignalStation::GetInstance();

  // init common utils
  CommonUtils::GetInstance();

  // application proxy configure
  auto proxy_enable = settings.value("proxy/enable", false).toBool();

  // if enable proxy for application
  if (proxy_enable) {
    try {
      QString proxy_type =
          settings.value("proxy/proxy_type", QString{}).toString();
      QString proxy_host =
          settings.value("proxy/proxy_host", QString{}).toString();
      int proxy_port = settings.value("prox/port", 0).toInt();
      QString const proxy_username =
          settings.value("proxy/username", QString{}).toString();
      QString const proxy_password =
          settings.value("proxy/password", QString{}).toString();
      GF_UI_LOG_DEBUG("proxy settings: type {}, host {}, port: {}", proxy_type,
                      proxy_host, proxy_port);

      QNetworkProxy::ProxyType proxy_type_qt = QNetworkProxy::NoProxy;
      if (proxy_type == "HTTP") {
        proxy_type_qt = QNetworkProxy::HttpProxy;
      } else if (proxy_type == "Socks5") {
        proxy_type_qt = QNetworkProxy::Socks5Proxy;
      } else {
        proxy_type_qt = QNetworkProxy::DefaultProxy;
      }

      // create proxy object and apply settings
      QNetworkProxy proxy;
      if (proxy_type_qt != QNetworkProxy::DefaultProxy) {
        proxy.setType(proxy_type_qt);
        proxy.setHostName(proxy_host);
        proxy.setPort(proxy_port);
        if (!proxy_username.isEmpty()) {
          proxy.setUser(proxy_username);
        }
        if (!proxy_password.isEmpty()) {
          proxy.setPassword(proxy_password);
        }
      } else {
        proxy.setType(proxy_type_qt);
      }
      QNetworkProxy::setApplicationProxy(proxy);

    } catch (...) {
      GF_UI_LOG_ERROR("setting operation error: proxy setings");
      // no proxy by default
      QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
  } else {
    // no proxy by default
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
  }

  if (Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.basic", 0) ==
      0) {
    WaitEnvCheckingProcess();
  }
}

auto RunGpgFrontendUI(QApplication* app) -> int {
  // create main window and show it
  auto main_window = std::make_unique<GpgFrontend::UI::MainWindow>();

  // pre-check, if application need to restart
  if (CommonUtils::GetInstance()->isApplicationNeedRestart()) {
    GF_UI_LOG_DEBUG("application need to restart, before main window init");
    return kDeepRestartCode;
  }

  // init main window
  main_window->Init();

  // show main windows
  GF_UI_LOG_DEBUG("main window is ready to show");
  main_window->show();

  // start the main event loop
  return QApplication::exec();
}

void GPGFRONTEND_UI_EXPORT DestroyGpgFrontendUI() {}

/**
 * @brief setup the locale and load the translations
 *
 */
void InitUITranslations() {
  for (const auto& translator : registered_translators) {
    QCoreApplication::removeTranslator(translator);
  }
  registered_translators.clear();
  loaded_qm_datum.clear();

  auto* translator = new QTranslator(QCoreApplication::instance());
  if (translator->load(QLocale(), QLatin1String("qt"), QLatin1String("_"),
                       QLatin1String(":/i18n_qt"), QLatin1String(".qm"))) {
    GF_UI_LOG_DEBUG("load qt translation file done, locale: {}",
                    QLocale().name());

    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
  }

  translator = new QTranslator(QCoreApplication::instance());
  if (translator->load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"),
                       QLatin1String(":/i18n_qt"), QLatin1String(".qm"))) {
    GF_UI_LOG_DEBUG("load qtbase translation file done, locale: {}",
                    QLocale().name());
    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
  }

  translator = new QTranslator(QCoreApplication::instance());
  if (translator->load(QLocale(), QLatin1String(PROJECT_NAME),
                       QLatin1String("."), QLatin1String(":/i18n"),
                       QLatin1String(".qm"))) {
    GF_UI_LOG_DEBUG("load target translation file done, locale: {}",
                    QLocale().name());
    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
  }
}

auto InstallTranslatorFromQMData(const QByteArray& data) -> bool {
  auto* translator = new QTranslator(QCoreApplication::instance());

  if (translator->load(reinterpret_cast<uchar*>(const_cast<char*>(data.data())),
                       data.size())) {
    GF_UI_LOG_DEBUG("load target translation file done, locale: {}",
                    QLocale().name());
    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
    loaded_qm_datum.append(data);

    return true;
  }

  return false;
}

}  // namespace GpgFrontend::UI
