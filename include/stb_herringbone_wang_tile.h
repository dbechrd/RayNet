#include "../src/common/common.h"

/* stbhw - v0.7 -  http://nothings.org/gamedev/herringbone
   Herringbone Wang Tile Generator - Sean Barrett 2014 - public domain

== LICENSE ==============================

This software is dual-licensed to the public domain and under the following
license: you are granted a perpetual, irrevocable license to copy, modify,
publish, and distribute this file as you see fit.

== WHAT IT IS ===========================

 This library is an SDK for Herringbone Wang Tile generation:

      http://nothings.org/gamedev/herringbone

 The core design is that you use this library offline to generate a
 "template" of the tiles you'll create. You then edit those tiles, then
 load the created tile image file back into this library and use it at
 runtime to generate "maps".

 You cannot load arbitrary tile image files with this library; it is
 only designed to load image files made from the template it created.
 It stores a binary description of the tile sizes & constraints in a
 few pixels, and uses those to recover the rules, rather than trying
 to parse the tiles themselves.

 You *can* use this library to generate from arbitrary tile sets, but
 only by loading the tile set and specifying the constraints explicitly
 yourself.

== COMPILING ============================

 1. #define STB_HERRINGBONE_WANG_TILE_IMPLEMENTATION before including this
    header file in *one* source file to create the implementation
    in that source file.

 2. optionally #define STB_HBWANG_RAND() to be a random number
    generator. if you don't define it, it will use rand(),
    and you need to seed srand() yourself.

 3. optionally #define STB_HBWANG_ASSERT(x), otherwise
    it will use assert()

 4. optionally #define STB_HBWANG_STATIC to force all symbols to be
    static instead of public, so they are only accesible
    in the source file that creates the implementation

 5. optionally #define STB_HBWANG_NO_REPITITION_REDUCTION to disable
    the code that tries to reduce having the same tile appear
    adjacent to itself in wang-corner-tile mode (e.g. imagine
    if you were doing something where 90% of things should be
    the same grass tile, you need to disable this system)

 6. optionally define STB_HBWANG_MAX_X and STB_HBWANG_MAX_Y
    to be the max dimensions of the generated map in multiples
    of the wang tile's short side's length (e.g. if you
    have 20x10 wang tiles, so short_side_len=10, and you
    have MAX_X is 17, then the largest map you can generate
    is 170 pixels wide). The defaults are 100x100. This
    is used to define static arrays which affect memory
    usage.

== USING ================================

  To use the map generator, you need a tileset. You can download
  some sample tilesets from http://nothings.org/gamedev/herringbone

  Then see the "sample application" below.

  You can also use this file to generate templates for
  tilesets which you then hand-edit to create the data.


== MEMORY MANAGEMENT ====================

  The tileset loader allocates memory with malloc(). The map
  generator does no memory allocation, so e.g. you can load
  tilesets at startup and never free them and never do any
  further allocation.


== SAMPLE APPLICATION ===================

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"        // http://nothings.org/stb_image.c

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"  // http://nothings.org/stb/stb_image_write.h

#define STB_HBWANG_IMPLEMENTATION
#include "stb_hbwang.h"

int main(int argc, char **argv)
{
   unsigned char *data;
   int xs,ys, w,h;
   stbhw_tileset ts;

   if (argc != 4) {
      fprintf(stderr, "Usage: mapgen {tile-file} {xsize} {ysize}\n"
                      "generates file named 'test_map.png'\n");
      exit(1);
   }
   data = stbi_load(argv[1], &w, &h, NULL, 3);
   xs = atoi(argv[2]);
   ys = atoi(argv[3]);
   if (data == NULL) {
      fprintf(stderr, "Error opening or parsing '%s' as an image file\n", argv[1]);
      exit(1);
   }
   if (xs < 1 || xs > 1000) {
      fprintf(stderr, "xsize invalid or out of range\n");
      exit(1);
   }
   if (ys < 1 || ys > 1000) {
      fprintf(stderr, "ysize invalid or out of range\n");
      exit(1);
   }

   stbhw_build_tileset_from_image(&ts, data, w*3, w, h);
   free(data);

   // allocate a buffer to create the final image to
   data = malloc(3 * xs * ys);

   srand(time(NULL));
   stbhw_generate_image(&ts, NULL, data, xs*3, xs, ys);

   stbi_write_png("test_map.png", xs, ys, 3, data, xs*3);

   stbhw_free_tileset(&ts);
   free(data);

   return 0;
}

== VERSION HISTORY ===================

   0.7   2019-03-04   - fix warnings
	0.6   2014-08-17   - fix broken map-maker
	0.5   2014-07-07   - initial release

*/

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                         HEADER FILE SECTION                              //
//                                                                          //

#ifndef INCLUDE_STB_HWANG_H
#define INCLUDE_STB_HWANG_H

#ifdef STB_HBWANG_STATIC
#define STBHW_EXTERN static
#else
#ifdef __cplusplus
#define STBHW_EXTERN extern "C"
#else
#define STBHW_EXTERN extern
#endif
#endif

typedef struct stbhw_tileset stbhw_tileset;

// returns description of last error produced by any function (not thread-safe)
STBHW_EXTERN const char *stbhw_get_last_error(void);

// build a tileset from an image that conforms to a template created by this
// library. (you allocate storage for stbhw_tileset and function fills it out;
// memory for individual tiles are malloc()ed).
// returns non-zero on success, 0 on error
STBHW_EXTERN int stbhw_build_tileset_from_image(stbhw_tileset *ts, Image image);

// free a tileset built by stbhw_build_tileset_from_image
STBHW_EXTERN void stbhw_free_tileset(stbhw_tileset *ts);

// generate a map that is w * h pixels (3-bytes each)
// returns non-zero on success, 0 on error
// not thread-safe (uses a global data structure to avoid memory management)
// weighting should be NULL, as non-NULL weighting is currently untested
STBHW_EXTERN int stbhw_generate_image(stbhw_tileset *ts, int **weighting,
                     unsigned char *pixels, int stride_in_bytes, int w, int h);

//////////////////////////////////////
//
// TILESET DATA STRUCTURE
//
// if you use the image-to-tileset system from this file, you
// don't need to worry about these data structures. but if you
// want to build/load a tileset yourself, you'll need to fill
// these out.

typedef struct
{
   // the edge or vertex constraints, according to diagram below
   signed char a,b,c,d,e,f;

   // The herringbone wang tile data; it is a bitmap which is either
   // w=2*short_sidelen,h=short_sidelen, or w=short_sidelen,h=2*short_sidelen.
   // it is always RGB, stored row-major, with no padding between rows.
   // (allocate stbhw_tile structure to be large enough for the pixel data)
   //unsigned char pixels[1];
   uint32_t x,y;  // uv coords of top left of tile
} stbhw_tile;

struct stbhw_tileset
{
   int is_corner;
   int num_color[6];  // number of colors for each of 6 edge types or 4 corner types
   int short_side_len;
   stbhw_tile **h_tiles;
   stbhw_tile **v_tiles;
   int num_h_tiles, max_h_tiles;
   int num_v_tiles, max_v_tiles;
   Image img{};
};

///////////////  TEMPLATE GENERATOR  //////////////////////////

// when requesting a template, you fill out this data
typedef struct
{
   int is_corner;      // using corner colors or edge colors?
   int short_side_len; // rectangles is 2n x n, n = short_side_len
   int num_color[6];   // see below diagram for meaning of the index to this;
                       // 6 values if edge (!is_corner), 4 values if is_corner
                       // legal numbers: 1..8 if edge, 1..4 if is_corner
   int num_vary_x;     // additional number of variations along x axis in the template
   int num_vary_y;     // additional number of variations along y axis in the template
   int corner_type_color_template[4][4];
      // if corner_type_color_template[s][t] is non-zero, then any
      // corner of type s generated as color t will get a little
      // corner sample markup in the template image data

} stbhw_config;

// computes the size needed for the template image
STBHW_EXTERN void stbhw_get_template_size(stbhw_config *c, int *w, int *h);

// generates a template image, assuming data is 3*w*h bytes long, RGB format
STBHW_EXTERN int stbhw_make_template(stbhw_config *c, Image image);

#endif//INCLUDE_STB_HWANG_H


// TILE CONSTRAINT TYPES
//
// there are 4 "types" of corners and 6 types of edges.
// you can configure the tileset to have different numbers
// of colors for each type of color or edge.
//
// corner types:
//
//                     0---*---1---*---2---*---3
//                     |       |               |
//                     *       *               *
//                     |       |               |
//     1---*---2---*---3       0---*---1---*---2
//     |               |       |
//     *               *       *
//     |               |       |
//     0---*---1---*---2---*---3
//
//
//  edge types:
//
//     *---2---*---3---*      *---0---*
//     |               |      |       |
//     1               4      5       1
//     |               |      |       |
//     *---0---*---2---*      *       *
//                            |       |
//                            4       5
//                            |       |
//                            *---3---*
//
// TILE CONSTRAINTS
//
// each corner/edge has a color; this shows the name
// of the variable containing the color
//
// corner constraints:
//
//                        a---*---d
//                        |       |
//                        *       *
//                        |       |
//     a---*---b---*---c  b       e
//     |               |  |       |
//     *               *  *       *
//     |               |  |       |
//     d---*---e---*---f  c---*---f
//
//
//  edge constraints:
//
//     *---a---*---b---*      *---a---*
//     |               |      |       |
//     c               d      b       c
//     |               |      |       |
//     *---e---*---f---*      *       *
//                            |       |
//                            d       e
//                            |       |
//                            *---f---*
//
