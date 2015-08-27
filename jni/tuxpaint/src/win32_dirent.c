/****************************************************/  
/*                                                  */ 
/* For Win32 that lacks Unix direct support.        */ 
/*    - avoids including "windows.h"                */ 
/*                                                  */ 
/* Copyright (c) 2002 John Popplewell               */ 
/* john@johnnypops.demon.co.uk                      */ 
/*                                                  */ 
/* Version 1.0.1 - fixed bug in opendir()           */ 
/* Version 1.0.0 - initial version                  */ 
/*                                                  */ 
/****************************************************/ 
  
/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/ 
  
/* $Id: win32_dirent.c,v 1.5 2006/08/27 21:00:55 wkendrick Exp $ */ 
  
#include <stdlib.h>
#include <string.h>
#include <assert.h>
  
#include "win32_dirent.h"
#include "debug.h"
  DIR * opendir(const char *pSpec) 
{
  char pathname[MAX_PATH + 2];
  DIR * pDir = calloc(1, sizeof(DIR));
  if (!pDir)
    return NULL;
  strcpy(pathname, pSpec);
  strcat(pathname, "/*");
  pDir->hFind = FindFirstFile(pathname, &pDir->wfd);
  if (pDir->hFind == INVALID_HANDLE_VALUE)
    
  {
    free(pDir);
    pDir = NULL;
  }
  return pDir;
}
void closedir(DIR * pDir) 
{
  assert(pDir != NULL);
  free(pDir);
} struct dirent *readdir(struct DIR *pDir) 
{
  assert(pDir != NULL);
  if (pDir->hFind)
    
  {
    strcpy(pDir->de.d_name, (const char *) pDir->wfd.cFileName);
    if (!FindNextFile(pDir->hFind, &pDir->wfd))
      
    {
      FindClose(pDir->hFind);
      pDir->hFind = NULL;
    }
    return &pDir->de;
  }
  return NULL;
}
int alphasort(const void *a, const void *b) 
{
  return (strcmp
	   ((*(const struct dirent **) a)->d_name,
	    (*(const struct dirent **) b)->d_name));
} static int addToList(int i, struct dirent ***namelist,
			   struct dirent *entry) 
{
  int size;
  struct dirent *block;
  *namelist =
    (struct dirent **) realloc((void *) (*namelist),
			       (size_t) ((i + 1) * sizeof(struct dirent *)));
  if (*namelist == NULL)
    return -1;
  size =
    (((char *) &entry->d_name) - ((char *) entry)) + strlen(entry->d_name) +
    1;
  block = (struct dirent *) malloc(size);
  if (block == NULL)
    return -1;
  (*namelist)[i] = block;
  memcpy(block, entry, size);
  return ++i;
}
int scandir(const char *dir, struct dirent ***namelist, selectCB select,
	       comparCB compar) 
{
  DIR * pDir;
  int count;
  struct dirent *entry;
  assert((dir != NULL) && (namelist != NULL));
  pDir = opendir(dir);
  if (!pDir)
    return -1;
  count = 0;
  while ((entry = readdir(pDir)) != NULL)
    
  {
    if (select == NULL || (select != NULL && select(entry)))
      if ((count = addToList(count, namelist, entry)) < 0)
	break;
  }
  closedir(pDir);
  if (count <= 0)
    return -1;
  if (compar != NULL)
    qsort((void *) (*namelist), (size_t) count, sizeof(struct dirent *),
	   compar);
  return count;
}


