/* pinentry.c - The PIN entry support library
 * Copyright (C) 2002, 2003, 2007, 2008, 2010, 2015, 2016, 2021 g10 Code GmbH
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WINDOWS
#include <errno.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef WINDOWS
#include <sys/utsname.h>
#endif
#ifndef WINDOWS
#include <locale.h>
#endif
#include <limits.h>
#ifdef WINDOWS
#include <windows.h>
#endif

#include <assuan.h>

#include "core/utils/MemoryUtils.h"
#include "pinentry.h"

#ifdef WINDOWS
#define getpid() GetCurrentProcessId()
#endif

/* Keep the name of our program here. */
static char this_pgmname[50];

struct pinentry pinentry;

static const char *flavor_flag;

/* Because gtk_init removes the --display arg from the command lines
 * and our command line parser is called after gtk_init (so that it
 * does not see gtk specific options) we don't have a way to get hold
 * of the --display option.  Our solution is to remember --display in
 * the call to pinentry_have_display and set it then in our
 * parser.  */
static char *remember_display;

static void pinentry_reset(int use_defaults) {
  /* GPG Agent sets these options once when it starts the pinentry.
     Don't reset them.  */
  int grab = pinentry.grab;
  char *ttyname = pinentry.ttyname;
  char *ttytype = pinentry.ttytype_l;
  char *ttyalert = pinentry.ttyalert;
  char *lc_ctype = pinentry.lc_ctype;
  char *lc_messages = pinentry.lc_messages;
  int allow_external_password_cache = pinentry.allow_external_password_cache;
  char *default_ok = pinentry.default_ok;
  char *default_cancel = pinentry.default_cancel;
  char *default_prompt = pinentry.default_prompt;
  char *default_pwmngr = pinentry.default_pwmngr;
  char *default_cf_visi = pinentry.default_cf_visi;
  char *default_tt_visi = pinentry.default_tt_visi;
  char *default_tt_hide = pinentry.default_tt_hide;
  char *default_capshint = pinentry.default_capshint;
  char *touch_file = pinentry.touch_file;
  unsigned long owner_pid = pinentry.owner_pid;
  int owner_uid = pinentry.owner_uid;
  char *owner_host = pinentry.owner_host;
  int constraints_enforce = pinentry.constraints_enforce;
  char *constraints_hint_short = pinentry.constraints_hint_short;
  char *constraints_hint_long = pinentry.constraints_hint_long;
  char *constraints_error_title = pinentry.constraints_error_title;

  /* These options are set from the command line.  Don't reset
     them.  */
  int debug = pinentry.debug;
  char *display = pinentry.display;
  int parent_wid = pinentry.parent_wid;

  pinentry_color_t color_fg = pinentry.color_fg;
  int color_fg_bright = pinentry.color_fg_bright;
  pinentry_color_t color_bg = pinentry.color_bg;
  pinentry_color_t color_so = pinentry.color_so;
  int color_so_bright = pinentry.color_so_bright;
  pinentry_color_t color_ok = pinentry.color_ok;
  int color_ok_bright = pinentry.color_ok_bright;
  pinentry_color_t color_qualitybar = pinentry.color_qualitybar;
  int color_qualitybar_bright = pinentry.color_qualitybar_bright;

  int timeout = pinentry.timeout;

  char *invisible_char = pinentry.invisible_char;

  /* Free any allocated memory.  */
  if (use_defaults) {
    free(pinentry.ttyname);
    free(pinentry.ttytype_l);
    free(pinentry.ttyalert);
    free(pinentry.lc_ctype);
    free(pinentry.lc_messages);
    free(pinentry.default_ok);
    free(pinentry.default_cancel);
    free(pinentry.default_prompt);
    free(pinentry.default_pwmngr);
    free(pinentry.default_cf_visi);
    free(pinentry.default_tt_visi);
    free(pinentry.default_tt_hide);
    free(pinentry.default_capshint);
    free(pinentry.touch_file);
    free(pinentry.owner_host);
    free(pinentry.display);
    free(pinentry.constraints_hint_short);
    free(pinentry.constraints_hint_long);
    free(pinentry.constraints_error_title);
  }

  free(pinentry.title);
  free(pinentry.description);
  free(pinentry.error);
  free(pinentry.prompt);
  free(pinentry.ok);
  free(pinentry.notok);
  free(pinentry.cancel);
  GpgFrontend::SecureFree(pinentry.pin);
  free(pinentry.repeat_passphrase);
  free(pinentry.repeat_error_string);
  free(pinentry.quality_bar);
  free(pinentry.quality_bar_tt);
  free(pinentry.formatted_passphrase_hint);
  free(pinentry.keyinfo);
  free(pinentry.specific_err_info);

  /* Reset the pinentry structure.  */
  memset(&pinentry, 0, sizeof(pinentry));

  /* Restore options without a default we want to preserve.  */
  pinentry.invisible_char = invisible_char;

  /* Restore other options or set defaults.  */

  if (use_defaults) {
    /* Pinentry timeout in seconds.  */
    pinentry.timeout = 60;

    /* Global grab.  */
    pinentry.grab = 1;

    pinentry.color_fg = PINENTRY_COLOR_DEFAULT;
    pinentry.color_fg_bright = 0;
    pinentry.color_bg = PINENTRY_COLOR_DEFAULT;
    pinentry.color_so = PINENTRY_COLOR_DEFAULT;
    pinentry.color_so_bright = 0;
    pinentry.color_ok = PINENTRY_COLOR_DEFAULT;
    pinentry.color_ok_bright = 0;
    pinentry.color_qualitybar = PINENTRY_COLOR_DEFAULT;
    pinentry.color_qualitybar_bright = 0;

    pinentry.owner_uid = -1;
  } else /* Restore the options.  */
  {
    pinentry.grab = grab;
    pinentry.ttyname = ttyname;
    pinentry.ttytype_l = ttytype;
    pinentry.ttyalert = ttyalert;
    pinentry.lc_ctype = lc_ctype;
    pinentry.lc_messages = lc_messages;
    pinentry.allow_external_password_cache = allow_external_password_cache;
    pinentry.default_ok = default_ok;
    pinentry.default_cancel = default_cancel;
    pinentry.default_prompt = default_prompt;
    pinentry.default_pwmngr = default_pwmngr;
    pinentry.default_cf_visi = default_cf_visi;
    pinentry.default_tt_visi = default_tt_visi;
    pinentry.default_tt_hide = default_tt_hide;
    pinentry.default_capshint = default_capshint;
    pinentry.touch_file = touch_file;
    pinentry.owner_pid = owner_pid;
    pinentry.owner_uid = owner_uid;
    pinentry.owner_host = owner_host;
    pinentry.constraints_enforce = constraints_enforce;
    pinentry.constraints_hint_short = constraints_hint_short;
    pinentry.constraints_hint_long = constraints_hint_long;
    pinentry.constraints_error_title = constraints_error_title;

    pinentry.debug = debug;
    pinentry.display = display;
    pinentry.parent_wid = parent_wid;

    pinentry.color_fg = color_fg;
    pinentry.color_fg_bright = color_fg_bright;
    pinentry.color_bg = color_bg;
    pinentry.color_so = color_so;
    pinentry.color_so_bright = color_so_bright;
    pinentry.color_ok = color_ok;
    pinentry.color_ok_bright = color_ok_bright;
    pinentry.color_qualitybar = color_qualitybar;
    pinentry.color_qualitybar_bright = color_qualitybar_bright;

    pinentry.timeout = timeout;
  }
}

static gpg_error_t pinentry_assuan_reset_handler(assuan_context_t ctx,
                                                 char *line) {
  (void)ctx;
  (void)line;

  pinentry_reset(0);

  return 0;
}

/* Copy TEXT or TEXTLEN to BUFFER and escape as required.  Return a
   pointer to the end of the new buffer.  Note that BUFFER must be
   large enough to keep the entire text; allocataing it 3 times of
   TEXTLEN is sufficient.  */
static char *copy_and_escape(char *buffer, const void *text, size_t textlen) {
  int i;
  const unsigned char *s = (unsigned char *)text;
  char *p = buffer;

  for (i = 0; i < textlen; i++) {
    if (s[i] < ' ' || s[i] == '+') {
      snprintf(p, 4, "%%%02X", s[i]);
      p += 3;
    } else if (s[i] == ' ')
      *p++ = '+';
    else
      *p++ = s[i];
  }
  return p;
}

/* Perform percent unescaping in STRING and return the new valid length
   of the string.  A terminating Nul character is inserted at the end of
   the unescaped string.
 */
static size_t do_unescape_inplace(char *s) {
  unsigned char *p, *p0;

  p = p0 = (unsigned char *)s;
  while (*s) {
    if (*s == '%' && s[1] && s[2]) {
      s++;
      *p++ = xtoi_2(s);
      s += 2;
    } else
      *p++ = *s++;
  }
  *p = 0;

  return (p - p0);
}

/* Return a malloced copy of the commandline for PID.  If this is not
 * possible NULL is returned.  */
#ifndef WINDOWS
static char *get_cmdline(unsigned long pid) {
  char buffer[200];
  FILE *fp;
  size_t i, n;

  snprintf(buffer, sizeof buffer, "/proc/%lu/cmdline", pid);

  fp = fopen(buffer, "rb");
  if (!fp) return NULL;
  n = fread(buffer, 1, sizeof buffer - 1, fp);
  if (n < sizeof buffer - 1 && ferror(fp)) {
    /* Some error occurred.  */
    fclose(fp);
    return NULL;
  }
  fclose(fp);
  if (n == 0) return NULL;
  /* Arguments are delimited by Nuls.  We should do proper quoting but
   * that can be a bit complicated, thus we simply replace the Nuls by
   * spaces.  */
  for (i = 0; i < n; i++)
    if (!buffer[i] && i < n - 1) buffer[i] = ' ';
  buffer[i] = 0; /* Make sure the last byte is the string terminator.  */

  return strdup(buffer);
}
#endif /*!WINDOWS*/

/* Atomically ask the kernel for information about process PID.
 * Return a malloc'ed copy of the process name as long as the process
 * uid matches UID.  If it cannot determine that the process has uid
 * UID, it returns NULL.
 *
 * This is not as informative as get_cmdline, but it verifies that the
 * process does belong to the user in question.
 */
#ifndef WINDOWS
static char *get_pid_name_for_uid(unsigned long pid, int uid) {
  char buffer[400];
  FILE *fp;
  size_t end, n;
  char *uidstr;

  snprintf(buffer, sizeof buffer, "/proc/%lu/status", pid);

  fp = fopen(buffer, "rb");
  if (!fp) return NULL;
  n = fread(buffer, 1, sizeof buffer - 1, fp);
  if (n < sizeof buffer - 1 && ferror(fp)) {
    /* Some error occurred.  */
    fclose(fp);
    return NULL;
  }
  fclose(fp);
  if (n == 0) return NULL;
  buffer[n] = 0;
  /* Fixme: Is it specified that "Name" is always the first line?  For
   * robustness I would prefer to have a real parser here. -wk  */
  if (strncmp(buffer, "Name:\t", 6)) return NULL;
  end = strcspn(buffer + 6, "\n") + 6;
  buffer[end] = 0;

  /* check that uid matches what we expect */
  uidstr = strstr(buffer + end + 1, "\nUid:\t");
  if (!uidstr) return NULL;
  if (atoi(uidstr + 6) != uid) return NULL;

  return strdup(buffer + 6);
}
#endif /*!WINDOWS*/

const char *pinentry_get_pgmname(void) { return this_pgmname; }

/* Return a malloced string with the title.  The caller mus free the
 * string.  If no title is available or the title string has an error
 * NULL is returned.  */
char *pinentry_get_title(pinentry_t pe) {
  char *title;

  if (pe->title) title = strdup(pe->title);
#ifndef WINDOWS
  else if (pe->owner_pid) {
    char buf[200];
    struct utsname utsbuf;
    char *pidname = NULL;
    char *cmdline = NULL;

    if (pe->owner_host && !uname(&utsbuf) &&
        !strcmp(utsbuf.nodename, pe->owner_host)) {
      pidname = get_pid_name_for_uid(pe->owner_pid, pe->owner_uid);
      if (pidname) cmdline = get_cmdline(pe->owner_pid);
    }

    if (pe->owner_host && (cmdline || pidname))
      snprintf(buf, sizeof buf, "[%lu]@%s (%s)", pe->owner_pid, pe->owner_host,
               cmdline ? cmdline : pidname);
    else if (pe->owner_host)
      snprintf(buf, sizeof buf, "[%lu]@%s", pe->owner_pid, pe->owner_host);
    else
      snprintf(buf, sizeof buf, "[%lu] <unknown host>", pe->owner_pid);
    free(pidname);
    free(cmdline);
    title = strdup(buf);
  }
#endif /*!WINDOWS*/
  else
    title = strdup(this_pgmname);

  return title;
}

/* Run a quality inquiry for PASSPHRASE of LENGTH.  (We need LENGTH
   because not all backends might be able to return a proper
   C-string.).  Returns: A value between -100 and 100 to give an
   estimate of the passphrase's quality.  Negative values are use if
   the caller won't even accept that passphrase.  Note that we expect
   just one data line which should not be escaped in any represent a
   numeric signed decimal value.  Extra data is currently ignored but
   should not be send at all.  */
int pinentry_inq_quality(pinentry_t pin, const char *passphrase,
                         size_t length) {
  assuan_context_t ctx = (assuan_context_t)pin->ctx_assuan;
  const char prefix[] = "INQUIRE QUALITY ";
  char *command;
  char *line;
  size_t linelen;
  int gotvalue = 0;
  int value = 0;
  int rc;

  if (!ctx) return 0; /* Can't run the callback.  */

  if (length > 300)
    length = 300; /* Limit so that it definitely fits into an Assuan
                     line.  */

  command =
      GpgFrontend::SecureMallocAsType<char>(strlen(prefix) + 3 * length + 1);
  if (!command) return 0;
  strcpy(command, prefix);
  copy_and_escape(command + strlen(command), passphrase, length);
  rc = assuan_write_line(ctx, command);
  GpgFrontend::SecureFree(command);
  if (rc) {
    fprintf(stderr, "ASSUAN WRITE LINE failed: rc=%d\n", rc);
    return 0;
  }

  for (;;) {
    do {
      rc = assuan_read_line(ctx, &line, &linelen);
      if (rc) {
        fprintf(stderr, "ASSUAN READ LINE failed: rc=%d\n", rc);
        return 0;
      }
    } while (*line == '#' || !linelen);
    if (line[0] == 'E' && line[1] == 'N' && line[2] == 'D' &&
        (!line[3] || line[3] == ' '))
      break; /* END command received*/
    if (line[0] == 'C' && line[1] == 'A' && line[2] == 'N' &&
        (!line[3] || line[3] == ' '))
      break; /* CAN command received*/
    if (line[0] == 'E' && line[1] == 'R' && line[2] == 'R' &&
        (!line[3] || line[3] == ' '))
      break; /* ERR command received*/
    if (line[0] != 'D' || line[1] != ' ' || linelen < 3 || gotvalue) continue;
    gotvalue = 1;
    value = atoi(line + 2);
  }
  if (value < -100)
    value = -100;
  else if (value > 100)
    value = 100;

  return value;
}

/* Run a checkpin inquiry */
char *pinentry_inq_checkpin(pinentry_t pin, const char *passphrase,
                            size_t length) {
  assuan_context_t ctx = (assuan_context_t)pin->ctx_assuan;
  const char prefix[] = "INQUIRE CHECKPIN ";
  char *command;
  char *line;
  size_t linelen;
  int gotvalue = 0;
  char *value = NULL;
  int rc;

  if (!ctx) return 0; /* Can't run the callback.  */

  if (length > 300)
    length = 300; /* Limit so that it definitely fits into an Assuan
                     line.  */

  command =
      GpgFrontend::SecureMallocAsType<char>(strlen(prefix) + 3 * length + 1);
  if (!command) return 0;
  strcpy(command, prefix);
  copy_and_escape(command + strlen(command), passphrase, length);
  rc = assuan_write_line(ctx, command);
  GpgFrontend::SecureFree(command);
  if (rc) {
    fprintf(stderr, "ASSUAN WRITE LINE failed: rc=%d\n", rc);
    return 0;
  }

  for (;;) {
    do {
      rc = assuan_read_line(ctx, &line, &linelen);
      if (rc) {
        fprintf(stderr, "ASSUAN READ LINE failed: rc=%d\n", rc);
        return 0;
      }
    } while (*line == '#' || !linelen);
    if (line[0] == 'E' && line[1] == 'N' && line[2] == 'D' &&
        (!line[3] || line[3] == ' '))
      break; /* END command received*/
    if (line[0] == 'C' && line[1] == 'A' && line[2] == 'N' &&
        (!line[3] || line[3] == ' '))
      break; /* CAN command received*/
    if (line[0] == 'E' && line[1] == 'R' && line[2] == 'R' &&
        (!line[3] || line[3] == ' '))
      break; /* ERR command received*/
    if (line[0] != 'D' || line[1] != ' ' || linelen < 3 || gotvalue) continue;
    gotvalue = 1;
    value = strdup(line + 2);
  }

  return value;
}

/* Run a genpin inquiry */
char *pinentry_inq_genpin(pinentry_t pin) {
  assuan_context_t ctx = (assuan_context_t)pin->ctx_assuan;
  const char prefix[] = "INQUIRE GENPIN";
  char *line;
  size_t linelen;
  int gotvalue = 0;
  char *value = NULL;
  int rc;

  if (!ctx) return 0; /* Can't run the callback.  */

  rc = assuan_write_line(ctx, prefix);
  if (rc) {
    fprintf(stderr, "ASSUAN WRITE LINE failed: rc=%d\n", rc);
    return 0;
  }

  for (;;) {
    do {
      rc = assuan_read_line(ctx, &line, &linelen);
      if (rc) {
        fprintf(stderr, "ASSUAN READ LINE failed: rc=%d\n", rc);
        free(value);
        return 0;
      }
    } while (*line == '#' || !linelen);
    if (line[0] == 'E' && line[1] == 'N' && line[2] == 'D' &&
        (!line[3] || line[3] == ' '))
      break; /* END command received*/
    if (line[0] == 'C' && line[1] == 'A' && line[2] == 'N' &&
        (!line[3] || line[3] == ' '))
      break; /* CAN command received*/
    if (line[0] == 'E' && line[1] == 'R' && line[2] == 'R' &&
        (!line[3] || line[3] == ' '))
      break; /* ERR command received*/
    if (line[0] != 'D' || line[1] != ' ' || linelen < 3 || gotvalue) continue;
    gotvalue = 1;
    value = strdup(line + 2);
  }

  return value;
}

/* Try to make room for at least LEN bytes in the pinentry.  Returns
   new buffer on success and 0 on failure or when the old buffer is
   sufficient.  */
char *pinentry_setbufferlen(pinentry_t pin, int len) {
  char *newp;

  if (pin->pin_len)
    assert(pin->pin);
  else
    assert(!pin->pin);

  if (len < 2048) len = 2048;

  if (len <= pin->pin_len) return pin->pin;

  newp = GpgFrontend::SecureReallocAsType<char>(pin->pin, len);
  if (newp) {
    pin->pin = newp;
    pin->pin_len = len;
  } else {
    GpgFrontend::SecureFree(pin->pin);
    pin->pin = 0;
    pin->pin_len = 0;
  }
  return newp;
}

static void pinentry_setbuffer_clear(pinentry_t pin) {
  if (!pin->pin) {
    assert(pin->pin_len == 0);
    return;
  }

  assert(pin->pin_len > 0);

  GpgFrontend::SecureFree(pin->pin);
  pin->pin = NULL;
  pin->pin_len = 0;
}

/* passphrase better be alloced with secmem_alloc.  */
void pinentry_setbuffer_use(pinentry_t pin, char *passphrase, int len) {
  if (!passphrase) {
    assert(len == 0);
    pinentry_setbuffer_clear(pin);

    return;
  }

  if (passphrase && len == 0) len = strlen(passphrase) + 1;

  if (pin->pin) GpgFrontend::SecureFree(pin->pin);

  pin->pin = passphrase;
  pin->pin_len = len;
}

static struct assuan_malloc_hooks assuan_malloc_hooks = {
    GpgFrontend::SecureMalloc, GpgFrontend::SecureRealloc,
    GpgFrontend::SecureFree};

/* Initialize the secure memory subsystem, drop privileges and return.
   Must be called early. */
void pinentry_init(const char *pgmname) {
  /* Store away our name. */
  if (strlen(pgmname) > sizeof this_pgmname - 2) abort();
  strcpy(this_pgmname, pgmname);

  gpgrt_check_version(NULL);

  assuan_set_malloc_hooks(&assuan_malloc_hooks);
}

/* Simple test to check whether DISPLAY is set or the option --display
   was given.  Used to decide whether the GUI or curses should be
   initialized.  */
int pinentry_have_display(int argc, char **argv) {
  int found = 0;

  for (; argc; argc--, argv++) {
    if (!strcmp(*argv, "--display")) {
      if (argv[1] && !remember_display) {
        remember_display = strdup(argv[1]);
        if (!remember_display) {
#ifndef WINDOWS
          fprintf(stderr, "%s: %s\n", this_pgmname, strerror(errno));
#endif
          exit(EXIT_FAILURE);
        }
      }
      found = 1;
      break;
    } else if (!strncmp(*argv, "--display=", 10)) {
      if (!remember_display) {
        remember_display = strdup(*argv + 10);
        if (!remember_display) {
#ifndef WINDOWS
          fprintf(stderr, "%s: %s\n", this_pgmname, strerror(errno));
#endif
          exit(EXIT_FAILURE);
        }
      }
      found = 1;
      break;
    }
  }

#ifndef WINDOWS
  {
    const char *s;
    s = getenv("DISPLAY");
    if (s && *s) found = 1;
  }
#endif

  return found;
}

/* Print usage information and and provide strings for help. */
static const char *my_strusage(int level) {
  const char *p;

  switch (level) {
    case 11:
      p = this_pgmname;
      break;
    case 12:
      p = "pinentry";
      break;
    case 13:
      p = 0;
      break;
    case 14:
      p = "Copyright (C) 2023 Saturneric";
      break;
    case 19:
      p = "Please report bugs to <eric@bktus.com>.\n";
      break;
    case 1:
    case 40: {
      static char *str;

      if (!str) {
        size_t n = 50 + strlen(this_pgmname);
        str = static_cast<char *>(malloc(n));
        if (str) {
          snprintf(str, n, "Usage: %s [options] (-h for help)", this_pgmname);
        }
      }
      p = str;
    } break;
    case 41:
      p = "Ask securely for a secret and print it to stdout.";
      break;

    case 42:
      p = "1"; /* Flag print 40 as part of 41. */
      break;

    default:
      p = NULL;
      break;
  }
  return p;
}

char *parse_color(char *arg, pinentry_color_t *color_p, int *bright_p) {
  static struct {
    const char *name;
    pinentry_color_t color;
  } colors[] = {
      {"none", PINENTRY_COLOR_NONE},   {"default", PINENTRY_COLOR_DEFAULT},
      {"black", PINENTRY_COLOR_BLACK}, {"red", PINENTRY_COLOR_RED},
      {"green", PINENTRY_COLOR_GREEN}, {"yellow", PINENTRY_COLOR_YELLOW},
      {"blue", PINENTRY_COLOR_BLUE},   {"magenta", PINENTRY_COLOR_MAGENTA},
      {"cyan", PINENTRY_COLOR_CYAN},   {"white", PINENTRY_COLOR_WHITE}};

  int i;
  char *new_arg;
  pinentry_color_t color = PINENTRY_COLOR_DEFAULT;

  if (!arg) return NULL;

  new_arg = strchr(arg, ',');
  if (new_arg) new_arg++;

  if (bright_p) {
    const char *bname[] = {"bright-", "bright", "bold-", "bold"};

    *bright_p = 0;
    for (i = 0; i < sizeof(bname) / sizeof(bname[0]); i++)
      if (!strncasecmp(arg, bname[i], strlen(bname[i]))) {
        *bright_p = 1;
        arg += strlen(bname[i]);
      }
  }

  for (i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
    if (!strncasecmp(arg, colors[i].name, strlen(colors[i].name)))
      color = colors[i].color;

  *color_p = color;
  return new_arg;
}

/* Set the optional flag used with getinfo. */
void pinentry_set_flavor_flag(const char *string) { flavor_flag = string; }

static gpg_error_t option_handler(assuan_context_t ctx, const char *key,
                                  const char *value) {
  (void)ctx;

  if (!strcmp(key, "no-grab") && !*value)
    pinentry.grab = 0;
  else if (!strcmp(key, "grab") && !*value)
    pinentry.grab = 1;
  else if (!strcmp(key, "debug-wait")) {
#ifndef WINDOWS
    fprintf(stderr, "%s: waiting for debugger - my pid is %u ...\n",
            this_pgmname, (unsigned int)getpid());
    sleep(*value ? atoi(value) : 5);
    fprintf(stderr, "%s: ... okay\n", this_pgmname);
#endif
  } else if (!strcmp(key, "display")) {
    if (pinentry.display) free(pinentry.display);
    pinentry.display = strdup(value);
    if (!pinentry.display) return gpg_error_from_syserror();
  } else if (!strcmp(key, "ttyname")) {
    if (pinentry.ttyname) free(pinentry.ttyname);
    pinentry.ttyname = strdup(value);
    if (!pinentry.ttyname) return gpg_error_from_syserror();
  } else if (!strcmp(key, "ttytype")) {
    if (pinentry.ttytype_l) free(pinentry.ttytype_l);
    pinentry.ttytype_l = strdup(value);
    if (!pinentry.ttytype_l) return gpg_error_from_syserror();
  } else if (!strcmp(key, "ttyalert")) {
    if (pinentry.ttyalert) free(pinentry.ttyalert);
    pinentry.ttyalert = strdup(value);
    if (!pinentry.ttyalert) return gpg_error_from_syserror();
  } else if (!strcmp(key, "lc-ctype")) {
    if (pinentry.lc_ctype) free(pinentry.lc_ctype);
    pinentry.lc_ctype = strdup(value);
    if (!pinentry.lc_ctype) return gpg_error_from_syserror();
  } else if (!strcmp(key, "lc-messages")) {
    if (pinentry.lc_messages) free(pinentry.lc_messages);
    pinentry.lc_messages = strdup(value);
    if (!pinentry.lc_messages) return gpg_error_from_syserror();
  } else if (!strcmp(key, "owner")) {
    long along;
    char *endp;

    free(pinentry.owner_host);
    pinentry.owner_host = NULL;
    pinentry.owner_uid = -1;
    pinentry.owner_pid = 0;

    errno = 0;
    along = strtol(value, &endp, 10);
    if (along && !errno) {
      pinentry.owner_pid = (unsigned long)along;
      if (*endp) {
        errno = 0;
        if (*endp == '/') { /* we have a uid */
          endp++;
          along = strtol(endp, &endp, 10);
          if (along >= 0 && !errno) pinentry.owner_uid = (int)along;
        }
        if (endp) {
          while (*endp == ' ') endp++;
          if (*endp) {
            pinentry.owner_host = strdup(endp);
            for (endp = pinentry.owner_host; *endp && *endp != ' '; endp++)
              ;
            *endp = 0;
          }
        }
      }
    }
  } else if (!strcmp(key, "parent-wid")) {
    pinentry.parent_wid = atoi(value);
    /* FIXME: Use strtol and add some error handling.  */
  } else if (!strcmp(key, "touch-file")) {
    if (pinentry.touch_file) free(pinentry.touch_file);
    pinentry.touch_file = strdup(value);
    if (!pinentry.touch_file) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-ok")) {
    pinentry.default_ok = strdup(value);
    if (!pinentry.default_ok) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-cancel")) {
    pinentry.default_cancel = strdup(value);
    if (!pinentry.default_cancel) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-prompt")) {
    pinentry.default_prompt = strdup(value);
    if (!pinentry.default_prompt) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-pwmngr")) {
    pinentry.default_pwmngr = strdup(value);
    if (!pinentry.default_pwmngr) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-cf-visi")) {
    pinentry.default_cf_visi = strdup(value);
    if (!pinentry.default_cf_visi) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-tt-visi")) {
    pinentry.default_tt_visi = strdup(value);
    if (!pinentry.default_tt_visi) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-tt-hide")) {
    pinentry.default_tt_hide = strdup(value);
    if (!pinentry.default_tt_hide) return gpg_error_from_syserror();
  } else if (!strcmp(key, "default-capshint")) {
    pinentry.default_capshint = strdup(value);
    if (!pinentry.default_capshint) return gpg_error_from_syserror();
  } else if (!strcmp(key, "allow-external-password-cache") && !*value) {
    pinentry.allow_external_password_cache = 1;
    pinentry.tried_password_cache = 0;
  } else if (!strcmp(key, "allow-emacs-prompt") && !*value) {
#ifdef INSIDE_EMACS
    pinentry_enable_emacs_cmd_handler();
#endif
  } else if (!strcmp(key, "invisible-char")) {
    if (pinentry.invisible_char) free(pinentry.invisible_char);
    pinentry.invisible_char = strdup(value);
    if (!pinentry.invisible_char) return gpg_error_from_syserror();
  } else if (!strcmp(key, "formatted-passphrase") && !*value) {
    pinentry.formatted_passphrase = 1;
  } else if (!strcmp(key, "formatted-passphrase-hint")) {
    if (pinentry.formatted_passphrase_hint)
      free(pinentry.formatted_passphrase_hint);
    pinentry.formatted_passphrase_hint = strdup(value);
    if (!pinentry.formatted_passphrase_hint) return gpg_error_from_syserror();
    do_unescape_inplace(pinentry.formatted_passphrase_hint);
  } else if (!strcmp(key, "constraints-enforce") && !*value)
    pinentry.constraints_enforce = 1;
  else if (!strcmp(key, "constraints-hint-short")) {
    if (pinentry.constraints_hint_short) free(pinentry.constraints_hint_short);
    pinentry.constraints_hint_short = strdup(value);
    if (!pinentry.constraints_hint_short) return gpg_error_from_syserror();
    do_unescape_inplace(pinentry.constraints_hint_short);
  } else if (!strcmp(key, "constraints-hint-long")) {
    if (pinentry.constraints_hint_long) free(pinentry.constraints_hint_long);
    pinentry.constraints_hint_long = strdup(value);
    if (!pinentry.constraints_hint_long) return gpg_error_from_syserror();
    do_unescape_inplace(pinentry.constraints_hint_long);
  } else if (!strcmp(key, "constraints-error-title")) {
    if (pinentry.constraints_error_title)
      free(pinentry.constraints_error_title);
    pinentry.constraints_error_title = strdup(value);
    if (!pinentry.constraints_error_title) return gpg_error_from_syserror();
    do_unescape_inplace(pinentry.constraints_error_title);
  } else
    return gpg_error(GPG_ERR_UNKNOWN_OPTION);
  return 0;
}

/* Note, that it is sufficient to allocate the target string D as
   long as the source string S, i.e.: strlen(s)+1; */
static void strcpy_escaped(char *d, const char *s) {
  while (*s) {
    if (*s == '%' && s[1] && s[2]) {
      s++;
      *d++ = xtoi_2(s);
      s += 2;
    } else
      *d++ = *s++;
  }
  *d = 0;
}

/* Return a staically allocated string with information on the mode,
 * uid, and gid of DEVICE.  On error "?" is returned if DEVICE is
 * NULL, "-" is returned.  */
static const char *device_stat_string(const char *device) {
#ifdef HAVE_STAT
  static char buf[40];
  struct stat st;

  if (!device || !*device) return "-";

  if (stat(device, &st)) return "?"; /* Error */
  snprintf(buf, sizeof buf, "%lo/%lu/%lu", (unsigned long)st.st_mode,
           (unsigned long)st.st_uid, (unsigned long)st.st_gid);
  return buf;
#else
  return "-";
#endif
}