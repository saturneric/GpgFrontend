/* pinentry.h - The interface for the PIN entry support library.
 * Copyright (C) 2002, 2003, 2010, 2015, 2021 g10 Code GmbH
 *
 * This file is part of PINENTRY.
 *
 * PINENTRY is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * PINENTRY is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef PINENTRY_H
#define PINENTRY_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef enum {
  PINENTRY_COLOR_NONE,
  PINENTRY_COLOR_DEFAULT,
  PINENTRY_COLOR_BLACK,
  PINENTRY_COLOR_RED,
  PINENTRY_COLOR_GREEN,
  PINENTRY_COLOR_YELLOW,
  PINENTRY_COLOR_BLUE,
  PINENTRY_COLOR_MAGENTA,
  PINENTRY_COLOR_CYAN,
  PINENTRY_COLOR_WHITE
} pinentry_color_t;

struct pinentry {
  /* The window title, or NULL.  (Assuan: "SETTITLE TITLE".)  */
  char *title;
  /* The description to display, or NULL.  (Assuan: "SETDESC
     DESC".) */
  char *description;
  /* The error message to display, or NULL.  (Assuan: "SETERROR
     MESSAGE".) */
  char *error;
  /* The prompt to display, or NULL.  (Assuan: "SETPROMPT
     prompt".)  */
  char *prompt;
  /* The OK button text to display, or NULL.  (Assuan: "SETOK
     OK".)  */
  char *ok;
  /* The Not-OK button text to display, or NULL.  This is the text for
     the alternative option shown by the third button.  (Assuan:
     "SETNOTOK NOTOK".)  */
  char *notok;
  /* The Cancel button text to display, or NULL.  (Assuan: "SETCANCEL
     CANCEL".)  */
  char *cancel;

  /* The buffer to store the secret into.  */
  char *pin;
  /* The length of the buffer.  */
  int pin_len;
  /* Whether the pin was read from an external cache (1) or entered by
     the user (0). */
  int pin_from_cache;

  /* The name of the X display to use if X is available and supported.
     (Assuan: "OPTION display DISPLAY".)  */
  char *display;
  /* The name of the terminal node to open if X not available or
     supported.  (Assuan: "OPTION ttyname TTYNAME".)  */
  char *ttyname;
  /* The type of the terminal.  (Assuan: "OPTION ttytype TTYTYPE".)  */
  char *ttytype_l;
  /* Set the alert mode (none, beep or flash).  */
  char *ttyalert;
  /* The LC_CTYPE value for the terminal.  (Assuan: "OPTION lc-ctype
     LC_CTYPE".)  */
  char *lc_ctype;
  /* The LC_MESSAGES value for the terminal.  (Assuan: "OPTION
     lc-messages LC_MESSAGES".)  */
  char *lc_messages;

  /* True if debug mode is requested.  */
  int debug;

  /* The number of seconds before giving up while waiting for user input. */
  int timeout;

  /* True if caller should grab the keyboard.  (Assuan: "OPTION grab"
     or "OPTION no-grab".)  */
  int grab;

  /* The PID of the owner or 0 if not known.  The owner is the process
   * which actually triggered the the pinentry.  For example gpg.  */
  unsigned long owner_pid;

  /* The numeric uid (user ID) of the owner process or -1 if not
   * known. */
  int owner_uid;

  /* The malloced hostname of the owner or NULL.  */
  char *owner_host;

  /* The window ID of the parent window over which the pinentry window
     should be displayed.  (Assuan: "OPTION parent-wid WID".)  */
  int parent_wid;

  /* The name of an optional file which will be touched after a curses
     entry has been displayed.  (Assuan: "OPTION touch-file
     FILENAME".)  */
  char *touch_file;

  /* The frontend should set this to -1 if the user canceled the
     request, and to the length of the PIN stored in pin
     otherwise.  */
  int result;

  /* The frontend should set this if the NOTOK button was pressed.  */
  int canceled;

  /* The frontend should set this to true if an error with the local
     conversion occurred. */
  int locale_err;

  /* The frontend should set this to a gpg-error so that commands are
     able to return specific error codes.  This is an ugly hack due to
     the fact that pinentry_cmd_handler_t returns the length of the
     passphrase or a negative error code.  */
  int specific_err;

  /* The frontend may store a string with the error location here.  */
  const char *specific_err_loc;

  /* The frontend may store a malloced string here to emit an ERROR
   * status code with this extra info along with SPECIFIC_ERR.  */
  char *specific_err_info;

  /* The frontend should set this to true if the window close button
     has been used.  This flag is used in addition to a regular return
     value.  */
  int close_button;

  /* The caller should set this to true if only one button is
     required.  This is useful for notification dialogs where only a
     dismiss button is required. */
  int one_button;

  /* Whether this is a CONFIRM pinentry. */
  int confirm;

  /* If true a second prompt for the passphrase is shown and the user
     is expected to enter the same passphrase again.  Pinentry checks
     that both match.  (Assuan: "SETREPEAT".)  */
  char *repeat_passphrase;

  /* The string to show if a repeated passphrase does not match.
     (Assuan: "SETREPEATERROR ERROR".)  */
  char *repeat_error_string;

  /* The string to show if a repeated passphrase does match.
     (Assuan: "SETREPEATOK STRING".)  */
  char *repeat_ok_string;

  /* Set to true if the passphrase has been entered a second time and
     matches the first passphrase.  */
  int repeat_okay;

  /* If this is not NULL, a passphrase quality indicator is shown.
     There will also be an inquiry back to the caller to get an
     indication of the quality for the passphrase entered so far.  The
     string is used as a label for the quality bar.  (Assuan:
     "SETQUALITYBAR LABEL".)  */
  char *quality_bar;

  /* The tooltip to be shown for the qualitybar.  Malloced or NULL.
     (Assuan: "SETQUALITYBAR_TT TOOLTIP".)  */
  char *quality_bar_tt;

  /* If this is not NULL, a generate action should be shown.
     There will be an inquiry back to the caller to get such a
     PIN. generate action.  Malloced or NULL.
     (Assuan: "SETGENPIN LABEL" .)  */
  char *genpin_label;

  /* The tooltip to be shown for the generate action.  Malloced or NULL.
     (Assuan: "SETGENPIN_TT TOOLTIP".)  */
  char *genpin_tt;

  /* Specifies whether passphrase formatting should be enabled.
     (Assuan: "OPTION formatted-passphrase")  */
  int formatted_passphrase;

  /* A hint to be shown near the passphrase input field if passphrase
     formatting is enabled.  Malloced or NULL.
     (Assuan: "OPTION formatted-passphrase-hint=HINT".)  */
  char *formatted_passphrase_hint;

  /* For the curses pinentry, the color of error messages.  */
  pinentry_color_t color_fg;
  int color_fg_bright;
  pinentry_color_t color_bg;
  pinentry_color_t color_so;
  int color_so_bright;
  pinentry_color_t color_ok;
  int color_ok_bright;
  pinentry_color_t color_qualitybar;
  int color_qualitybar_bright;

  /* Malloced and i18ned default strings or NULL.  These strings may
     include an underscore character to indicate an accelerator key.
     A double underscore represents a plain one.  */
  /* (Assuan: "OPTION default-ok OK").  */
  char *default_ok;
  /* (Assuan: "OPTION default-cancel CANCEL").  */
  char *default_cancel;
  /* (Assuan: "OPTION default-prompt PROMPT").  */
  char *default_prompt;
  /* (Assuan: "OPTION default-pwmngr
     SAVE_PASSWORD_WITH_PASSWORD_MANAGER?").  */
  char *default_pwmngr;
  /* (Assuan: "OPTION default-cf-visi
     Do you really want to make your passphrase visible?").  */
  char *default_cf_visi;
  /* (Assuan: "OPTION default-tt-visi
     Make passphrase visible?").  */
  char *default_tt_visi;
  /* (Assuan: "OPTION default-tt-hide
     Hide passphrase").  */
  char *default_tt_hide;
  /* (Assuan: "OPTION default-capshint
     Caps Lock is on").  */
  char *default_capshint;

  /* Whether we are allowed to read the password from an external
     cache.  (Assuan: "OPTION allow-external-password-cache")  */
  int allow_external_password_cache;

  /* We only try the cache once.  */
  int tried_password_cache;

  /* A stable identifier for the key.  (Assuan: "SETKEYINFO
     KEYINFO".)  */
  char *keyinfo;

  /* Whether we may cache the password (according to the user).  */
  int may_cache_password;

  /* NOTE: If you add any additional fields to this structure, be sure
     to update the initializer in pinentry/pinentry.c!!!  */

  /* For the quality indicator and genpin we need to do an inquiry.
     Thus we need to save the assuan ctx.  */
  void *ctx_assuan;

  /* An UTF-8 string with an invisible character used to override the
     default in some pinentries.  Only the first character is
     used.  */
  char *invisible_char;

  /* Whether the passphrase constraints are enforced by gpg-agent.
     (Assuan: "OPTION constraints-enforce")  */
  int constraints_enforce;

  /* A short translated hint for the user with the constraints for new
     passphrases to be displayed near the passphrase input field.
     Malloced or NULL.
     (Assuan: "OPTION constraints-hint-short=At least 8 characters".)  */
  char *constraints_hint_short;

  /* A longer translated hint for the user with the constraints for new
     passphrases to be displayed for example as tooltip.  Malloced or NULL.
     (Assuan: "OPTION constraints-hint-long=The passphrase must ...".)  */
  char *constraints_hint_long;

  /* A short translated title for an error dialog informing the user about
     unsatisfied passphrase constraints.  Malloced or NULL.
     (Assuan: "OPTION constraints-error-title=Passphrase Not Allowed".)  */
  char *constraints_error_title;
};
typedef struct pinentry *pinentry_t;

/* The pinentry command handler type processes the pinentry request
   PIN.  If PIN->pin is zero, request a confirmation, otherwise a PIN
   entry.  On confirmation, the function should return TRUE if
   confirmed, and FALSE otherwise.  On PIN entry, the function should
   return -1 if an error occurred or the user cancelled the operation
   and 1 otherwise.  */
typedef int (*pinentry_cmd_handler_t)(pinentry_t pin);

const char *pinentry_get_pgmname(void);

char *pinentry_get_title(pinentry_t pe);

/* Run a quality inquiry for PASSPHRASE of LENGTH. */
int pinentry_inq_quality(const QString &passphrase);

/* Run a genpin iquriry. Returns a malloced string or NULL */
char *pinentry_inq_genpin(pinentry_t pin);

/* Try to make room for at least LEN bytes for the pin in the pinentry
   PIN.  Returns new buffer on success and 0 on failure.  */
char *pinentry_setbufferlen(pinentry_t pin, int len);

/* Return true if either DISPLAY is set or ARGV contains the string
   "--display". */
int pinentry_have_display(int argc, char **argv);

/* Parse the command line options.  May exit the program if only help
   or version output is requested.  */
void pinentry_parse_opts(int argc, char *argv[]);

/* Set the optional flag used with getinfo. */
void pinentry_set_flavor_flag(const char *string);

#ifdef WINDOWS
/* Windows declares sleep as obsolete, but provides a definition for
   _sleep but non for the still existing sleep.  */
#define sleep(a) _sleep((a))
#endif /*WINDOWS*/

#if 0
{
#endif
#ifdef __cplusplus
}
#endif

#endif /* PINENTRY_H */
