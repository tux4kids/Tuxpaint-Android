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
  (See COPYING.txt)
*/

#include "android_assets.h"

AAssetManager * asset_manager = NULL;

AAssetDir * open_asset_dir(char * dirname)
{
  return(AAssetManager_openDir(asset_manager, dirname));

}
void load_assets_dir(char * dirname, tp_ftw_str ** ffilenames, unsigned  * num_file_names)
{
  AAssetDir* assetDir = AAssetManager_openDir(asset_manager, dirname);
  const char* filename = (const char*)NULL;
  tp_ftw_str * filenames = NULL;
  
  unsigned max_file_names = 0;
  int fulllen = 0;
  *num_file_names = 0;


  while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL)
  {
    char *cp;
    
    fulllen = strlen(filename) + 1;

    if (*num_file_names == max_file_names)
    {
      max_file_names = max_file_names * 5 / 4 + 30;
      filenames = realloc(filenames, max_file_names * sizeof *filenames);
    }
    cp = malloc(fulllen + 1);
    memcpy(cp, filename, fulllen);
    filenames[*num_file_names].str = cp;
    filenames[*num_file_names].len = fulllen;
    *num_file_names = *num_file_names + 1;
  }

  AAssetDir_close(assetDir);

  *ffilenames = filenames;
}

JNIEXPORT jboolean Java_org_tuxpaint_tuxpaintActivity_managertojni(JNIEnv * env, jclass clazz, jobject  mgr)
{
  asset_manager = AAssetManager_fromJava(env, mgr);

  if (asset_manager == NULL)
    return 0;
  else
    return 1;
}



void load_brushes_from_assets(SDL_Surface * screen, SDL_Texture *texture, SDL_Renderer *renderer, const char * dirname, void (*fn) (SDL_Surface * screen,
				  SDL_Texture * texture,
				  SDL_Renderer * renderer,
				  const char *restrict const dir,
				  unsigned dirlen, tp_ftw_str * files,
				  unsigned count, const char *restrict const locale))
{
  unsigned num_file_names = 0;
  char * dir = "data/brushes";
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dirname);

  memcpy(buf, dir, dirlen);

  tp_ftw_str * filenames = NULL;

  load_assets_dir(dirname, &filenames, &num_file_names);
  fn(screen, texture, renderer, dir, dirlen, filenames, num_file_names, NULL);
}




void load_from_assets(SDL_Surface * screen, SDL_Texture *texture, SDL_Renderer *renderer, const char * dirname, void (*fn) (SDL_Surface * screen,
				  SDL_Texture * texture,
				  SDL_Renderer * renderer,
				  const char *restrict const dir,
				  unsigned dirlen, tp_ftw_str * files,
				  unsigned count, const char *restrict const locale))
{
  unsigned num_file_names = 0;
  // char * dir = "data/stamps/cartoon/tux";
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dirname);

  memcpy(buf, dirname, dirlen);

  tp_ftw_str * filenames = NULL;

  load_assets_dir(dirname, &filenames, &num_file_names);
  fn(screen, texture, renderer, dirname, dirlen, filenames, num_file_names, NULL);
}
