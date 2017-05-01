#include <SDL.h>
#include "tp_android_assets_fopen.h"


static int rw_read(void * ctx, char * buffer, int size)
{
  return SDL_RWread((SDL_RWops *) ctx, buffer, 1, size);
}

static int rw_write(void * ctx, const char * buffer, int size)
{
  return SDL_RWwrite((SDL_RWops *)ctx, buffer, 1, size);
}

static fpos_t rw_seek(void * ctx ,fpos_t off, int whence)
{
  return (fpos_t)SDL_RWseek((SDL_RWops *)ctx, off, whence);
}
static int rw_close(void * ctx)
{
  return SDL_RWclose((SDL_RWops *) ctx);
}


FILE* android_fopen(char const* fname, char const* mode)
{
  int i;
  int previous;
  int current;
  int pos;
  char * path;
  
  /* That call to the real fopen will not be preprocessed, needed to still be able to access to the filesystem */
  FILE* fi= (fopen) (fname, mode);
  if (fi != NULL)
    return fi;

  /* If the real fopen() didn't found the file then fallback to inside assets */
  /* Code inspired from http://www.50ply.com/blog/2013/01/19/loading-compressed-android-assets-with-file-pointer from Brian Taylor*/
  /* Rewrited to use SDL RW functions/fallbacks that don't need to pass them an asset manager */

  /* Android assets aren't writable */
  if(mode[0] == 'w') return NULL;

  /* Don't fallback full paths, they don't fit in android assets */
  if (strcmp(&fname[0] , "/") == 0)
    return NULL;
  
  /* Remove double slashes */
  path = strdup(fname);
  pos = 0;
  for (i = 0; i < strlen(fname); i++)
  {
    previous = current;
    if (fname[i] == '/')
      current = 1;
    else
      current = 0;
    if (!(previous && current))
    {
      path[pos] = fname[i];
      pos++;
    }
    path[pos] = 0;
  }

  SDL_RWops * rwopss = SDL_RWFromFile(path, mode);
  free(path);
  
  if (rwopss == NULL)
    return NULL;
  
  return funopen(rwopss, rw_read, rw_write, rw_seek, rw_close);
}

 
