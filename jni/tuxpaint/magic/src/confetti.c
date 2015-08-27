#include <time.h>	//For time()

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#define CONFETTI_BRUSH_SIZE 8	//radius of each confetti circle
#define CONFETTI_QUANTITY 3		//how many circles will be created every click?

struct confetti_rgb
{
	Uint8 r, g, b;
};

struct confetti_rgb confetti_colors;		//storage for colors, just for having everything in one place

Mix_Chunk * confetti_snd;

/* Local function prototypes: */
Uint32 confetti_api_version(void);
void confetti_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int confetti_init(magic_api * api);
int confetti_get_tool_count(magic_api * api);
SDL_Surface * confetti_get_icon(magic_api * api, int which);
char * confetti_get_name(magic_api * api, int which);
char * confetti_get_description(magic_api * api, int which, int mode);
int confetti_requires_colors(magic_api * api, int which);
void confetti_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
		      int x, int y, SDL_Rect * update_rect);
void confetti_shutdown(magic_api * api);
inline char confetti_get_greater(const char what1, const char what2);
inline char confetti_get_lesser(const char what1, const char what2);
Uint32 confetti_get_new_color(void * ptr, SDL_Surface * canvas);
void confetti_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
		    int x, int y, SDL_Rect * update_rect);
void confetti_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void confetti_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int confetti_modes(magic_api * api, int which);

//				Housekeeping functions

void confetti_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);

Uint32 confetti_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}

void confetti_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)	//get the colors from API and store it in structure
{
	confetti_colors.r=r;
	confetti_colors.g=g;
	confetti_colors.b=b;
}

int confetti_init(magic_api * api)
{
  char fname[1024];
	
    snprintf(fname, sizeof(fname), "%s/sounds/magic/confetti.ogg", api->data_directory);
    confetti_snd = Mix_LoadWAV(fname);

  return(1);
}

int confetti_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

SDL_Surface * confetti_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/confetti.png",
	     api->data_directory);

  return(IMG_Load(fname));
}

char * confetti_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return strdup(gettext_noop("Confetti")); }

char * confetti_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED) { return strdup(gettext_noop("Click to throw confetti!")); }

int confetti_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return 1; }

void confetti_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  
}

void confetti_shutdown(magic_api * api ATTRIBUTE_UNUSED)
	{ Mix_FreeChunk(confetti_snd); }


//private functions
	
inline char confetti_get_greater(const char what1, const char what2) { if (what1>what2) return what1; else return what2; }

inline char confetti_get_lesser(const char what1, const char what2) { if (what1<what2) return what1; else return what2; }

// Interactivity functions

Uint32 confetti_get_new_color(void * ptr, SDL_Surface * canvas)		//this function creates new color very similar to the one choosen
{
	magic_api * api = (magic_api *) ptr;
	
	float hsv_h, hsv_s, hsv_v;
	Uint8 temp_r, temp_g, temp_b;
	
	api->rgbtohsv(confetti_colors.r, confetti_colors.g, confetti_colors.b, &hsv_h, &hsv_s, &hsv_v);		//color choosen by user is converted
																					//to HSV palette

	hsv_h+=((rand()%60)-30)%360;														//Every circle has different, but
																					//smilar color
	if (hsv_h<0)
		hsv_h *= -1;
	
	api->hsvtorgb(hsv_h, hsv_s, hsv_v, &temp_r, &temp_g, &temp_b);							//...and come back to RGB

	return SDL_MapRGB(canvas->format, temp_r, temp_g, temp_b);
}
	
	
static void confetti_circle(void * ptr, int which ATTRIBUTE_UNUSED,
		    SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
                    int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  
  int xx, yy;
  Uint32 color=confetti_get_new_color(api, canvas);
	
  for (yy = y - CONFETTI_BRUSH_SIZE/2; yy < y + CONFETTI_BRUSH_SIZE/2; yy++)
	
	for (xx = x - CONFETTI_BRUSH_SIZE/2; xx < x + CONFETTI_BRUSH_SIZE/2; xx++)
	  
		if (api->in_circle(xx - x , yy - y , CONFETTI_BRUSH_SIZE/2))
			api->putpixel(canvas, xx, yy, color);
}

void confetti_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
	unsigned char i;
	char min_x = 0, max_x = 0, min_y = 0, max_y = 0;
	char dx = 0, dy = 0;
	
	for (i=0; i<CONFETTI_QUANTITY; i++)
	{
		srand((dx+dy)/2 + time(0));			//to get a unique seed even if dx and dy aren't defined
		dx=(rand()%100)-50;						//generate a value between <-50; +50>
		dy=(rand()%100)-50;						//to spread confetti around the cursor position
		
		if (!i)
		{
			min_x=max_x=dx;
			min_y=max_y=dy;
		}
		else
		{
			min_x=confetti_get_lesser(min_x, dx);		//any candidates to new min/max values? Hands up please...
			max_x=confetti_get_greater(max_x, dx);
			min_y=confetti_get_lesser(min_y, dy);
			max_y=confetti_get_greater(max_y, dy);
		}
		confetti_circle((void *)api, which, canvas, last, x+dx, y+dy);
	}

	update_rect->x = x+min_x - CONFETTI_BRUSH_SIZE/2;
	update_rect->y = y+ min_y  - CONFETTI_BRUSH_SIZE/2;
	update_rect->w = CONFETTI_BRUSH_SIZE*1.5+max_x-min_x;
	update_rect->h = CONFETTI_BRUSH_SIZE*1.5+max_y-min_y;
	
	api->playsound(confetti_snd, (x * 255) / canvas->w,255);
}

void confetti_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
	int temp;
	
	if (ox>x) {temp=x; x=ox; ox=temp;}
	if (oy>y) {temp=y; y=oy; oy=temp; }
	
	confetti_click(api, which, MODE_PAINT, canvas, snapshot, x, y, update_rect);
}

void confetti_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void confetti_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int confetti_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
