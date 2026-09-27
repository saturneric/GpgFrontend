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
#include "core/function/CoreInitProgress.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/function/ApplicationRestart.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/OpenPGPEnvGuard.h"
#include "ui/function/PassphrasePrompt.h"
#include "ui/function/UIStyle.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

namespace {

QContainer<QTranslator*> registered_translators;

[[noreturn]] void TerminateSelfImmediately() {
  qWarning() << "Application startup was canceled. Terminating process.";
  std::_Exit(0);
}

/**
 * @brief The wording for one startup step.
 *
 * The core reports a code, not a sentence, because it begins initializing
 * before InitUITranslations() has installed a translator -- a string built
 * there would resolve against an empty catalog and reach the user in English
 * whatever their language. So the wording lives here, and QCoreApplication::tr
 * keeps these strings in the same catalog context as the rest of this dialog.
 */
auto DescribeCoreInitStep(CoreInitStep step, const QString& subject)
    -> QString {
  switch (step) {
    case CoreInitStep::kSTARTING_UP:
      return QCoreApplication::tr("Starting up...");
    case CoreInitStep::kCHECKING_GNUPG_ENV:
      return QCoreApplication::tr("Checking the GnuPG environment...");
    case CoreInitStep::kCHECKING_RUST_ENGINE:
      return QCoreApplication::tr("Checking the rPGP engine...");
    case CoreInitStep::kRESOLVING_PATHS:
      return QCoreApplication::tr("Resolving GnuPG paths...");
    case CoreInitStep::kREFRESHING_BACKEND_ENGINE:
      return QCoreApplication::tr("Preparing the OpenPGP backend engine...");
    case CoreInitStep::kBUILDING_DEFAULT_CONTEXT:
      return QCoreApplication::tr("Building the default engine context...");
    case CoreInitStep::kLOADING_KEY_DATABASE:
      return subject.isEmpty()
                 ? QCoreApplication::tr("Loading key databases...")
                 : QCoreApplication::tr("Loading key database \"%1\"...")
                       .arg(subject);
    case CoreInitStep::kSCANNING_MODULES:
      return QCoreApplication::tr("Scanning modules...");
    case CoreInitStep::kVERIFYING_MODULES:
      return QCoreApplication::tr("Verifying modules...");
    case CoreInitStep::kLOADING_MODULE:
      return subject.isEmpty()
                 ? QCoreApplication::tr("Loading modules...")
                 : QCoreApplication::tr("Loading module \"%1\"...")
                       .arg(subject);
    case CoreInitStep::kREADY:
      return QCoreApplication::tr("Ready.");
  }
  return {};
}

/// The startup dialog's width, and what the layout leaves inside its margins.
constexpr int kStartupDialogWidth = 460;
constexpr int kStartupDialogMargin = 28;
constexpr int kStartupDialogContentWidth =
    kStartupDialogWidth - 2 * kStartupDialogMargin;
/// The badge beside the heading.
constexpr int kStartupLogoSize = 48;
/// The progress track: a slim rounded line rather than the platform's slab.
constexpr int kStartupProgressHeight = 6;
/// Space between the step line and the percentage beside it.
constexpr int kStartupStatusSpacing = 12;

/**
 * @brief The logo, heading and version line that open the startup dialog.
 *
 * Built here rather than with CreateDialogHeader(): that one ends in a rule
 * and suits a dialog that asks something, where this one only reports, and a
 * rule across a window this small cuts it in half.
 */
auto CreateStartupHeader(QWidget* parent) -> QLayout* {
  auto* logo_label = new QLabel(parent);
  const auto dpr = parent->devicePixelRatioF();
  auto logo = QPixmap(QStringLiteral(":/icons/gpgfrontend_logo.png"))
                  .scaled(QSize(kStartupLogoSize, kStartupLogoSize) * dpr,
                          Qt::KeepAspectRatio, Qt::SmoothTransformation);
  logo.setDevicePixelRatio(dpr);
  logo_label->setPixmap(logo);
  logo_label->setFixedSize(kStartupLogoSize, kStartupLogoSize);

  auto* title_label =
      new QLabel(QCoreApplication::tr("Preparing OpenPGP Environment"), parent);
  auto title_font = title_label->font();
  title_font.setBold(true);
  title_font.setPointSizeF(title_font.pointSizeF() * 1.3);
  title_label->setFont(title_font);

  auto* version_label = new QLabel(
      QStringLiteral("%1 %2").arg(GetProjectName(), GetProjectVersion()),
      parent);
  version_label->setTextFormat(Qt::PlainText);
  SetLabelTextColor(version_label, MutedTextColor(version_label->palette()));

  auto* text_column = new QVBoxLayout();
  text_column->setSpacing(2);
  text_column->addStretch();
  text_column->addWidget(title_label);
  text_column->addWidget(version_label);
  text_column->addStretch();

  auto* header = new QHBoxLayout();
  header->setSpacing(16);
  header->addWidget(logo_label, 0, Qt::AlignVCenter);
  header->addLayout(text_column, 1);
  return header;
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

  // A slim rounded line in the palette's own highlight, the same on every
  // platform. The percentage is its own label beside the step line: a number
  // painted inside a 6px track cannot be read, and one hung off its end by the
  // style floats free of everything else.
  auto* progress_bar = new QProgressBar;
  progress_bar->setRange(0, 100);
  progress_bar->setValue(0);
  progress_bar->setTextVisible(false);
  progress_bar->setFixedHeight(kStartupProgressHeight);
  const auto radius = kStartupProgressHeight / 2;
  progress_bar->setStyleSheet(
      QStringLiteral("QProgressBar { border: none; border-radius: %1px;"
                     " background: %2; }"
                     "QProgressBar::chunk { border-radius: %1px;"
                     " background: %3; }")
          .arg(radius)
          .arg(BorderColor(progress_bar->palette()).name(),
               progress_bar->palette().color(QPalette::Highlight).name()));

  // Reports arrive in jumps -- a whole key database or module at a time -- so
  // the bar glides to each new value instead of snapping to it.
  auto* progress_animation =
      new QPropertyAnimation(progress_bar, QByteArrayLiteral("value"), dialog);
  progress_animation->setDuration(250);
  progress_animation->setEasingCurve(QEasingCurve::OutCubic);

  // The one line that says what is actually happening. Elided rather than
  // wrapped: a key database is a filesystem path, and letting one wrap would
  // resize the dialog every few hundred milliseconds during a start.
  auto* detail_label = new QLabel;
  detail_label->setWordWrap(false);
  detail_label->setTextFormat(Qt::PlainText);
  SetLabelTextColor(detail_label, MutedTextColor(detail_label->palette()));

  // Wide enough for "100%" from the start, so the step line beside it never
  // shifts as the number grows a digit.
  auto* percent_label = new QLabel(QStringLiteral("0%"));
  percent_label->setTextFormat(Qt::PlainText);
  percent_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  percent_label->setFixedWidth(
      percent_label->fontMetrics().horizontalAdvance(QStringLiteral("100%")));
  SetLabelTextColor(percent_label, MutedTextColor(percent_label->palette()));
  // Follows the bar rather than the report, so the number counts up with the
  // glide instead of racing ahead of it.
  QObject::connect(progress_bar, &QProgressBar::valueChanged, percent_label,
                   [percent_label](int value) {
                     percent_label->setText(QStringLiteral("%1%").arg(value));
                   });

  auto* status_layout = new QHBoxLayout;
  status_layout->setSpacing(kStartupStatusSpacing);
  status_layout->addWidget(detail_label, 1);
  status_layout->addWidget(percent_label);

  auto* cancel_button = new QPushButton(QCoreApplication::tr("Cancel"));

  auto* button_layout = new QHBoxLayout;
  button_layout->addStretch();
  button_layout->addWidget(cancel_button);

  auto* layout = new QVBoxLayout(dialog);
  layout->setContentsMargins(kStartupDialogMargin, 24, kStartupDialogMargin,
                             20);
  layout->setSpacing(0);
  layout->addLayout(CreateStartupHeader(dialog));
  layout->addSpacing(22);
  layout->addWidget(progress_bar);
  layout->addSpacing(8);
  layout->addLayout(status_layout);
  layout->addSpacing(18);
  layout->addLayout(button_layout);

  QEventLoop looper;

  const auto apply_progress = [progress_bar, progress_animation, detail_label,
                               percent_label](int percent, CoreInitStep step,
                                              const QString& subject) {
    // Glide forward; a report that goes back (a restarted phase) is shown as
    // it is rather than animated in reverse.
    progress_animation->stop();
    if (percent > progress_bar->value()) {
      progress_animation->setStartValue(progress_bar->value());
      progress_animation->setEndValue(percent);
      progress_animation->start();
    } else {
      progress_bar->setValue(percent);
    }

    const auto text = DescribeCoreInitStep(step, subject);

    // Reports can arrive before the dialog is mapped, when the label has no
    // width yet and eliding against it would leave nothing but the ellipsis.
    // Fall back to what the layout will give it: the dialog's width less its
    // horizontal margins and the percentage beside it.
    auto available = detail_label->width();
    const auto fallback = kStartupDialogContentWidth - kStartupStatusSpacing -
                          percent_label->width();
    if (available < fallback) available = fallback;

    detail_label->setText(detail_label->fontMetrics().elidedText(
        text, Qt::ElideMiddle, available));
    detail_label->setToolTip(text);
  };

  // Something true on the line before the first report arrives.
  apply_progress(0, CoreInitStep::kSTARTING_UP, {});

  // Fixed, not merely resized: the step line changes several times a second
  // and nothing it says should be able to move the window.
  dialog->setFixedSize(kStartupDialogWidth, dialog->sizeHint().height());

  QApplication::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalCoreInitProgress, dialog,
      [apply_progress](int percent, CoreInitStep step, QString subject) {
        apply_progress(percent, step, subject);
      });

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

  // Same reasoning as the terminal-state re-check below, applied to the bar
  // itself: SignalCoreInitProgress is edge-triggered too, so every report that
  // landed before this dialog existed was delivered to nobody. Without this
  // the bar reads 0% for the whole of a fast start, which is the case it was
  // least useful in already.
  {
    auto& progress = CoreInitProgress::GetInstance();
    apply_progress(progress.Percent(), progress.CurrentStep(),
                   progress.CurrentSubject());
  }

  if (core_reached_terminal_state()) {
    LOG_D()
        << "core reached terminal state before waiting; skipping event loop";
    close_dialog();
    return;
  }

  looper.exec();
}

}  // namespace

void PreInitGpgFrontendUI() {
  // These must be subscribed before the core can report anything: the
  // bad-environment path ends in std::exit(0), so a handler installed later
  // never hears it at all. Each one is idempotent.
  InstallBadOpenPGPEnvHandler();
  InstallRestartHandler();
  InstallKeyDatabaseRefreshHandler();
  InstallPassphrasePromptHandler();
}

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
  auto theme = settings.value("appearance/theme").toString().toLower();

  // Resolve the effective style. If the user has explicitly chosen a valid
  // style, honor it; otherwise fall back to Fusion on macOS and on the AppImage
  // build to make them look better.
  auto available_styles = QStyleFactory::keys();
  for (QString& s : available_styles) s = s.toLower();

  QString effective_style;
  if (!theme.isEmpty() && available_styles.contains(theme)) {
    effective_style = theme;
  } else {
#if defined(Q_OS_MACOS)
    effective_style = "fusion";
#else
    if (IsAppImageENV()) effective_style = "fusion";
#endif
  }

  // Apply the resolved style.
#if defined(Q_OS_MACOS)
  // On macOS apply the style plainly. SetFusionAsDefaultStyle's hand-rolled
  // dark palette does not fit the platform; Fusion picks up the native system
  // palette on its own.
  if (!effective_style.isEmpty()) {
    QApplication::setStyle(QStyleFactory::create(effective_style));
  }
#else
  // Elsewhere, Fusion goes through the dedicated default treatment (which also
  // sets the dark palette), regardless of whether it comes from the fallback
  // above or from an explicitly saved setting. This keeps the appearance
  // identical across launches: previously the dark palette was only applied
  // when no theme was saved, so once "fusion" got persisted (e.g. by opening
  // and closing the settings dialog) later launches silently dropped it.
  if (effective_style.compare("fusion", Qt::CaseInsensitive) == 0) {
    SetFusionAsDefaultStyle();
  } else if (!effective_style.isEmpty()) {
    QApplication::setStyle(QStyleFactory::create(effective_style));
  }
#endif

  // init signal station
  UISignalStation::GetInstance();

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
  if (IsApplicationNeedRestart()) {
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
  // Uninstall *and* destroy: this runs again on every language switch and on
  // every restart loop pass, so merely forgetting the pointers would pile up
  // translators that qApp still consults.
  for (auto* translator : registered_translators) {
    QCoreApplication::removeTranslator(translator);
    delete translator;
  }
  registered_translators.clear();

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
