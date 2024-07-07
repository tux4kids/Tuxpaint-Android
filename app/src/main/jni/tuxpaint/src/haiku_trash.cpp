/*
 * haiku_trash.cpp
 *
 * Place a file in the Trash under Haiku.
 *
 * From Haiku `trash.cpp`
 * (https://github.com/haiku/haiku/blob/3e910e720f3408a2ccf7dae9019621a390f3527b/src/bin/trash.cpp)
 *
 * Copyright (c) 2004 by Francois Revol <revol@free.fr>
 * provided under the MIT licence:
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation files
 *   (the “Software”), to deal in the Software without restriction,
 *   including without limitation the rights to use, copy, modify, merge,
 *   publish, distribute, sublicense, and/or sell copies of the Software,
 *   and to permit persons to whom the Software is furnished to do so,
 *   subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF
 *   ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 *   AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *   IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 *
 * h/t Jérôme Duval <https://github.com/korli> for pointing us to this
 * (https://github.com/haikuports/haikuports/issues/10568)
 *
 * Last modified 2024-06-06
*/

/* FIXME: Not sure how many of these are strictly necessary for
   _just_ the `haiku_trash()` function.  Luc, please check!
   -bjk 2024-06-06 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <app/Message.h>
#include <app/Messenger.h>
#include <kernel/fs_attr.h>
#include <kernel/fs_info.h>
#include <storage/Directory.h>
#include <storage/Entry.h>
#include <storage/FindDirectory.h>
#include <storage/Node.h>
#include <storage/Path.h>
#include <support/TypeConstants.h>

static const char *kAttrOriginalPath = "_trk/original_path";

extern "C" status_t haiku_trash(const char *f)
{
  status_t err;
  attr_info ai;
  dev_t dev = -1;
  int nr;
  const char *original_path;
  char trash_dir[B_PATH_NAME_LENGTH];
  char trashed_file[B_PATH_NAME_LENGTH];

  dev = dev_for_path(f);
  err = find_directory(B_TRASH_DIRECTORY, dev, false, trash_dir, B_PATH_NAME_LENGTH);
  if (err < 0)
    return err;
  BNode node(f);

  err = node.InitCheck();
  if (err < 0)
    return err;
  err = node.GetAttrInfo(kAttrOriginalPath, &ai);
  if (err == B_OK)
    return EALREADY;
  if (!strncmp(f, trash_dir, strlen(trash_dir)))
    return EALREADY;
  entry_ref er;

  err = get_ref_for_path(f, &er);
  if (err < 0)
    return err;
  BPath orgPath(&er);

  err = orgPath.InitCheck();
  if (err < 0)
    return err;
  original_path = orgPath.Path();
  BDirectory trashDir(trash_dir);

  err = trashDir.InitCheck();
  if (err < 0)
    return err;
  for (nr = 0;; nr++)
  {
    if (nr > INT_MAX - 1)
      return B_ERROR;
    if (nr)
      snprintf(trashed_file, B_PATH_NAME_LENGTH - 1, "%s/%s %d", trash_dir, er.name, nr);
    else
      snprintf(trashed_file, B_PATH_NAME_LENGTH - 1, "%s/%s", trash_dir, er.name);
    if (!trashDir.Contains(trashed_file))
      break;
  }
  err = rename(original_path, trashed_file);
  if (err < 0)
    return err;

  err = node.WriteAttr(kAttrOriginalPath, B_STRING_TYPE, 0LL, original_path, strlen(original_path) + 1);
  if (err < 0)
    return err;
  return 0;
}
