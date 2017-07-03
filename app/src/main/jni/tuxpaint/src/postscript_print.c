/*
  postscript_print.c

  For Tux Paint
  PostScript(r) printing routine.
  (for non-Windows, non-Mac OS X, non-BeOS platforms, e.g. Linux)
  (moved from tuxpaint.c in 0.9.17)

  Copyright (c) 2009 by Bill Kendrick and others
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

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

  Based loosely on examination of NetPBM's "pnmtops" code and output:
  copyright (c) 1989 by Jef Poskanzer.
  License from "pnmtops.c":
    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose and without fee is hereby granted, provided
    that the above copyright notice appear in all copies and that both that
    copyright notice and this permission notice appear in supporting
    documentation.  This software is provided "as is" without express or
    implied warranty.


  June 24, 2007 - January 29, 2009
  $Id: postscript_print.c,v 1.9 2009/11/23 07:45:25 albert Exp $
*/

#include "postscript_print.h"

#ifdef PRINTMETHOD_PS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <paper.h>
#include <math.h>
#include <errno.h>

#ifndef PAPER_H
#error "---------------------------------------------------"
#error "If you installed libpaper from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libpaper-dev.rpm')"
#error "---------------------------------------------------"
#endif

#include "pixels.h"

#define MARGIN 36 /* Margin to put around image, in points (inch/72) (36pt = 0.5") */

#define my_min(x,y) ((x < y) ? (x) : (y))

static int f2int(float f)
{
  return ((int)f);
}

static int f2dec(float f)
{
  return (int)((f - f2int(f)) * 100);
}

/* Actually save the PostScript data to the file stream: */
int do_ps_save(FILE * fi,
		const char *restrict const fname,
		SDL_Surface * surf,
		const char *restrict pprsize,
                int is_pipe)
{
  const struct paper * ppr;
  int img_w = surf->w;
  int img_h = surf->h;
  int r_img_w, r_img_h;
  int ppr_w, ppr_h;
  int x, y;
  float tlate_x, tlate_y;
  int cur_line_len;
  int plane;
  Uint8 r, g, b;
  char buf[256];
  Uint32(*getpixel) (SDL_Surface *, int, int) =
    getpixels[surf->format->BytesPerPixel];
  int printed_img_w, printed_img_h;
  time_t t = time(NULL);
  int rotate;
  float scale;


  /* Determine paper size: */

  paperinit(); // FIXME: Should we do this at startup? -bjk 2007.06.25

  if (pprsize == NULL)
  {
    /* User did not request a specific paper size (on command-line or
       in config file), ask the system.  It will return either their
       $PAPER env. var., the value from /etc/papersize, or NULL: */

    pprsize = systempapername();

    if (pprsize == NULL)
    {
      /* No setting, env. var. or /etc/ file; use the default! */

      pprsize = defaultpapername();

#ifdef DEBUG
      printf("Using default paper\n");
#endif
    }
#ifdef DEBUG
    else
    {
      printf("Using system paper\n");
    }
#endif
  }
#ifdef DEBUG
  else
  {
    printf("Using user paper\n");
  }
#endif
  
#ifdef DEBUG
  printf("Using paper size: %s\n", pprsize);
#endif


  /* Determine attributes of paper of the size chosen/determined: */

  ppr = paperinfo(pprsize);

  ppr_w = paperpswidth(ppr);
  ppr_h = paperpsheight(ppr);

#ifdef DEBUG
  printf("Paper is %d x %d (%.2f\" x %.2f\")\n", ppr_w, ppr_h,
	 (float) ppr_w / 72.0, (float) ppr_h / 72.0);
#endif

  paperdone(); // FIXME: Should we do this at quit? -bjk 2007.06.25


  /* Determine whether it's best to rotate the image: */

  if ((ppr_w >= ppr_h && img_w >= img_h) ||
      (ppr_w <= ppr_h && img_w <= img_h))
  {
    rotate = 0;
    r_img_w = img_w;
    r_img_h = img_h;
  }
  else
  {
    rotate = 1;
    r_img_w = img_h;
    r_img_h = img_w;
  }

#ifdef DEBUG
  printf("Image is %d x %d\n", img_w, img_h);
  printf("Rotated? %s\n", rotate ? "yes" : "no");
  printf("Will print %d x %d pixels\n", r_img_w, r_img_h);
#endif


  /* Determine scale: */

  scale = my_min(((float) (ppr_w - (MARGIN * 2)) / (float) r_img_w),
         ((float) (ppr_h - (MARGIN * 2)) / (float) r_img_h));

  printed_img_w = r_img_w * scale;
  printed_img_h = r_img_h * scale;

#ifdef DEBUG
  printf("Scaling image by %.2f (to %d x %d)\n", scale,
	 printed_img_w, printed_img_h);
#endif


  // FIXME - doesn't seem to center well -bjk 2007.06.25
  tlate_x = (ppr_w - printed_img_w) / 2;
  tlate_y = (ppr_h - printed_img_h) / 2;


  /* Based off of output from "pnmtops", Tux Paint 0.9.15 thru
     0.9.17 CVS as of June 2007, and Adobe Systems Incorporated's
     'PostScript(r) Language Reference, 3rd Ed.' */

  /* Begin PostScript output with some useful meta info in comments: */

  fprintf(fi, "%%!PS-Adobe-2.0 EPSF-2.0\n"); // we need LanguageLevel2 for color

  fprintf(fi, "%%%%Title: (%s)\n", fname);

  strftime(buf, sizeof buf - 1, "%a %b %e %H:%M:%S %Y", localtime(&t));
  fprintf(fi, "%%%%CreationDate: (%s)\n", buf);

  fprintf(fi, "%%%%Creator: (Tux Paint " VER_VERSION ", " VER_DATE ")\n");

  fprintf(fi, "%%%%Pages: 1\n");

  fprintf(fi, "%%%%BoundingBox: 0 0 %d %d\n", (int) (ppr_w + 0.5), (int)
(ppr_h + 0.5));

  fprintf(fi, "%%%%EndComments\n");


  /* Define a 'readstring' routine and 'picstr' routines for RGB: */

  fprintf(fi, "/readstring {\n");
  fprintf(fi, "  currentfile exch readhexstring pop\n");
  fprintf(fi, "} bind def\n");

  fprintf(fi, "/rpicstr %d string def\n", img_w);
  fprintf(fi, "/gpicstr %d string def\n", img_w);
  fprintf(fi, "/bpicstr %d string def\n", img_w);

  fprintf(fi, "%%%%EndProlog\n");

  fprintf(fi, "%%%%Page: 1 1\n");

  fprintf(fi, "<< /PageSize [ %d %d ] /ImagingBBox null >> setpagedevice\n",
              ppr_w, ppr_h);

  fprintf(fi, "gsave\n");

  /* 'translate' moves the user space origin to a new position with
  respect to the current page, leaving the orientation of the axes and
  the unit lengths unchanged. */
  fprintf(fi, "%d.%02d %d.%02d translate\n",
	  f2int(tlate_x), f2dec(tlate_x),
	  f2int(tlate_y), f2dec(tlate_y));

  /* 'scale' modiﬁes the unit lengths independently along the current
  x and y axes, leaving the origin location and the orientation of the
  axes unchanged. */
  fprintf(fi, "%d.%02d %d.%02d scale\n",
          f2int(printed_img_w), f2dec(printed_img_w),
          f2int(printed_img_h), f2dec(printed_img_h));

  /* Rotate the image */
  if (rotate)
    fprintf(fi, "0.5 0.5 translate  90 rotate  -0.5 -0.5 translate\n");

  fprintf(fi, "%d %d 8\n", img_w, img_h);
  fprintf(fi, "[ %d 0 0 %d 0 %d ]\n", img_w, -img_h, img_h);

  fprintf(fi, "{ rpicstr readstring }\n");
  fprintf(fi, "{ gpicstr readstring }\n");
  fprintf(fi, "{ bpicstr readstring }\n");

  fprintf(fi, "true 3\n");

  fprintf(fi, "colorimage\n");

  cur_line_len = 0;

  for (y = 0; y < img_h; y++)
  {
    for (plane = 0; plane < 3; plane++)
    {
      for (x = 0; x < img_w; x++)
      {
        SDL_GetRGB(getpixel(surf, x, y), surf->format, &r, &g, &b);
        fprintf(fi, "%02x", (plane == 0 ? r : (plane == 1 ? g : b)));

        cur_line_len++;
        if (cur_line_len >= 30)
        {
          fprintf(fi, "\n");
          cur_line_len = 0;
        }
      }
    }
  }

  fprintf(fi, "\n");
  fprintf(fi, "grestore\n");
  fprintf(fi, "showpage\n");
  fprintf(fi, "%%%%Trailer\n");
  fprintf(fi, "%%%%EOF\n");

  if (!is_pipe)
  {
    fclose(fi);
    return 1;
  }
  else
  {
    pid_t child_pid, w;
    int status;

    child_pid = pclose(fi);

/* debug */
/*
    printf("pclose returned %d\n", child_pid); fflush(stdout);
    printf("errno = %d\n", errno); fflush(stdout);
*/

    if (child_pid < 0 || (errno != 0 && errno != EAGAIN)) { /* FIXME: This right? */
      return 0;
    } else if (child_pid == 0) {
      return 1;
    }

    do
    {
      w = waitpid(child_pid, &status, 0);

/* debug */
/*
            if (w == -1) { perror("waitpid"); exit(EXIT_FAILURE); }
            if (WIFEXITED(status)) {
                printf("exited, status=%d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d\n", WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                printf("continued\n");
            }
*/
    }
    while (w != -1 && !WIFEXITED(status) && !WIFSIGNALED(status));

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) /* Not happy exit */
      return 0;

    return 1;
  }
}

#endif

