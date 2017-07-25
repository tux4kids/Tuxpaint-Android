//optimized version soon :)
//when "folding" same corner many times it gives strange results. Now it's allowed. Let me know
//if you think it shouldn't be.

//sound playing needs fixing.

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#define FOLD_LEN 80

int right_arm_x, right_arm_y, left_arm_x, left_arm_y;
int fold_ox, fold_oy;
int fold_x, fold_y;
Uint8 fold_shadow_value;
Uint8 corner;
Mix_Chunk * fold_snd;
Uint8 fold_r, fold_g, fold_b;
Uint32 fold_color;
SDL_Surface * fold_surface_src, * fold_surface_dst;


void fold_draw(magic_api * api, int which,
	       SDL_Surface * canvas, SDL_Surface * snapshot,
	       int x, int y, SDL_Rect * update_rect);
static void fold_erase(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
	       int x, int y);
void translate_coords(SDL_Surface * canvas,int angle);
SDL_Surface * rotate(magic_api * api, SDL_Surface * canvas, int angle);
void fold_draw(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	       int x, int y, SDL_Rect * update_rect);
static void fold_print_line(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
			    int x, int y);
static void fold_print_dark_line(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
				  int x, int y);
void translate_xy(SDL_Surface * canvas, int x, int y, int * a, int * b, int rotation);
Uint32 fold_api_version(void);
void fold_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int fold_init(magic_api * api);
int fold_get_tool_count(magic_api * api);
SDL_Surface * fold_get_icon(magic_api * api, int which);
char * fold_get_name(magic_api * api, int which);
char * fold_get_description(magic_api * api, int which, int mode);
int fold_requires_colors(magic_api * api, int which);
void fold_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);
void fold_shutdown(magic_api * api);
void fold_click(magic_api * ptr, int which, int mode,
		SDL_Surface * canvas, SDL_Surface * snapshot,
		int x, int y, SDL_Rect * update_rect);
void fold_preview(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
int fold_modes(magic_api * api, int which);
//				Housekeeping functions

void fold_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void fold_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
static Uint8 fold_what_corner(int x, int y, SDL_Surface * canvas);
void fold_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);

Uint32 fold_api_version(void)

{
  return(TP_MAGIC_API_VERSION);
}

void fold_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)	//get the colors from API and store it in structure
{
	fold_r=r;
	fold_g=g;
	fold_b=b;
}

int fold_init(magic_api * api)
{
  char fname[1024];
	
    snprintf(fname, sizeof(fname), "%ssounds/magic/fold.wav", api->data_directory);
    fold_snd = Mix_LoadWAV(fname);
	
  return(1);
}

int fold_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

SDL_Surface * fold_get_icon(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/fold.png",
	     api->data_directory);

  return(IMG_Load(fname));
}

char * fold_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return(gettext_noop("Fold")); }

char * fold_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED) { return strdup(gettext_noop("Choose a background color and click to turn the corner of the page over.")); }

int fold_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return 1; }		//selected color will be a "backpage" color


static void fold_shadow(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * temp,
		       int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  Uint8 r,g,b,a;
	  SDL_GetRGBA(api->getpixel(temp, x, y),
		      temp->format, &r, &g, &b, &a);
	  api->putpixel(canvas, x, y, SDL_MapRGBA(canvas->format,
						  max(r-160+fold_shadow_value*4,0), max(g-160+fold_shadow_value*4,0), max(b-160+fold_shadow_value*4,0), a));
}

void fold_draw(magic_api * api, int which,
	       SDL_Surface * canvas, SDL_Surface * snapshot,
	       int x, int y, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  float right_step_x, right_step_y, left_step_x, left_step_y;
  float dist_x, dist_y;
  int left_y, right_x;
  float w, h;
  SDL_Surface * temp;
  temp=SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h, canvas->format->BitsPerPixel,
			    canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);
  SDL_BlitSurface(canvas,0,temp,0);

  right_step_x=(float)(x-left_arm_x)/(float)(left_arm_x-fold_ox);
  right_step_y=(float)(y-left_arm_y)/(float)(left_arm_x-fold_ox);
  left_step_x=(float)(x-right_arm_x)/(float)(right_arm_y-fold_oy);
  left_step_y=(float)(y-right_arm_y)/(float)(right_arm_y-fold_oy);
  for (w=0;w < canvas->w;w+=0.5)
    for(h=0;h < canvas->h;h+=0.5)
      {
	dist_x=right_step_x*w+left_step_x*h;
	dist_y=right_step_y*w+left_step_y*h;
	api->putpixel(canvas, x-dist_x, y-dist_y, api->getpixel(temp,w,h));
      }

  // Erasing the triangle.
  // The 1 pixel in plus  is a workaround for api-line not getting the end in some lines.
  if (left_arm_x > canvas->w)
    {
      left_y=(float)right_arm_y/left_arm_x*(left_arm_x-canvas->w);
      for (h = 0; h <= right_arm_y; h++)
	api->line((void *)api, which, canvas, snapshot, canvas->w, left_y-h, -1, right_arm_y-h, 1, fold_erase);
    }
  else if (right_arm_y > canvas->h)
    {
      right_x=(float)left_arm_x/right_arm_y*(right_arm_y-canvas->h);
      for (w = 0; w <= left_arm_x; w++)
	api->line((void *)api, which, canvas, snapshot, left_arm_x-w, 0, right_x-w, canvas->h +1, 1, fold_erase);
    }
  else
    for (w = 0; w <= min(left_arm_x,right_arm_y); w++)  // The -1 values are because api->line 
      api->line((void *)api, which, canvas, snapshot, left_arm_x-w, 0, -1, right_arm_y-w, 1, fold_erase);

  SDL_BlitSurface(canvas,0,temp,0);

  // Shadows
  if (left_arm_x > canvas->w)
    {
      for (fold_shadow_value = 0; fold_shadow_value < 40; fold_shadow_value+=1)
	api->line((void *)api, which, canvas, temp, canvas->w, left_y-fold_shadow_value, 0, right_arm_y-fold_shadow_value, 1, fold_shadow);

    }
  else if (right_arm_y > canvas->h)
    {
      for (fold_shadow_value = 0; fold_shadow_value < 40; fold_shadow_value+=1)
	api->line((void *)api, which, canvas, temp, left_arm_x-fold_shadow_value, 0, right_x - fold_shadow_value, canvas->h, 1, fold_shadow);

    }

  else
    for (fold_shadow_value = 0; fold_shadow_value < 40; fold_shadow_value+=1)
      api->line((void *)api, which, canvas, temp, left_arm_x-fold_shadow_value, 0, 0, right_arm_y-fold_shadow_value, 1, fold_shadow);

  SDL_BlitSurface(canvas,0,temp,0);

  for (fold_shadow_value = 0; fold_shadow_value < 40; fold_shadow_value+=1)
    {
      if (fold_shadow_value*left_step_x > x || fold_shadow_value*right_step_y > y) break;

      dist_x=fold_shadow_value*(right_step_x+left_step_x);
      dist_y=fold_shadow_value*(right_step_y+left_step_y);
      api->line((void *)api, which, canvas, temp, left_arm_x+fold_shadow_value*right_step_x, fold_shadow_value*right_step_y, fold_shadow_value*left_step_x, right_arm_y+fold_shadow_value*left_step_y, 1, fold_shadow);
    }

  api->line((void *)api, which, canvas, snapshot, x, y, right_arm_x, right_arm_y, 1, fold_print_line);
  api->line((void *)api, which, canvas, snapshot, x, y, left_arm_x, left_arm_y, 1, fold_print_line);
  api->line((void *)api, which, canvas, snapshot, left_arm_x, left_arm_y, right_arm_x, right_arm_y, 1, fold_print_dark_line);

}

SDL_Surface * rotate(magic_api * api, SDL_Surface * canvas, int angle)
{
  SDL_Surface * temp;
  int x,y;
  int a,b;

  if (angle==180)
    temp=SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h, canvas->format->BitsPerPixel,
			      canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);
  else
    temp=SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->h, canvas->w, canvas->format->BitsPerPixel,
			      canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  switch (angle)
    {
    case 90:
	for (x=0; x<canvas->w; x++)
	  for (y=0; y<canvas->h; y++)
	    {
	      translate_xy(canvas,x,y,&a,&b,90);
	      api->putpixel(temp,a,b,api->getpixel(canvas, x, y));
	    }
	break;
	
    case 180:
      //      printf("%i, %i\n",temp,canvas);
      for (x=0; x<canvas->w; x++)
	for (y=0; y<canvas->h; y++)
	  {
	    translate_xy(canvas,x,y,&a,&b,180);
	    api->putpixel(temp,a,b,api->getpixel(canvas, x, y));
	  }
      break;
      
    case 270:
      for (x=0; x<canvas->w; x++)
	for (y=0; y<canvas->h; y++)
	  {
	    translate_xy(canvas,x,y,&a,&b,270);
	    api->putpixel(temp,a,b,api->getpixel(canvas, x, y));
	  }
      break;
    }
  return temp;
}

  void translate_coords(SDL_Surface * canvas,int angle)
{
  int a,b;
  switch (angle)
    {
      case 90:
	translate_xy(canvas,right_arm_x,right_arm_y,&a,&b,90);
	right_arm_x=a;
	right_arm_y=b;
	translate_xy(canvas,left_arm_x,left_arm_y,&a,&b,90);
	left_arm_x=a;
	left_arm_y=b;

	break;
      case 180:
	right_arm_x=canvas->w-1-right_arm_x;
	right_arm_y=canvas->h-1-right_arm_y;
	left_arm_x=canvas->w-1-left_arm_x;
	left_arm_y=canvas->h-1-left_arm_y;
	break;
      case 270:
	translate_xy(canvas,right_arm_x,right_arm_y,&a,&b,270);
	right_arm_x=a;
	right_arm_y=b;
	translate_xy(canvas,left_arm_x, left_arm_y, &a, &b, 270);
	left_arm_x=a;
	left_arm_y=b;
	break;
      }
}

void translate_xy(SDL_Surface * canvas, int x, int y, int * a, int * b, int rotation)
{
  switch (rotation)
    {
    case 90:
      *a=y;
      *b=canvas->w -1 -x;
      break;
    case 180:
      *a=canvas->w -1 -x;
      *b=canvas->h -1 -y;
      break;
    case 270:
      *a=canvas->h -1 -y;
      *b=x;
      break; 
    }
}

void fold_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect)
{
  int a,b;
  SDL_Surface * temp, *temp2;

  x=fold_x;
  y=fold_y;
  fold_ox=fold_oy=0;
  SDL_BlitSurface(snapshot, 0, canvas, 0);
  switch (corner)
    {
    case 1:
      translate_xy(canvas,x,y,&a,&b,90);
      translate_coords(canvas,90);
      temp=rotate(api, canvas, 90);
      fold_draw (api, which, temp, snapshot, a, b,update_rect);
      temp2=rotate(api,temp,270);
      SDL_BlitSurface(temp2,0,canvas,0);
      SDL_FreeSurface(temp);
      SDL_FreeSurface(temp2);
      break;

    case 2:
      fold_draw (api, which, canvas, snapshot, x,y,update_rect);
      break;
      
    case 3:
	translate_xy(canvas,x,y,&a,&b,270);
	translate_coords(canvas,270);
	temp=rotate(api, canvas, 270);
	fold_draw (api, which, temp, snapshot, a, b,update_rect);
	temp2=rotate(api,temp,90);
	SDL_BlitSurface(temp2,0,canvas,0);
	SDL_FreeSurface(temp);
	SDL_FreeSurface(temp2);
	break;

    case 4:
      	translate_xy(canvas,x,y,&a,&b,180);
	translate_coords(canvas,180);
	temp=rotate(api, canvas, 180);
	fold_draw (api, which, temp, snapshot, a, b,update_rect);
	temp2=rotate(api,temp,180);
	SDL_BlitSurface(temp2,0,canvas,0);
	SDL_FreeSurface (temp);
	SDL_FreeSurface (temp2);
	break;
    }

  update_rect->x=update_rect->y=0;
  update_rect->w=canvas->w;
  update_rect->h=canvas->h;
  api->playsound(fold_snd, (x * 255) / canvas->w, 255);
}

void fold_shutdown(magic_api * api ATTRIBUTE_UNUSED) 
	{ 
		Mix_FreeChunk(fold_snd);
		SDL_FreeSurface(fold_surface_dst);
		SDL_FreeSurface(fold_surface_src);
	}

// Interactivity functions

static Uint8 fold_what_corner(int x, int y, SDL_Surface * canvas)
{
	if (x>=canvas->w/2)
	{
		if (y>=canvas->h/2) return 4;
		else return 1;
	}
	else 
	{
		if (y>=canvas->h/2) return 3;
		else return 2;
	}
}


static void fold_print_line(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y)
{
	magic_api * api = (magic_api *) ptr;
	api->putpixel(canvas, x, y, SDL_MapRGB(last->format, 222, 222, 222));		//Middle gray. Color have been set arbitrary. 
}
static void fold_print_dark_line(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
                int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  api->putpixel(canvas, x, y, SDL_MapRGB(last->format, 90, 90, 90));	//It should not look too black nor too white with shadowed colors. 
}

static void fold_erase(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
                int x, int y)
{
	magic_api * api = (magic_api *) ptr;
	api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, fold_r, fold_g, fold_b));
}

void fold_click(magic_api * ptr, int which, int mode ATTRIBUTE_UNUSED,
		SDL_Surface * canvas, SDL_Surface * snapshot,
		int x, int y, SDL_Rect * update_rect)
{
  magic_api * api = (magic_api *) ptr;
  corner=fold_what_corner(x, y, snapshot);
	
  switch (corner)
    {
    case 1:
      fold_ox=canvas->w-1;
      fold_oy=0;
      break;
    case 2:
      fold_ox=fold_oy=0;
      break;
    case 3:
      fold_ox=0;
      fold_oy=canvas->h-1;
      break;
    case 4:
      fold_ox=canvas->w-1;
      fold_oy=canvas->h-1;
      break;
    }

  fold_drag(api, which, canvas, snapshot, x, y, x, y, update_rect); 
}

void fold_preview(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y,
		  SDL_Rect * update_rect)
{
  int middle_point_x;
  int middle_point_y;

  fold_x=x;
  fold_y=y;
  SDL_BlitSurface(snapshot,0,canvas,0);

  middle_point_x=(fold_ox+x)/2;
  middle_point_y=(fold_oy+y)/2;

  switch(corner)
    {
    case 1:					//Right Upper			
      right_arm_x=fold_ox- (fold_ox-middle_point_x)-middle_point_y*middle_point_y/(fold_ox-middle_point_x);
      right_arm_y=fold_oy;

      left_arm_x=fold_ox;
      left_arm_y=fold_oy-(fold_oy-middle_point_y)-(fold_ox-middle_point_x)*(fold_ox-middle_point_x)/(fold_oy-middle_point_y);
      break;

    case 2:					//LU
      right_arm_x=fold_ox;
      right_arm_y=middle_point_y+middle_point_x*middle_point_x/middle_point_y;

      left_arm_x=middle_point_x+middle_point_y*middle_point_y/middle_point_x;  
      left_arm_y=fold_oy;
      break;
		
    case 3:					//LL
      right_arm_x=middle_point_x+(fold_oy-middle_point_y)*(fold_oy-middle_point_y)/middle_point_x;
      right_arm_y=fold_oy;

      left_arm_x=fold_ox;
      left_arm_y=fold_oy-(fold_oy-middle_point_y)-(fold_ox-middle_point_x)*(fold_ox-middle_point_x)/(fold_oy-middle_point_y);
      break;

    case 4:					//RL
      right_arm_x=fold_ox;
      right_arm_y=fold_oy-(fold_oy-middle_point_y)-(fold_ox-middle_point_x)*(fold_ox-middle_point_x)/(fold_oy-middle_point_y);

      left_arm_x=fold_ox-(fold_ox-middle_point_x)-(fold_oy-middle_point_y)*(fold_oy-middle_point_y)/(fold_ox-middle_point_x);
      left_arm_y=fold_oy;
      break;
    }

  api->line((void *)api, which, canvas, snapshot, x, y, right_arm_x, right_arm_y, 1, fold_print_line);
  api->line((void *)api, which, canvas, snapshot, x, y, left_arm_x, left_arm_y, 1, fold_print_line);
  api->line((void *)api, which, canvas, snapshot, left_arm_x, left_arm_y, right_arm_x, right_arm_y, 1, fold_print_line);

  update_rect->x=update_rect->y=0;
  update_rect->w=canvas->w;
  update_rect->h=canvas->h;
}

void fold_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  // Avoid division by zero when calculating the preview
  x=clamp(2,x,canvas->w-2);
  y=clamp(2,y,canvas->h-2);
  fold_preview(api, which, canvas, snapshot, ox, oy, x, y, update_rect);
}

void fold_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void fold_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int fold_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT_WITH_PREVIEW);
}
