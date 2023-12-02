/* main.cpp - A Qt dialog for PIN entry.
 * Copyright (C) 2002, 2008 Klarälvdalens Datakonsult AB (KDAB)
 * Copyright (C) 2003, 2021 g10 Code GmbH
 * Copyright 2007 Ingo Klöcker
 *
 * Written by Steffen Hansen <steffen@klaralvdalens-datakonsult.se>.
 * Modified by Marcus Brinkmann <marcus@g10code.de>.
 * Modified by Marc Mutz <marc@kdab.com>
 * Software engineering by Ingo Klöcker <dev@ingo-kloecker.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include "accessibility.h"
#include "keyboardfocusindication.h"
#include "pinentry.h"
#include "pinentryconfirm.h"
#include "pinentrydialog.h"
#include "util.h"
#if QT_VERSION >= 0x050000
#include <QWindow>
#endif

#include <errno.h>
#include <gpg-error.h>
#include <stdio.h>

#include <stdexcept>

#ifdef FALLBACK_CURSES
#include <pinentry-curses.h>
#endif

#if QT_VERSION >= 0x050000 && defined(QT_STATIC)
#include <QtPlugin>
#ifdef Q_OS_WIN
#include <shlobj.h>
#include <windows.h>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#elif defined(Q_OS_MAC)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
#else
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
#endif
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "pinentry_debug.h"

static QString escape_accel(const QString &s) {
  QString result;
  result.reserve(s.size());

  bool afterUnderscore = false;

  for (unsigned int i = 0, end = s.size(); i != end; ++i) {
    const QChar ch = s[i];
    if (ch == QLatin1Char('_')) {
      if (afterUnderscore) {  // escaped _
        result += QLatin1Char('_');
        afterUnderscore = false;
      } else {  // accel
        afterUnderscore = true;
      }
    } else {
      if (afterUnderscore ||         // accel
          ch == QLatin1Char('&')) {  // escape & from being interpreted by Qt
        result += QLatin1Char('&');
      }
      result += ch;
      afterUnderscore = false;
    }
  }

  if (afterUnderscore)
  // trailing single underscore: shouldn't happen, but deal with it robustly:
  {
    result += QLatin1Char('_');
  }

  return result;
}

namespace {
class InvalidUtf8 : public std::invalid_argument {
 public:
  InvalidUtf8() : std::invalid_argument("invalid utf8") {}
  ~InvalidUtf8() throw() {}
};
}  // namespace

static const bool GPG_AGENT_IS_PORTED_TO_ONLY_SEND_UTF8 = false;

static QString from_utf8(const char *s) {
  const QString result = QString::fromUtf8(s);
  if (result.contains(QChar::ReplacementCharacter)) {
    if (GPG_AGENT_IS_PORTED_TO_ONLY_SEND_UTF8) {
      throw InvalidUtf8();
    } else {
      return QString::fromLocal8Bit(s);
    }
  }

  return result;
}

static void setup_foreground_window(QWidget *widget, WId parentWid) {
#if QT_VERSION >= 0x050000
  /* For windows set the desktop window as the transient parent */
  QWindow *parentWindow = nullptr;
  if (parentWid) {
    parentWindow = QWindow::fromWinId(parentWid);
  }
#ifdef Q_OS_WIN
  if (!parentWindow) {
    HWND desktop = GetDesktopWindow();
    if (desktop) {
      parentWindow = QWindow::fromWinId((WId)desktop);
    }
  }
#endif
  if (parentWindow) {
    // Ensure that we have a native wid
    widget->winId();
    QWindow *wndHandle = widget->windowHandle();

    if (wndHandle) {
      wndHandle->setTransientParent(parentWindow);
    }
  }
#endif
  widget->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint |
                         Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                         Qt::WindowStaysOnTopHint |
                         Qt::WindowMinimizeButtonHint);
}

static int qt_cmd_handler(pinentry_t pe) {
  int want_pass = !!pe->pin;

  const QString ok = pe->ok           ? escape_accel(from_utf8(pe->ok))
                     : pe->default_ok ? escape_accel(from_utf8(pe->default_ok))
                                      :
                                      /* else */ QLatin1String("&OK");
  const QString cancel = pe->cancel ? escape_accel(from_utf8(pe->cancel))
                         : pe->default_cancel
                             ? escape_accel(from_utf8(pe->default_cancel))
                             :
                             /* else */ QLatin1String("&Cancel");

  unique_malloced_ptr<char> str{pinentry_get_title(pe)};
  const QString title = str ? from_utf8(str.get()) :
                            /* else */ QLatin1String("pinentry-qt");

  const QString repeatError = pe->repeat_error_string
                                  ? from_utf8(pe->repeat_error_string)
                                  : QLatin1String("Passphrases do not match");
  const QString repeatString =
      pe->repeat_passphrase ? from_utf8(pe->repeat_passphrase) : QString();
  const QString visibilityTT = pe->default_tt_visi
                                   ? from_utf8(pe->default_tt_visi)
                                   : QLatin1String("Show passphrase");
  const QString hideTT = pe->default_tt_hide ? from_utf8(pe->default_tt_hide)
                                             : QLatin1String("Hide passphrase");

  const QString capsLockHint = pe->default_capshint
                                   ? from_utf8(pe->default_capshint)
                                   : QLatin1String("Caps Lock is on");

  const QString generateLbl =
      pe->genpin_label ? from_utf8(pe->genpin_label) : QString();
  const QString generateTT =
      pe->genpin_tt ? from_utf8(pe->genpin_tt) : QString();

  if (want_pass) {
    PinEntryDialog pinentry(nullptr, 0, pe->timeout, true, !!pe->quality_bar,
                            repeatString, visibilityTT, hideTT);
    setup_foreground_window(&pinentry, pe->parent_wid);
    pinentry.setPinentryInfo(pe);
    pinentry.setPrompt(escape_accel(from_utf8(pe->prompt)));
    pinentry.setDescription(from_utf8(pe->description));
    pinentry.setRepeatErrorText(repeatError);
    pinentry.setGenpinLabel(generateLbl);
    pinentry.setGenpinTT(generateTT);
    pinentry.setCapsLockHint(capsLockHint);
    pinentry.setFormattedPassphrase({bool(pe->formatted_passphrase),
                                     from_utf8(pe->formatted_passphrase_hint)});
    pinentry.setConstraintsOptions({bool(pe->constraints_enforce),
                                    from_utf8(pe->constraints_hint_short),
                                    from_utf8(pe->constraints_hint_long),
                                    from_utf8(pe->constraints_error_title)});

    if (!title.isEmpty()) {
      pinentry.setWindowTitle(title);
    }

    /* If we reuse the same dialog window.  */
    pinentry.setPin(QString());

    pinentry.setOkText(ok);
    pinentry.setCancelText(cancel);
    if (pe->error) {
      pinentry.setError(from_utf8(pe->error));
    }
    if (pe->quality_bar) {
      pinentry.setQualityBar(from_utf8(pe->quality_bar));
    }
    if (pe->quality_bar_tt) {
      pinentry.setQualityBarTT(from_utf8(pe->quality_bar_tt));
    }
    bool ret = pinentry.exec();
    if (!ret) {
      if (pinentry.timedOut()) pe->specific_err = gpg_error(GPG_ERR_TIMEOUT);
      return -1;
    }

    const QString pinStr = pinentry.pin();
    QByteArray pin = pinStr.toUtf8();

    if (!!pe->repeat_passphrase) {
      /* Should not have been possible to accept
         the dialog in that case but we do a safety
         check here */
      pe->repeat_okay = (pinStr == pinentry.repeatedPin());
    }

    int len = strlen(pin.constData());
    if (len >= 0) {
      pinentry_setbufferlen(pe, len + 1);
      if (pe->pin) {
        strcpy(pe->pin, pin.constData());
        return len;
      }
    }
    return -1;
  } else {
    const QString desc =
        pe->description ? from_utf8(pe->description) : QString();
    const QString notok =
        pe->notok ? escape_accel(from_utf8(pe->notok)) : QString();

    const QMessageBox::StandardButtons buttons =
        pe->one_button ? QMessageBox::Ok
        : pe->notok ? QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
                    :
                    /* else */ QMessageBox::Ok | QMessageBox::Cancel;

    PinentryConfirm box{QMessageBox::Information, title, desc, buttons};
    box.setTextFormat(Qt::PlainText);
    box.setTextInteractionFlags(Qt::TextSelectableByMouse);
    box.setTimeout(std::chrono::seconds{pe->timeout});
    setup_foreground_window(&box, pe->parent_wid);

    const struct {
      QMessageBox::StandardButton button;
      QString label;
    } buttonLabels[] = {
        {QMessageBox::Ok, ok},
        {QMessageBox::Yes, ok},
        {QMessageBox::No, notok},
        {QMessageBox::Cancel, cancel},
    };

    for (size_t i = 0; i < sizeof buttonLabels / sizeof *buttonLabels; ++i)
      if ((buttons & buttonLabels[i].button) &&
          !buttonLabels[i].label.isEmpty()) {
        box.button(buttonLabels[i].button)->setText(buttonLabels[i].label);
        Accessibility::setDescription(box.button(buttonLabels[i].button),
                                      buttonLabels[i].label);
      }

    box.setIconPixmap(applicationIconPixmap());

    if (!pe->one_button) {
      box.setDefaultButton(QMessageBox::Cancel);
    }

    box.show();
    raiseWindow(&box);

    const int rc = box.exec();

    if (rc == QMessageBox::Cancel) {
      pe->canceled = true;
    }
    if (box.timedOut()) {
      pe->specific_err = gpg_error(GPG_ERR_TIMEOUT);
    }

    return rc == QMessageBox::Ok || rc == QMessageBox::Yes;
  }
}

static int qt_cmd_handler_ex(pinentry_t pe) {
  try {
    return qt_cmd_handler(pe);
  } catch (const InvalidUtf8 &) {
    pe->locale_err = true;
    return pe->pin ? -1 : false;
  } catch (...) {
    pe->canceled = true;
    return pe->pin ? -1 : false;
  }
}

pinentry_cmd_handler_t pinentry_cmd_handler = qt_cmd_handler_ex;

int pinentry_main(int argc, char *argv[]) {
  pinentry_init("pinentry-qt");

  QApplication *app = NULL;
  int new_argc = 0;

#ifdef FALLBACK_CURSES
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
  // check a few environment variables that are usually set on X11 or Wayland
  // sessions
  const bool hasWaylandDisplay = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
  const bool isWaylandSessionType = qgetenv("XDG_SESSION_TYPE") == "wayland";
  const bool hasX11Display = pinentry_have_display(argc, argv);
  const bool isX11SessionType = qgetenv("XDG_SESSION_TYPE") == "x11";
  const bool isGUISession = hasWaylandDisplay || isWaylandSessionType ||
                            hasX11Display || isX11SessionType;
  qCDebug(PINENTRY_LOG) << "hasWaylandDisplay:" << hasWaylandDisplay;
  qCDebug(PINENTRY_LOG) << "isWaylandSessionType:" << isWaylandSessionType;
  qCDebug(PINENTRY_LOG) << "hasX11Display:" << hasX11Display;
  qCDebug(PINENTRY_LOG) << "isX11SessionType:" << isX11SessionType;
  qCDebug(PINENTRY_LOG) << "isGUISession:" << isGUISession;
#else
  const bool isGUISession = pinentry_have_display(argc, argv);
#endif
  if (!isGUISession) {
    pinentry_cmd_handler = curses_cmd_handler;
    pinentry_set_flavor_flag("curses");
  } else
#endif
  {
    /* Qt does only understand -display but not --display; thus we
       are fixing that here.  The code is pretty simply and may get
       confused if an argument is called "--display". */
    char **new_argv, *p;
    size_t n;
    int i, done;

    for (n = 0, i = 0; i < argc; i++) {
      n += strlen(argv[i]) + 1;
    }
    n++;
    new_argv = (char **)calloc(argc + 1, sizeof *new_argv);
    if (new_argv) {
      *new_argv = (char *)malloc(n);
    }
    if (!new_argv || !*new_argv) {
      fprintf(stderr, "pinentry-qt: can't fixup argument list: %s\n",
              strerror(errno));
      exit(EXIT_FAILURE);
    }
    for (done = 0, p = *new_argv, i = 0; i < argc; i++)
      if (!done && !strcmp(argv[i], "--display")) {
        new_argv[i] = strcpy(p, argv[i] + 1);
        p += strlen(argv[i] + 1) + 1;
        done = 1;
      } else {
        new_argv[i] = strcpy(p, argv[i]);
        p += strlen(argv[i]) + 1;
      }

    /* Note: QApplication uses int &argc so argc has to be valid
     * for the full lifetime of the application.
     *
     * As Qt might modify argc / argv we use copies here so that
     * we do not loose options that are handled in both. e.g. display.
     */
    new_argc = argc;
    Q_ASSERT(new_argc);
    app = new QApplication(new_argc, new_argv);
    app->setWindowIcon(QIcon(QLatin1String(":/document-encrypt.png")));
    (void)new KeyboardFocusIndication{app};
  }

  pinentry_parse_opts(argc, argv);

  int rc = pinentry_loop();
  delete app;
  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
