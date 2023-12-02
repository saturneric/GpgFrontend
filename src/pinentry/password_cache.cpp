/* password-cache.c - Password cache support.
   Copyright (C) 2015 g10 Code GmbH

   This file is part of PINENTRY.

   PINENTRY is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   PINENTRY is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.
   SPDX-License-Identifier: GPL-2.0+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif

#include "password_cache.h"
#include "secmem.h"

#ifdef HAVE_LIBSECRET
static const SecretSchema *gpg_schema(void) {
  static const SecretSchema the_schema = {
      "org.gnupg.Passphrase",
      SECRET_SCHEMA_NONE,
      {
          {"stored-by", SECRET_SCHEMA_ATTRIBUTE_STRING},
          {"keygrip", SECRET_SCHEMA_ATTRIBUTE_STRING},
          {"NULL", 0},
      }};
  return &the_schema;
}

static char *keygrip_to_label(const char *keygrip) {
  char const prefix[] = "GnuPG: ";
  char *label;

  label = malloc(sizeof(prefix) + strlen(keygrip));
  if (label) {
    memcpy(label, prefix, sizeof(prefix) - 1);
    strcpy(&label[sizeof(prefix) - 1], keygrip);
  }
  return label;
}
#endif

void password_cache_save(const char *keygrip, const char *password) {
#ifdef HAVE_LIBSECRET
  char *label;
  GError *error = NULL;

  if (!*keygrip) return;

  label = keygrip_to_label(keygrip);
  if (!label) return;

  if (!secret_password_store_sync(gpg_schema(), SECRET_COLLECTION_DEFAULT,
                                  label, password, NULL, &error, "stored-by",
                                  "GnuPG Pinentry", "keygrip", keygrip, NULL)) {
    fprintf(stderr,
            "Failed to cache password for key %s with secret service: %s\n",
            keygrip, error->message);

    g_error_free(error);
  }

  free(label);
#else
  (void)keygrip;
  (void)password;
  return;
#endif
}

char *password_cache_lookup(const char *keygrip, int *fatal_error) {
#ifdef HAVE_LIBSECRET
  GError *error = NULL;
  char *password;
  char *password2;

  if (!*keygrip) return NULL;

  password = secret_password_lookup_nonpageable_sync(gpg_schema(), NULL, &error,
                                                     "keygrip", keygrip, NULL);

  if (error != NULL) {
    if (fatal_error) *fatal_error = 1;

    fprintf(stderr,
            "Failed to lookup password for key %s with secret service: %s\n",
            keygrip, error->message);
    g_error_free(error);
    return NULL;
  }
  if (!password)
    /* The password for this key is not cached.  Just return NULL.  */
    return NULL;

  /* The password needs to be returned in secmem allocated memory.  */
  password2 = secmem_malloc(strlen(password) + 1);
  if (password2)
    strcpy(password2, password);
  else
    fprintf(stderr, "secmem_malloc failed: can't copy password!\n");

  secret_password_free(password);

  return password2;
#else
  (void)keygrip;
  (void)fatal_error;
  return NULL;
#endif
}

/* Try and remove the cached password for key grip.  Returns -1 on
   error, 0 if the key is not found and 1 if the password was
   removed.  */
int password_cache_clear(const char *keygrip) {
#ifdef HAVE_LIBSECRET
  GError *error = NULL;
  int removed = secret_password_clear_sync(gpg_schema(), NULL, &error,
                                           "keygrip", keygrip, NULL);
  if (error != NULL) {
    fprintf(stderr,
            "Failed to clear password for key %s with secret service: %s\n",
            keygrip, error->message);
    g_debug("%s", error->message);
    g_error_free(error);
    return -1;
  }
  if (removed) return 1;
  return 0;
#else
  (void)keygrip;
  return -1;
#endif
}
