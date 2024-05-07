#ifndef TRANSCODE_H
#define TRANSCODE_H

#define NOEDGE 255
#define POSSIBLE_EDGE 128
#define EDGE 0

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define DEFAULT_SMPL_FRAMES 64
#define DEFAULT_AGGRESSIVE_CROP 0
#define DEFAULT_W_BAND_PERC 35
#define DEFAULT_H_BAND_PERC 18
#define DEFAULT_LEVEL_BLACK 23
#define DEFAULT_LEVEL_DELTA 65
#define DEFAULT_BORDER_BLACK 15
#define DEFAULT_BORDER_BAND 4
#define DEFAULT_CANNY_THRESHOLD 170
#define DEFAULT_CANNY_SIGMA 1.2
#define DEFAULT_CANNY_TLOW 0.3
#define DEFAULT_CANNY_THIGH 0.8
#define DEFAULT_PIXEL_RATIO 0

enum direction
{ HORIZONTAL, VERTICAL };

enum corners
{ NW, NE, SW, SE, CORNERS };

enum sides
{ LEFT, TOP, RIGHT, BOTTOM, SIDES };

struct crop
{
  int           top, bottom, left, right;
};

struct swopt
{
  unsigned      info:1;
  unsigned      canny:1;
  unsigned      tworect:1;
  unsigned      pcx:1;
  unsigned      logomask:1;
  unsigned      jump_corner:1;
  u8            corner_mask;
  int		aggressive_crop;
  int           pixel_ratio;
  int           smpl_frames;
  int           w_band_perc;
  int           h_band_perc;
  int           level_black;
  int           level_delta;
  int           border_black;
  int           border_band;
  int           canny_threshold;
  float         canny_sigma;
  float         canny_tlow;
  float         canny_thigh;
  char         *fnamein;
};

struct shot
{
  u8           *image;
  enum corners  corner;
  int           timestamp_ms;
};

struct crn
{
  u8           *image;
  int           lx, ly;
  int           lw, lh;
  int           offx, offy;
  int           count;
};

extern struct swopt sw;

/* png_pcx.c */
int           save_image( char *format, int sequence, int bpp, int planes,
    int w, int h, u8 * image );

/* hysteresis.c */
void          follow_edges( u8 * edgemapptr, short *edgemagptr, short lowval,
    int cols );
void          apply_hysteresis( short int *mag, u8 * nms, int rows, int cols,
    float tlow, float thigh, u8 * edge );
void          non_max_supp( short *mag, short *gradx, short *grady, int nrows,
    int ncols, u8 * result );

/* edge_scan.c */
int           canny_logo( int w, int h, u32 wc, u32 hc, unsigned offs,
    struct shot *Shots, struct crn *Crn, int corner );

/* overlap_two.c */
int           overlap_tworect( u8 * C, unsigned wc, unsigned hc,
    struct crop *bbox, struct crop *bbox1, struct crop *bbox2 );

/* crop_delogo.c */
void          crop_delogo( int w, int h, int ow, int oh, struct shot *Shots );
int           bounding_box( u32 wc, u32 hc, struct crn *Crn,
    struct crop *box );
void          delogo_box( int corner, int w, int h, int ow, int oh, int wc,
    int hc, struct crn *Crn, struct crop *Crop, struct crop *box );

/* jumping_logo.c */
u8           *convolve_corners( int w, int h, int ow, int oh, int wc, int hc,
    struct crn *Corners, struct crop *Crop );
int           find_which_corner( u8 * img_match, struct crn *Corners,
    struct shot *Shot );

#endif
