#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <stdbool.h>

#define SEG_NONE 0

#define SEG_LEFT 1
#define SEG_RIGHT 2
#define SEG_TOP 4
#define SEG_BOTTOM 8

#define SEG_LEFT_RIGHT (SEG_LEFT | SEG_RIGHT)
#define SEG_TOP_BOTTOM (SEG_TOP | SEG_BOTTOM)
#define SEG_RIGHT_TOP (SEG_RIGHT | SEG_TOP)
#define SEG_RIGHT_BOTTOM (SEG_RIGHT | SEG_BOTTOM)
#define SEG_LEFT_TOP (SEG_LEFT | SEG_TOP)
#define SEG_LEFT_BOTTOM (SEG_LEFT | SEG_BOTTOM)
#define SEG_LEFT_RIGHT_TOP (SEG_LEFT | SEG_RIGHT | SEG_TOP)
#define SEG_LEFT_RIGHT_BOTTOM (SEG_LEFT | SEG_RIGHT | SEG_BOTTOM)
#define SEG_LEFT_TOP_BOTTOM (SEG_LEFT | SEG_TOP | SEG_BOTTOM)
#define SEG_RIGHT_TOP_BOTTOM (SEG_RIGHT | SEG_TOP | SEG_BOTTOM)
#define SEG_LEFT_RIGHT_TOP_BOTTOM (SEG_LEFT | SEG_RIGHT | SEG_TOP | SEG_BOTTOM)

Mix_Chunk * fretwork_snd;
unsigned int img_w, img_h;
unsigned int fretwork_segments_x, fretwork_segments_y;	//how many segments do we have?
static int fretwork_math_ceil(int x, int y);	//ceil() in cstdlib returns float and is relative slow, so we'll use our one
static Uint8 * fretwork_status_of_segments;	//a place to store an info about bitmap used for selected segment
static char ** fretwork_images;	//the pathes to all the images needed
static unsigned int fretwork_segment_modified;		//which segment was modified this time?
static unsigned int fretwork_segment_modified_last =0;     //which segment was last modified
static unsigned int fretwork_segment_to_add =0;            //a segment that should be added to solve corner joint
static unsigned int fretwork_segment_last_clicked;
static Uint8 fretwork_r, fretwork_g, fretwork_b;   
static unsigned int fretwork_full_runs;            //The count of the clicks in full mode
static unsigned int fretwork_segment_start_rectangle;       //the segment were the update_rectangle will start
static unsigned int fretwork_update_rectangle_width;  //the width of the update_rectangle
static unsigned int fretwork_update_rectangle_height; //the height of the update_rectangle
static SDL_Rect modification_rect;
static SDL_Surface * canvas_backup;
static SDL_Surface * fretwork_one_back, * fretwork_three_back, * fretwork_four_back, * fretwork_corner_back;


//				Housekeeping functions

Uint32 fretwork_api_version(void);
int fretwork_modes(magic_api * api, int which);
void fretwork_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
static void fretwork_colorize(magic_api * api, SDL_Surface * dest, SDL_Surface * src );
int fretwork_init(magic_api * api);
int fretwork_get_tool_count(magic_api * api);
SDL_Surface * fretwork_get_icon(magic_api * api, int which);
char * fretwork_get_name(magic_api * api, int which);
char * fretwork_get_description(magic_api * api, int which, int mode);
int fretwork_requires_colors(magic_api * api, int which);
void fretwork_release(magic_api * api, int which,
		      SDL_Surface * canvas, SDL_Surface * snapshot,
		      int x, int y, SDL_Rect * update_rect);
void fretwork_shutdown(magic_api * api);
void fretwork_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * snapshot);
void fretwork_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * snapshot);
static void fretwork_extract_coords_from_segment(unsigned int segment, Sint16 * x, Sint16 * y);
void fretwork_click(magic_api * api, int which, int mode,
		    SDL_Surface * canvas, SDL_Surface * snapshot,
		    int x, int y, SDL_Rect * update_rect);



void fretwork_drag(magic_api * api, int which, SDL_Surface * canvas,
		   SDL_Surface * snapshot, int ox, int oy, int x, int y,
		   SDL_Rect * update_rect);
static void fretwork_draw_wrapper(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
				  int x, int y);
static unsigned int fretwork_get_segment(int x, int y);


SDL_Surface * fretwork_one, * fretwork_three, * fretwork_four, * fretwork_corner;

Uint32 fretwork_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}

int fretwork_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT|MODE_FULLSCREEN);
}

void fretwork_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b)
{
  fretwork_r=r;
  fretwork_g=g;
  fretwork_b=b;
  fretwork_colorize(api,fretwork_one, fretwork_one_back);
  fretwork_colorize(api, fretwork_three, fretwork_three_back);
  fretwork_colorize(api, fretwork_four, fretwork_four_back);
  fretwork_colorize(api, fretwork_corner, fretwork_corner_back);
}

/* Adapted from flower.c */
static void fretwork_colorize(magic_api * api, SDL_Surface * dest, SDL_Surface * src )
{
  int x, y;
  Uint8 r, g, b, a;

  SDL_LockSurface(src);
  SDL_LockSurface(dest);

  for (y = 0; y < src->h; y++)
    {
      for (x = 0; x < src->w; x++)
	{
	  SDL_GetRGBA(api->getpixel(src, x, y),
		      src->format, &r, &g, &b, &a);

	  api->putpixel(dest, x, y,
			SDL_MapRGBA(dest->format,
				    fretwork_r, fretwork_g, fretwork_b, a));
	}
    }

  SDL_UnlockSurface(src);
  SDL_UnlockSurface(dest);
}


int fretwork_init(magic_api * api)
{
  char fname[1024];
  Uint8 i;		//is always < 4, so Uint8 seems to be a good idea
	
  fretwork_images=(char **)malloc(sizeof(char *)*4);
	
  for (i = 0; i < 4; i++)
    fretwork_images[i]=(char *)malloc(sizeof(char)*1024);
	
  snprintf(fretwork_images[0], 1024*sizeof(char), "%simages/magic/fretwork_one.png", api->data_directory);
  snprintf(fretwork_images[1], 1024*sizeof(char), "%simages/magic/fretwork_three.png", api->data_directory);
  snprintf(fretwork_images[2], 1024*sizeof(char), "%simages/magic/fretwork_four.png", api->data_directory);
  snprintf(fretwork_images[3], 1024*sizeof(char), "%simages/magic/fretwork_corner.png", api->data_directory);

  fretwork_one=IMG_Load(fretwork_images[0]);
  fretwork_three=IMG_Load(fretwork_images[1]);
  fretwork_four=IMG_Load(fretwork_images[2]);
  fretwork_corner=IMG_Load(fretwork_images[3]);
  fretwork_one_back=IMG_Load(fretwork_images[0]);
  fretwork_three_back=IMG_Load(fretwork_images[1]);
  fretwork_four_back=IMG_Load(fretwork_images[2]);
  fretwork_corner_back=IMG_Load(fretwork_images[3]);

  img_w = fretwork_one->w;
  img_h = fretwork_one->h;
    
  snprintf(fname, sizeof(fname), "%ssounds/magic/fretwork.ogg", api->data_directory);
  fretwork_snd = Mix_LoadWAV(fname);

  return(1);
}

int fretwork_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

SDL_Surface * fretwork_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/fretwork.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

char * fretwork_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return strdup(gettext_noop("Fretwork")); }

char * fretwork_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode) {
  if (mode==MODE_PAINT) 
    return strdup(gettext_noop("Click and drag to draw repetitive patterns. "));
  else
    return strdup(gettext_noop("Click to surround your picture with repetitive patterns."));
 }

int fretwork_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return 1;}

void fretwork_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
		      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
		      int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void fretwork_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Uint8 i;
	
  if (fretwork_snd!=NULL)
    Mix_FreeChunk(fretwork_snd);
  SDL_FreeSurface(fretwork_one);
  SDL_FreeSurface(fretwork_three);
  SDL_FreeSurface(fretwork_four);
  SDL_FreeSurface(fretwork_corner);
  SDL_FreeSurface(fretwork_one_back);
  SDL_FreeSurface(fretwork_three_back);
  SDL_FreeSurface(fretwork_four_back);
  SDL_FreeSurface(fretwork_corner_back);
  SDL_FreeSurface(canvas_backup);
	
  for (i = 0; i < 4; i++)
    free(fretwork_images[i]);
  free(fretwork_images);
  if (fretwork_status_of_segments != NULL)
    free(fretwork_status_of_segments);
}

void fretwork_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED)
{
  //we've to compute the quantity of segments in each direction

  canvas_backup=SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h, canvas->format->BitsPerPixel,
				     canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  SDL_BlitSurface(canvas, NULL, canvas_backup, NULL);
  fretwork_segments_x=fretwork_math_ceil(canvas->w,img_w);
  fretwork_segments_y=fretwork_math_ceil(canvas->h,img_h);
  fretwork_status_of_segments=(Uint8 *)calloc(fretwork_segments_x*fretwork_segments_y + 1, sizeof(Uint8)); //segments starts at 1, while fretwork_status_of_segments[] starts at 0
  fretwork_full_runs=1;
}


void fretwork_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED)
{
  free(fretwork_status_of_segments);
  fretwork_status_of_segments = NULL;
}

// Interactivity functions


static int fretwork_math_ceil(int x, int y)
{	
  int temp;
  temp=(int)x/y;
  if (x%y)
    return temp+1;
  else return temp;
}
	
static unsigned int fretwork_get_segment(int x, int y)
{
  int xx;  //segments are numerated just like pixels
  int yy;  //in computer graphics: left upper (=1), ... ,right upper,
           //left bottom, ... , right bottom
  xx=fretwork_math_ceil(x, img_w);
  yy=fretwork_math_ceil(y, img_h);

  return (yy-1)*fretwork_segments_x+xx;	
}

static void fretwork_extract_coords_from_segment(unsigned int segment, Sint16 * x, Sint16 * y)
{
  *x=((segment%fretwork_segments_x)-1)*img_w;					//useful to set update_rect as small as possible
  *y=(int)(segment/fretwork_segments_x)*img_h;
}

/* static void fretwork_flip(void * ptr, SDL_Surface * dest, SDL_Surface * src) */
/* { */
/*   magic_api * api = (magic_api *) ptr; */

/*   Sint16 x, y; */

/*   for (x=0; x<dest->w; x++) */
/*     for (y=0; y<dest->h; y++) */
/*       api->putpixel(dest, x, y, api->getpixel(src, x, src->h-y)); */
/* } */

static void fretwork_flip_flop(void * ptr, SDL_Surface * dest, SDL_Surface * src)
{
  magic_api * api = (magic_api *) ptr;
  Sint16 x, y;
  for (x=0; x<dest->w; x++)
    for (y=0; y<dest->h; y++)
      api->putpixel(dest, dest->w-1-x, dest->h-1-y, api->getpixel(src, x, y));
}

static void fretwork_rotate (void * ptr, SDL_Surface * dest, SDL_Surface * src, _Bool direction)
     //src and dest must have same size
{
  magic_api * api = (magic_api *) ptr;
  Sint16 x,y;
	
  if (direction)	//rotate -90 degs
    {
      for (x = 0; x<dest->w; x++)
	for (y =0; y<dest->h; y++)
	  api->putpixel(dest, x, y, api->getpixel(src,y,src->h-1-x));
    }
  else			//rotate +90 degs
    {
      for (x=0; x<dest->w; x++)
	for (y=0; y<dest->h; y++)
	  api->putpixel(dest,x,y,api->getpixel(src,src->h-y-1,x));
    }
	
}


void fretwork_click(magic_api * api, int which, int mode,
		    SDL_Surface * canvas, SDL_Surface * snapshot,
		    int x, int y, SDL_Rect * update_rect)
{
  int left_x, right_x, top_y, bottom_y;
  fretwork_segment_modified_last = 0;
  if (mode==MODE_PAINT)
    {
  fretwork_segment_last_clicked=fretwork_get_segment(x,y);
  fretwork_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
    }
  else
    {
      if (fretwork_full_runs<=min(fretwork_segments_x,fretwork_segments_y)/2)
	{
	  left_x=img_w*fretwork_full_runs;
	  right_x=img_w*fretwork_segments_x-img_w*fretwork_full_runs;
	  top_y=img_h*fretwork_full_runs;
	  bottom_y=img_h*fretwork_segments_y-img_h*(fretwork_full_runs-1);

	  //left line
	  api->line((void *) api, which, canvas, snapshot, left_x, top_y, left_x, bottom_y, img_w/2, fretwork_draw_wrapper);

	  //top line
	  api->line((void *) api, which, canvas, snapshot, left_x, top_y, right_x, top_y, img_w/2, fretwork_draw_wrapper);

	  //bottom line
	  api->line((void *) api, which, canvas, snapshot, left_x, bottom_y, right_x, bottom_y, img_w/2, fretwork_draw_wrapper);

	  //right line
	  api->line((void *) api, which, canvas, snapshot, right_x, top_y, right_x, bottom_y, img_w/2, fretwork_draw_wrapper);

	  fretwork_full_runs +=1;
	  update_rect->x=0;
	  update_rect->y=0;
	  update_rect->w=canvas->w;
	  update_rect->h=canvas->h;
	}
    }
}

static Uint8 fretwork_select_image(Uint16 segment)
{
  int take_up, take_down;
  int val_up, val_down, val_left, val_right;
  int from_top = 0, from_bottom = 0, from_left = 0, from_right = 0;
  int from_top_right = 0, from_top_left = 0, from_bottom_right = 0, from_bottom_left = 0;
  int TOP = 0, BOTTOM = 0, LEFT = 0, RIGHT = 0;

  //Checking from were we come...
  if (fretwork_segment_modified_last>0)
    {
      if (segment == fretwork_segment_modified_last + 1)
	from_left = 1;
      else if (segment == fretwork_segment_modified_last - 1)
	from_right = 1;
      else if (segment == fretwork_segment_modified_last - fretwork_segments_x)
	from_bottom = 1;
      else if (segment == fretwork_segment_modified_last + fretwork_segments_x)
	from_top = 1;
	
      // Very very few cases will reach this, segments are joining by the corner
      // We need to add a new segment to join by side, adding clockwise
      else if (segment == fretwork_segment_modified_last + fretwork_segments_x + 1)
	{
	  from_top_left = 1;
	  fretwork_segment_to_add = segment - fretwork_segments_x;
	}
      else if (segment == fretwork_segment_modified_last + fretwork_segments_x - 1)
	{
	  from_top_right = 1;
	  fretwork_segment_to_add = segment + 1;
	}
      else if (segment == fretwork_segment_modified_last - fretwork_segments_x - 1)
	{
	  from_bottom_right = 1;
	  fretwork_segment_to_add = segment + fretwork_segments_x;
	}
      else if (segment == fretwork_segment_modified_last - fretwork_segments_x + 1)
	{
	  from_bottom_left = 1;
	  fretwork_segment_to_add = segment -1;
	}
    }

  take_up=segment-fretwork_segments_x;	
  if (take_up<=0) val_up = SEG_NONE;
  else val_up = fretwork_status_of_segments[take_up];

  take_down=segment+fretwork_segments_x;
  if (take_down>(signed)(fretwork_segments_x*fretwork_segments_y)) val_down = SEG_NONE;
  else val_down = fretwork_status_of_segments[take_down];
	
  if ((segment%fretwork_segments_x)==1) val_left=SEG_NONE;
  else val_left = fretwork_status_of_segments[segment-1];
	
  if ((segment%fretwork_segments_x)==0) val_right=SEG_NONE;
  else val_right = fretwork_status_of_segments[segment+1];

  if ( from_left || (val_left & SEG_RIGHT) || from_bottom_left)
    {
      LEFT = 1;}
  if ( from_right || (val_right & SEG_LEFT) || from_top_right)
    RIGHT=1;
  if ( from_top || (val_up & SEG_BOTTOM) || from_top_left)
    TOP=1;
  if (from_bottom || (val_down & SEG_TOP) || from_bottom_right)
    BOTTOM=1;


  if (TOP && BOTTOM && LEFT && RIGHT)
    return SEG_LEFT_RIGHT_TOP_BOTTOM;
  if (LEFT && RIGHT && TOP)
    return SEG_LEFT_RIGHT_TOP;
  if (LEFT && RIGHT && BOTTOM)
    return SEG_LEFT_RIGHT_BOTTOM;
  if (TOP && BOTTOM && LEFT)
    return SEG_LEFT_TOP_BOTTOM;
  if (TOP && BOTTOM && RIGHT)
    return SEG_RIGHT_TOP_BOTTOM;
  if (LEFT &&RIGHT)
    return SEG_LEFT_RIGHT;
  if (TOP&&BOTTOM)
    return SEG_TOP_BOTTOM;
  if (LEFT&&TOP)
    return SEG_LEFT_TOP;
  if (LEFT&&BOTTOM)
    return SEG_LEFT_BOTTOM;
  if (RIGHT&&TOP)
    return SEG_RIGHT_TOP;
  if (RIGHT&&BOTTOM)
    return SEG_RIGHT_BOTTOM;
  if (LEFT|RIGHT)
    return SEG_LEFT_RIGHT;
  //if (TOP||BOTTOM)
  return SEG_TOP_BOTTOM;
}


static void fretwork_draw(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
			  int x, int y ATTRIBUTE_UNUSED, unsigned int segment)
{
  magic_api * api = (magic_api *) ptr;
  SDL_Surface * result, * temp;
  Uint8 image;
  _Bool use_temp;
	
  use_temp=0;
  if ((segment<1)|(segment>fretwork_segments_x*fretwork_segments_y))
    return;
  fretwork_extract_coords_from_segment(segment, &modification_rect.x, &modification_rect.y);
  modification_rect.h=img_w;
  modification_rect.w=img_h;
	
  image=fretwork_select_image(segment);		//select the image to display

  if (fretwork_status_of_segments[segment] == image)
    return;

  fretwork_status_of_segments[segment]=image;	//and write it to global table
	
  result=SDL_CreateRGBSurface(SDL_SWSURFACE, img_w, img_h, fretwork_one->format->BitsPerPixel,
			      fretwork_one->format->Rmask, fretwork_one->format->Gmask, fretwork_one->format->Bmask, fretwork_one->format->Amask);

  temp=SDL_CreateRGBSurface(SDL_SWSURFACE, img_w, img_h, fretwork_one->format->BitsPerPixel,
			    fretwork_one->format->Rmask, fretwork_one->format->Gmask, fretwork_one->format->Bmask, fretwork_one->format->Amask);

  SDL_BlitSurface(canvas_backup, &modification_rect, result, NULL);
	
  switch(image)
    {
    case 0:
    case SEG_TOP_BOTTOM:	
      SDL_BlitSurface(canvas_backup, &modification_rect, result, NULL);
      SDL_BlitSurface(fretwork_one, NULL, result, NULL);
      break;
		
    case SEG_LEFT_RIGHT:
      SDL_BlitSurface(canvas_backup, &modification_rect, result, NULL);
      fretwork_rotate(api, temp, fretwork_one, 1);
      use_temp=1;
      break;
		
    case SEG_LEFT_RIGHT_TOP_BOTTOM:
      SDL_BlitSurface(canvas_backup, &modification_rect, result, NULL);
      SDL_BlitSurface(fretwork_four, NULL, result, NULL);
      break;
		
    case SEG_LEFT_RIGHT_TOP:
      SDL_BlitSurface(fretwork_three, NULL, result, NULL);
      break;
		
    case SEG_LEFT_RIGHT_BOTTOM:
      fretwork_flip_flop(api, temp, fretwork_three);
      use_temp=1;
      break;
		
    case SEG_LEFT_TOP_BOTTOM:
      fretwork_rotate(api, temp, fretwork_three, 0);
      use_temp=1;
      break;
		
    case SEG_RIGHT_TOP_BOTTOM:
      fretwork_rotate(api, temp, fretwork_three, 1);
      use_temp=1;
      break;

    case SEG_RIGHT_TOP:
      SDL_BlitSurface(fretwork_corner, NULL, result, NULL);
      break;

    case SEG_RIGHT_BOTTOM:
      fretwork_rotate(api, temp, fretwork_corner,1);
      use_temp=1;
      break;

    case SEG_LEFT_TOP:
      fretwork_rotate(api, temp, fretwork_corner, 0);
      use_temp=1;
      break;

    case SEG_LEFT_BOTTOM:
      fretwork_flip_flop(api, temp, fretwork_corner);
      use_temp=1;
      break;
    }
	
  if (use_temp) 
    SDL_BlitSurface(temp, NULL, result, NULL);
	
  SDL_FreeSurface(temp);
  SDL_BlitSurface(result, NULL, canvas, &modification_rect);
  SDL_FreeSurface(result);
  api->playsound(fretwork_snd, (x * 255) / canvas->w, 255);
}


static void fretwork_draw_wrapper(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
				  int x, int y)
{
  fretwork_segment_modified=fretwork_get_segment(x,y);

  fretwork_draw((void *) ptr, which, canvas, last, x, y, fretwork_segment_modified);

  if (fretwork_segment_modified_last>0)

    {
      fretwork_draw((void *) ptr, which, canvas, last, x, y, fretwork_segment_modified_last);
      fretwork_extract_coords_from_segment(fretwork_segment_start_rectangle, &modification_rect.x, &modification_rect.y);
      modification_rect.w=fretwork_update_rectangle_width*img_w;
      modification_rect.h=fretwork_update_rectangle_height*img_h;
    }

  if (fretwork_segment_to_add>0){
    fretwork_draw((void *) ptr, which, canvas, last, x, y, fretwork_segment_to_add);
    fretwork_draw((void *) ptr, which, canvas, last, x, y, fretwork_segment_modified_last);
    fretwork_segment_to_add=0;}

  fretwork_segment_modified_last=fretwork_segment_modified;
}

void fretwork_drag(magic_api * api, int which,
		   SDL_Surface * canvas, SDL_Surface * snapshot, int ox, int oy, int x, int y,
		   SDL_Rect * update_rect)
{     int start_x, end_x, start_y, end_y, segment_start, segment_end, w, h;
 if ((x<canvas->w)&&(y<canvas->h)&&(ox<canvas->w)&&(oy<canvas->h)&&((signed)x>0)&&((signed)y>0)&&((signed)ox>0)&&((signed)oy>0))
   {
     api->line((void *) api, which, canvas, snapshot, ox, oy, x, y, img_w/2, fretwork_draw_wrapper);
     // This should be improved, maybe passed to fretwork_draw()
     start_x=min(ox,x);
     end_x=max(ox,x);
     start_y=min(oy,y);
     end_y=max(oy,y);
     segment_start=fretwork_get_segment(start_x-img_w, start_y-img_h);
     segment_end=fretwork_get_segment(end_x+img_w,end_y+img_h);

     x=((segment_start%fretwork_segments_x)-1)*img_w;
     y=(int)(segment_start/fretwork_segments_x)*img_h;
     w=((segment_end%fretwork_segments_x)-1)*img_w-x+img_w;
     h=(int)(segment_end/fretwork_segments_x)*img_h-y+img_h;

     update_rect->x=x;
     update_rect->y=y;
     update_rect->w=w;
     update_rect->h=h;}

}
