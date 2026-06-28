/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "core/GFConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

namespace {

QContainer<QTranslator*> registered_translators;
QContainer<QByteArray> loaded_qm_datum;

[[noreturn]] void TerminateSelfImmediately() {
  qWarning() << "Application startup was canceled. Terminating process.";
  std::_Exit(0);
}

void WaitEnvCheckingProcess() {
  FLOG_D() << "we need to wait for env checking process";

  auto env_state =
      Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.all", 0);
  FLOG_D("ui is ready to wait for env initialized, env_state: %d", env_state);

  if (env_state == 1) {
    FLOG_D("env state turned initialized before the looper start");
    return;
  }

  auto* dialog = new QDialog(nullptr);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(QCoreApplication::tr("Starting GpgFrontend"));
  dialog->setModal(true);
  dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  dialog->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

  auto* title_label =
      new QLabel(QCoreApplication::tr("Loading essential information"));
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSize(title_font.pointSize() + 1);
  title_label->setFont(title_font);

  auto* message_label = new QLabel(QCoreApplication::tr(
      "GpgFrontend is checking your OpenPGP environment and preparing the "
      "default engine. This may take a few seconds."));
  message_label->setWordWrap(true);

  auto* hint_label = new QLabel(QCoreApplication::tr(
      "Please keep this window open while the initialization is running."));
  hint_label->setWordWrap(true);

  auto* progress_bar = new QProgressBar;
  progress_bar->setRange(0, 0);
  progress_bar->setTextVisible(false);

#if defined(Q_OS_MACOS)
  progress_bar->setFixedHeight(14);
  progress_bar->setMinimumWidth(360);
#endif

  auto* cancel_button = new QPushButton(QCoreApplication::tr("Cancel"));

  auto* button_layout = new QHBoxLayout;
  button_layout->addStretch();
  button_layout->addWidget(cancel_button);

  auto* layout = new QVBoxLayout(dialog);
  layout->setContentsMargins(24, 20, 24, 20);
  layout->setSpacing(12);
  layout->addWidget(title_label);
  layout->addWidget(message_label);
  layout->addSpacing(4);
  layout->addWidget(progress_bar);
  layout->addWidget(hint_label);
  layout->addSpacing(4);
  layout->addLayout(button_layout);

  dialog->resize(460, dialog->sizeHint().height());

  QEventLoop looper;

  const auto close_dialog = [dialog]() {
    LOG_D() << "closing env checking dialog";

    if (dialog != nullptr) {
      dialog->accept();
      dialog->deleteLater();
    }
  };

  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalCoreFullyLoaded, dialog,
                        [close_dialog]() {
                          LOG_D() << "ui caught signal: core fully loaded";
                          close_dialog();
                        });

  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalBadOpenPGPEnv, dialog,
                        [close_dialog]() {
                          LOG_D() << "ui caught signal: core loading failed";
                          close_dialog();
                        });

  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalCoreFullyLoaded, &looper,
                        &QEventLoop::quit);

  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalBadOpenPGPEnv, &looper,
                        &QEventLoop::quit);

  QApplication::connect(cancel_button, &QPushButton::clicked, dialog, []() {
    FLOG_D("cancel clicked on waiting dialog");
    TerminateSelfImmediately();
  });

  QApplication::connect(dialog, &QDialog::rejected, dialog, []() {
    FLOG_D("waiting dialog rejected");
    TerminateSelfImmediately();
  });

  // The connections above already subscribe to the core's terminal state. They
  // are edge-triggered and cross-thread, though, so they only fire reliably
  // once looper.exec() is running. The gap is dialog->show(): on macOS it can
  // pump the event loop, delivering a queued QEventLoop::quit before exec()
  // starts -- where it is a no-op -- and the dialog would then wait forever
  // even though the core is already initialized (exactly the fresh-install
  // stall).
  dialog->show();
  dialog->raise();
  dialog->activateWindow();

  // Authoritative re-check, placed after show() (the only thing that pumps
  // events here) and immediately before blocking. The monitor publishes
  // env.state.* before emitting its signals, so reading the state now closes
  // the window the signals leave open -- no polling needed. From here to
  // looper.exec() nothing pumps events, so any later transition is safely
  // delivered to the running loop.
  const auto core_reached_terminal_state = []() -> bool {
    return Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.all",
                                                   0) == 1 ||
           Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.basic",
                                                   0) < 0;
  };

  if (core_reached_terminal_state()) {
    LOG_D()
        << "core reached terminal state before waiting; skipping event loop";
    close_dialog();
    return;
  }

  looper.exec();
}

}  // namespace

extern void InitUITranslations();

void PreInitGpgFrontendUI() { CommonUtils::GetInstance(); }

void SetFusionAsDefaultStyle() {
  // Set Fusion style for better dark mode support across platforms
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  // Check if system is using dark mode by comparing text/background lightness
  QPalette system_palette = QApplication::palette();
  QColor window_color = system_palette.color(QPalette::Window);
  QColor text_color = system_palette.color(QPalette::WindowText);

  // In dark themes, text is typically lighter than the background
  bool is_dark_mode = text_color.lightness() > window_color.lightness();
  LOG_D() << "dark mode status:" << is_dark_mode;

  if (is_dark_mode) {
    LOG_D() << "applying dark palette...";

    // Apply dark palette for Fusion
    QPalette dark_palette;
    dark_palette.setColor(QPalette::Window, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::WindowText, Qt::white);
    dark_palette.setColor(QPalette::Base, QColor(25, 25, 25));
    dark_palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::ToolTipBase, Qt::black);
    dark_palette.setColor(QPalette::ToolTipText, Qt::white);
    dark_palette.setColor(QPalette::Text, Qt::white);
    dark_palette.setColor(QPalette::Button, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::ButtonText, Qt::white);
    dark_palette.setColor(QPalette::BrightText, Qt::red);
    dark_palette.setColor(QPalette::Link, QColor(42, 130, 218));
    dark_palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    dark_palette.setColor(QPalette::HighlightedText, Qt::black);

    // Apply the dark palette
    QApplication::setPalette(dark_palette);
  }
}

void InitGpgFrontendUI(QApplication* app) {
  // init locale
  InitUITranslations();

  auto settings = GetSettings();
  auto theme = settings.value("appearance/theme").toString();

  // make appimage version look better; also default to Fusion on macOS
#if defined(Q_OS_MACOS)
  if (theme.isEmpty()) SetFusionAsDefaultStyle();
#else
  if (IsAppImageENV()) SetFusionAsDefaultStyle();
#endif

  // If user has explicitly set a theme in settings, use that instead
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
      FLOG_W("setting operation error: proxy setings");
      // no proxy by default
      QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
  } else {
    // no proxy by default
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
  }
}

void WaitingAllInitializationFinished() {
  if (Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.all", 0) ==
      0) {
    LOG_D() << "ui init is done, but core doesn't, going to wait for core...";
    WaitEnvCheckingProcess();
  }
  LOG_D() << "application fully initialized...";
}

auto RunGpgFrontendUI(QApplication* app) -> int {
  // create main window and show it
  auto main_window = QSharedPointer<GpgFrontend::UI::MainWindow>::create();
  main_window->setAttribute(Qt::WA_DeleteOnClose, false);

  // pre-check, if application need to restart
  if (CommonUtils::GetInstance()->IsApplicationNeedRestart()) {
    FLOG_D("application need to restart, before main window init.");
    return kDeepRestartCode;
  }

  LOG_D() << "main window start to initialize...";

  // init main window
  main_window->Init();

  // show main windows
  main_window->show();

  return QApplication::exec();
}

void GF_UI_EXPORT DestroyGpgFrontendUI() {}

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
    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
  }

  translator = new QTranslator(QCoreApplication::instance());
  if (translator->load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"),
                       QLatin1String(":/i18n_qt"), QLatin1String(".qm"))) {
    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
  }

  translator = new QTranslator(QCoreApplication::instance());
  if (translator->load(QLocale(), QLatin1String(PROJECT_NAME),
                       QLatin1String("."), QLatin1String(":/i18n"),
                       QLatin1String(".qm"))) {
    QCoreApplication::installTranslator(translator);
    registered_translators.append(translator);
  }
}

void InitModulesTranslations() {
  // register module's translations
  UIModuleManager::GetInstance().RegisterAllModuleTranslators();
}

}  // namespace GpgFrontend::UI
