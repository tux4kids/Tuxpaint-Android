/*
  test-png.c

  A tiny test program that opens a PNG file using libpng
  library.  If any warnings come back, this can help us tell
  when any images include oddities that would cause Tux Paint
  to echo warnings (via SDL_image->libpng) to stderr when it
  goes to load them.

  See https://sourceforge.net/p/tuxpaint/bugs/252/

  Example:
    ./test-png data/images/icon.png
    ./test-png `find . -name "*.png"` | grep -B 2 libpng

  Bill Kendrick <bill@newbreedsoftware.com>

  (Based loosely on example code by Yoshimasa Niwa, https://gist.github.com/niw,
  located at https://gist.github.com/niw/5963798, which itself was based on
  code by Guillaume Cottenceau found at http://zarb.org/~gc/html/libpng.html)

  2022-07-03 - 2022-07-03
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <png.h>

int main(int argc, char *argv[])
{
  int i, w, h, y;
  FILE *fi;
  png_structp png;
  png_infop info;
  png_byte ctype, depth;
  png_bytep *rows;


  /* Usage output */
  if (argc == 1 || strcmp(argv[1], "--help") == 0)
  {
    fprintf(stderr, "Usage: %s file.png [file.png ...]\n", argv[0]);
    exit(1);
  }


  /* Redirect stderr to stdout, so libpng's warnings can
     be in the same stream as our's (where we report each
     file's filename); allows more easily dumping to a file,
     piping through `| less`, etc. */
  dup2(STDOUT_FILENO, STDERR_FILENO);


  /* Open each PNG image!... */
  for (i = 1; i < argc; i++)
  {
    printf("%5d ------------------------------------------------------------------\n", i);
    printf("%s\n", argv[i]);
    fflush(stdout);

    /* Open the file */
    fi = fopen(argv[i], "rb");
    if (fi == NULL)
    {
      printf("Cannot open\n");
    }
    else
    {
      /* Prepare PNG library stuff... */
      png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      if (!png)
      {
        fprintf(stderr, "Cannot png_create_read_struct()!\n");
        exit(1);
      }

      info = png_create_info_struct(png);
      if (!info)
      {
        fprintf(stderr, "Cannot png_create_info_struct()!\n");
        exit(1);
      }

      if (setjmp(png_jmpbuf(png)))
      {
        fprintf(stderr, "Cannot setjmp(png_jmpbuf(png)))!\n");
        exit(1);
      }

      /* Read the PNG's info */
      png_init_io(png, fi);
      png_read_info(png, info);

      /* *** At this point, libpng would have reported issues 
         about some chunks; e.g. "iCCP: too many profiles",
         "sRGB: out of place", "bKGD: invalid gray level", etc. *** */

      w = png_get_image_width(png, info);
      h = png_get_image_height(png, info);
      ctype = png_get_color_type(png, info);
      depth = png_get_bit_depth(png, info);

      /* If 16-bit, strip to 8-bit */
      if (depth == 16)
      {
        printf("test-png warning: 16-bit\n");
        png_set_strip_16(png);
      }

      /* Switch palette to RGB */
      if (ctype == PNG_COLOR_TYPE_PALETTE)
      {
        printf("test-png warning: paletted\n");
        png_set_palette_to_rgb(png);
      }

      /* Expand low-depth greyscale up to 8-bit */
      if (ctype == PNG_COLOR_TYPE_GRAY && depth < 8)
      {
        printf("test-png warning: greyscale with only %d-bit depth\n", depth);
        png_set_expand_gray_1_2_4_to_8(png);
      }

      /* Expand tRNS chunks into alpha */
      if (png_get_valid(png, info, PNG_INFO_tRNS))
      {
        printf("test-png warning: contains tRNS chunk\n");
        png_set_tRNS_to_alpha(png);
      }

      /* Fill alpha channel if there is none */
      if (ctype == PNG_COLOR_TYPE_RGB || ctype == PNG_COLOR_TYPE_GRAY || ctype == PNG_COLOR_TYPE_PALETTE)
      {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
      }

      /* Expand grey to color */
      if (ctype == PNG_COLOR_TYPE_GRAY || ctype == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
        printf("test-png warning: greyscale\n");
        png_set_gray_to_rgb(png);
      }

      /* Update 'info' struct based on whatever was changed above */
      png_read_update_info(png, info);

      /* Allocate space */
      rows = (png_bytep *) malloc(sizeof(png_bytep) * h);
      if (!rows)
      {
        fprintf(stderr, "Failed to malloc() space for image data rows!\n");
        exit(1);
      }

      for (y = 0; y < h; y++)
      {
        rows[y] = (png_byte *) malloc(png_get_rowbytes(png, info));
        if (!rows[y])
        {
          fprintf(stderr, "Failed to malloc() space for image data row #%d!\n", y);
          exit(1);
        }
      }

      png_read_image(png, rows);

      /* *** At this point, libpng would have reported issues while
         loading the image, e.g.
         "Interlace handling should be turned on when using png_read_image" */


      /* Clean up and move on to the next file */
      for (y = 0; y < h; y++)
        free(rows[y]);
      free(rows);

      png_destroy_read_struct(&png, &info, NULL);
      fclose(fi);
    }

    printf("\n");
    fflush(stdout);
  }

  return (0);
}
