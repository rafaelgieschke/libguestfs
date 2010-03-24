/* libguestfs - the guestfsd daemon
 * Copyright (C) 2009 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/guestfs_protocol.h"
#include "daemon.h"
#include "c-ctype.h"
#include "actions.h"

/* Choose which tools like mke2fs to use.  For RHEL 5 (only) there
 * is a special set of tools which support ext2/3/4.  eg. On RHEL 5,
 * mke2fs only supports ext2/3, but mke4fs supports ext2/3/4.
 *
 * We specify e4fsprogs in the package list to ensure it is loaded
 * if it exists.
 */
static int
e2prog (char *name)
{
  char *p = strstr (name, "e2");
  if (!p) return 0;
  p++;

  *p = '4';
  if (access (name, X_OK) == 0)
    return 0;

  *p = '2';
  if (access (name, X_OK) == 0)
    return 0;

  reply_with_error ("cannot find required program %s", name);
  return -1;
}

char **
do_tune2fs_l (const char *device)
{
  int r;
  char *out, *err;
  char *p, *pend, *colon;
  char **ret = NULL;
  int size = 0, alloc = 0;

  char prog[] = "/sbin/tune2fs";
  if (e2prog (prog) == -1)
    return NULL;

  r = command (&out, &err, prog, "-l", device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    free (out);
    return NULL;
  }
  free (err);

  p = out;

  /* Discard the first line if it contains "tune2fs ...". */
  if (STRPREFIX (p, "tune2fs ") || STRPREFIX (p, "tune4fs ")) {
    p = strchr (p, '\n');
    if (p) p++;
    else {
      reply_with_error ("truncated output");
      free (out);
      return NULL;
    }
  }

  /* Read the lines and split into "key: value". */
  while (*p) {
    pend = strchrnul (p, '\n');
    if (*pend == '\n') {
      *pend = '\0';
      pend++;
    }

    if (!*p) continue;

    colon = strchr (p, ':');
    if (colon) {
      *colon = '\0';

      do { colon++; } while (*colon && c_isspace (*colon));

      if (add_string (&ret, &size, &alloc, p) == -1) {
        free (out);
        return NULL;
      }
      if (STREQ (colon, "<none>") ||
          STREQ (colon, "<not available>") ||
          STREQ (colon, "(none)")) {
        if (add_string (&ret, &size, &alloc, "") == -1) {
          free (out);
          return NULL;
        }
      } else {
        if (add_string (&ret, &size, &alloc, colon) == -1) {
          free (out);
          return NULL;
        }
      }
    }
    else {
      if (add_string (&ret, &size, &alloc, p) == -1) {
        free (out);
        return NULL;
      }
      if (add_string (&ret, &size, &alloc, "") == -1) {
        free (out);
        return NULL;
      }
    }

    p = pend;
  }

  free (out);

  if (add_string (&ret, &size, &alloc, NULL) == -1)
    return NULL;

  return ret;
}

int
do_set_e2label (const char *device, const char *label)
{
  int r;
  char *err;

  char prog[] = "/sbin/e2label";
  if (e2prog (prog) == -1)
    return -1;

  r = command (NULL, &err, prog, device, label, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

char *
do_get_e2label (const char *device)
{
  int r, len;
  char *out, *err;

  char prog[] = "/sbin/e2label";
  if (e2prog (prog) == -1)
    return NULL;

  r = command (&out, &err, prog, device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (out);
    free (err);
    return NULL;
  }

  free (err);

  /* Remove any trailing \n from the label. */
  len = strlen (out);
  if (len > 0 && out[len-1] == '\n')
    out[len-1] = '\0';

  return out;			/* caller frees */
}

int
do_set_e2uuid (const char *device, const char *uuid)
{
  int r;
  char *err;

  char prog[] = "/sbin/tune2fs";
  if (e2prog (prog) == -1)
    return -1;

  r = command (NULL, &err, prog, "-U", uuid, device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

char *
do_get_e2uuid (const char *device)
{
  int r;
  char *out, *err, *p, *q;

  /* It's not so straightforward to get the volume UUID.  We have
   * to use tune2fs -l and then look for a particular string in
   * the output.
   */
  char prog[] = "/sbin/tune2fs";
  if (e2prog (prog) == -1)
    return NULL;

  r = command (&out, &err, prog, "-l", device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (out);
    free (err);
    return NULL;
  }

  free (err);

  /* Look for /\nFilesystem UUID:\s+/ in the output. */
  p = strstr (out, "\nFilesystem UUID:");
  if (p == NULL) {
    reply_with_error ("no Filesystem UUID in the output of tune2fs -l");
    free (out);
    return NULL;
  }

  p += 17;
  while (*p && c_isspace (*p))
    p++;
  if (!*p) {
    reply_with_error ("malformed Filesystem UUID in the output of tune2fs -l");
    free (out);
    return NULL;
  }

  /* Now 'p' hopefully points to the start of the UUID. */
  q = p;
  while (*q && (c_isxdigit (*q) || *q == '-'))
    q++;
  if (!*q) {
    reply_with_error ("malformed Filesystem UUID in the output of tune2fs -l");
    free (out);
    return NULL;
  }

  *q = '\0';

  p = strdup (p);
  if (!p) {
    reply_with_perror ("strdup");
    free (out);
    return NULL;
  }

  free (out);
  return p;			/* caller frees */
}

int
do_resize2fs (const char *device)
{
  char *err;
  int r;

  char prog[] = "/sbin/resize2fs";
  if (e2prog (prog) == -1)
    return -1;

  r = command (NULL, &err, prog, device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_e2fsck_f (const char *device)
{
  char *err;
  int r;

  char prog[] = "/sbin/e2fsck";
  if (e2prog (prog) == -1)
    return -1;

  /* 0 = no errors, 1 = errors corrected.
   *
   * >= 4 means uncorrected or other errors.
   *
   * 2, 3 means errors were corrected and we require a reboot.  This is
   * a difficult corner case.
   */
  r = commandr (NULL, &err, prog, "-p", "-f", device, NULL);
  if (r == -1 || r >= 2) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_mke2journal (int blocksize, const char *device)
{
  char *err;
  int r;

  char prog[] = "/sbin/mke2fs";
  if (e2prog (prog) == -1)
    return -1;

  char blocksize_s[32];
  snprintf (blocksize_s, sizeof blocksize_s, "%d", blocksize);

  r = command (NULL, &err,
               prog, "-O", "journal_dev", "-b", blocksize_s,
               device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_mke2journal_L (int blocksize, const char *label, const char *device)
{
  char *err;
  int r;

  char prog[] = "/sbin/mke2fs";
  if (e2prog (prog) == -1)
    return -1;

  char blocksize_s[32];
  snprintf (blocksize_s, sizeof blocksize_s, "%d", blocksize);

  r = command (NULL, &err,
               prog, "-O", "journal_dev", "-b", blocksize_s,
               "-L", label,
               device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_mke2journal_U (int blocksize, const char *uuid, const char *device)
{
  char *err;
  int r;

  char prog[] = "/sbin/mke2fs";
  if (e2prog (prog) == -1)
    return -1;

  char blocksize_s[32];
  snprintf (blocksize_s, sizeof blocksize_s, "%d", blocksize);

  r = command (NULL, &err,
               prog, "-O", "journal_dev", "-b", blocksize_s,
               "-U", uuid,
               device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_mke2fs_J (const char *fstype, int blocksize, const char *device,
             const char *journal)
{
  char *err;
  int r;

  char prog[] = "/sbin/mke2fs";
  if (e2prog (prog) == -1)
    return -1;

  char blocksize_s[32];
  snprintf (blocksize_s, sizeof blocksize_s, "%d", blocksize);

  int len = strlen (journal);
  char jdev[len+32];
  snprintf (jdev, len+32, "device=%s", journal);

  r = command (NULL, &err,
               prog, "-t", fstype, "-J", jdev, "-b", blocksize_s,
               device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_mke2fs_JL (const char *fstype, int blocksize, const char *device,
              const char *label)
{
  char *err;
  int r;

  char prog[] = "/sbin/mke2fs";
  if (e2prog (prog) == -1)
    return -1;

  char blocksize_s[32];
  snprintf (blocksize_s, sizeof blocksize_s, "%d", blocksize);

  int len = strlen (label);
  char jdev[len+32];
  snprintf (jdev, len+32, "device=LABEL=%s", label);

  r = command (NULL, &err,
               prog, "-t", fstype, "-J", jdev, "-b", blocksize_s,
               device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}

int
do_mke2fs_JU (const char *fstype, int blocksize, const char *device,
              const char *uuid)
{
  char *err;
  int r;

  char prog[] = "/sbin/mke2fs";
  if (e2prog (prog) == -1)
    return -1;

  char blocksize_s[32];
  snprintf (blocksize_s, sizeof blocksize_s, "%d", blocksize);

  int len = strlen (uuid);
  char jdev[len+32];
  snprintf (jdev, len+32, "device=UUID=%s", uuid);

  r = command (NULL, &err,
               prog, "-t", fstype, "-J", jdev, "-b", blocksize_s,
               device, NULL);
  if (r == -1) {
    reply_with_error ("%s", err);
    free (err);
    return -1;
  }

  free (err);
  return 0;
}
