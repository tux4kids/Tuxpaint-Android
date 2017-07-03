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
#ifndef TP_ANDROID_ASSETS

#define TP_ANDROID_ASSETS
#include <jni.h>
#include "debug.h"   
#include "dirwalk.h"
#include "progressbar.h"
#include <android/asset_manager.h>

/* AAssetManager_openDir() only lists files, not directories, so we can't relay on 
   directory exploring and have to keep track of what we put on the assets dir */
#define ASSETS_BRUSHES_DIR "data/brushes"
#define ASSETS_STAMPS_DIR "stamps/cartoon/tux"

AAssetDir * open_asset_dir(char * dirname);

void load_brushes_from_assets(SDL_Surface * screen, SDL_Texture *texture, SDL_Renderer *renderer, const char * dirname, void (*fn) (SDL_Surface * screen,
				  SDL_Texture * texture,
				  SDL_Renderer * renderer,
				  const char *restrict const dir,
				  unsigned dirlen, tp_ftw_str * files,
				  unsigned count, const char *restrict const locale) );

void load_from_assets(SDL_Surface * screen, SDL_Texture *texture, SDL_Renderer *renderer, const char * dirname, void (*fn) (SDL_Surface * screen,
				  SDL_Texture * texture,
				  SDL_Renderer * renderer,
				  const char *restrict const dir,
				  unsigned dirlen, tp_ftw_str * files,
				  unsigned count, const char *restrict const locale) );

JNIEXPORT jboolean  Java_org_tuxpaint_tuxpaintActivity_managertojni(JNIEnv * env, jclass clazz, jobject  mgr);


#endif
