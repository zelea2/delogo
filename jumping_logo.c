#include <limits.h>
#include "delogo.h"

// #define DEBUG
#define AREA_RATIO 256

static void
select_two_corners( int sd, struct crn *Crn,
    struct crn **Crn1, struct crn **Crn2, enum direction *dir )
{
  switch ( sd )
  {
    case LEFT:
      *Crn1 = &Crn[NW];
      *Crn2 = &Crn[SW];
      *dir = VERTICAL;
      break;
    case RIGHT:
      *Crn1 = &Crn[NE];
      *Crn2 = &Crn[SE];
      *dir = VERTICAL;
      break;
    case TOP:
      *Crn1 = &Crn[NW];
      *Crn2 = &Crn[NE];
      *dir = HORIZONTAL;
      break;
    case BOTTOM:
      *Crn1 = &Crn[SW];
      *Crn2 = &Crn[SE];
      *dir = HORIZONTAL;
      break;
  }
}

static int
horizontal_match( int wc, int hc, int offy, struct crn *Crn1,
    struct crn *Crn2 )
{
  int           area, max_offx, max_conv;
  int           r, c, offx, conv;
  u8           *row1, *row2, *s1, *s2;

  max_offx = INT_MIN;
  max_conv = 0;
  for( offx = 32 - wc; offx <= wc - 32; offx++ )
  {
    conv = 0;
    area = ( hc - abs( offy ) ) * ( wc - abs( offx ) );
    for( r = 0; r < hc - abs( offy ); r++ )
    {
      if( offy < 0 )
      {
        row1 = Crn1->image + wc * ( r - offy );
        row2 = Crn2->image + wc * r;
      }
      else
      {
        row1 = Crn1->image + wc * r;
        row2 = Crn2->image + wc * ( r + offy );
      }
      for( c = 0; c < wc - abs( offx ); c++ )
      {
        if( offx < 0 )
        {
          s1 = row1 + c - offx;
          s2 = row2 + c;
        }
        else
        {
          s1 = row1 + c;
          s2 = row2 + c + offx;
        }
        conv += *s1 * *s2;
      }
    }
    if( conv > max_conv && conv > area * AREA_RATIO )
    {
      max_offx = offx;
      max_conv = conv;
    }
  }
  if( max_offx != INT_MIN )
  {
    // clear the horizontal sides
    for( r = 0; r < hc; r++ )
    {
      row1 = Crn1->image + wc * r;
      row2 = Crn2->image + wc * r;
      for( c = 0; c < abs( max_offx ); c++ )
      {
        if( max_offx < 0 )
        {
          *( row1 + c ) = 0;
          *( row2 + wc - c - 1 ) = 0;
        }
        else
        {
          *( row1 + wc - c - 1 ) = 0;
          *( row2 + c ) = 0;
        }
      }
    }
  }
  return max_offx;
}

static int
vertical_match( int wc, int hc, int offx, struct crn *Crn1, struct crn *Crn2 )
{
  int           area, max_offy, max_conv;
  int           r, c, offy, conv;
  u8           *row1, *row2, *s1, *s2, *si1, *si2;

  max_offy = INT_MIN;
  max_conv = 0;
  row1 = Crn1->image;
  row2 = Crn2->image;
  for( offy = 16 - hc; offy <= hc - 16; offy++ )
  {
    conv = 0;
    area = ( wc - abs( offx ) ) * ( hc - abs( offy ) );
    for( r = 0; r < hc - abs( offy ); r++ )
    {
      if( offy < 0 )
      {
        s1 = row1 + wc * ( r - offy );
        s2 = row2 + wc * r;
      }
      else
      {
        s1 = row1 + wc * r;
        s2 = row2 + wc * ( r + offy );
      }
      for( c = 0; c < wc - abs( offx ); c++ )
      {
        if( offx < 0 )
        {
          si1 = s1 + c - offx;
          si2 = s2 + c;
        }
        else
        {
          si1 = s1 + c;
          si2 = s2 + c + offx;
        }
        conv += *si1 * *si2;
      }
    }
    if( conv > max_conv && conv > area * AREA_RATIO )
    {
      max_offy = offy;
      max_conv = conv;
    }
  }
  if( max_offy != INT_MIN )
  {
    // clear the vertical sides
    for( r = 0; r < abs( max_offy ); r++ )
    {
      if( offy < 0 )
      {
        s1 = row1 + wc * r;
        s2 = row2 + wc * ( hc - r - 1 );
      }
      else
      {
        s1 = row1 + wc * ( hc - r - 1 );
        s2 = row2 + wc * r;
      }
      for( c = 0; c < wc; c++ )
        *s1++ = *s2++ = 0;
    }
  }
  return max_offy;
}

static int
convolve_two( int wc, int hc, struct crn *Crn1, struct crn *Crn2,
    enum direction dir, int *offx, int *offy )
{
  int           done, ox, oy;

  ox = *offx;
  oy = *offy;
  if( Crn1->image == NULL || Crn2->image == NULL )
    return 0;
  done = 0;
  if( dir == HORIZONTAL )
  {
    do
    {
      if( ( ox = horizontal_match( wc, hc, oy, Crn1, Crn2 ) ) == INT_MIN )
        return 0;
      if( ( oy = vertical_match( wc, hc, ox, Crn1, Crn2 ) ) == INT_MIN )
        return 0;
      if( ox == *offx )
        done = 1;
      *offx = ox;
      *offy = oy;
    }
    while( !done );
    return 1;
  }
  else if( dir == VERTICAL )
  {
    do
    {
      if( ( oy = vertical_match( wc, hc, ox, Crn1, Crn2 ) ) == INT_MIN )
        return 0;
      if( ( ox = horizontal_match( wc, hc, oy, Crn1, Crn2 ) ) == INT_MIN )
        return 0;
      if( oy == *offy )
        done = 1;
      *offx = ox;
      *offy = oy;
    }
    while( !done );
    return 1;
  }
  return 0;
}

static void
overlap( int wc, int hc, int cutx, int cuty, int offx, int offy,
    struct crn *Crn1, struct crn *Crn2, short *magnitude )
{
  int           r, c;
  short        *d;
  u8           *row1, *row2, *s1, *s2;
#ifdef DEBUG
  printf( "wc %d hc %d cutx %d cuty %d offx %d offy %d\n",
      wc, hc, cutx, cuty, offx, offy );
#endif
  if( offx < 0 )
    Crn1->offx = -offx;
  else if( offx > 0 )
    Crn2->offx = offx;
  if( offy < 0 )
    Crn1->offy = -offy;
  else if( offy > 0 )
    Crn2->offy = offy;
  for( r = 0; r < hc - cuty; r++ )
  {
    if( offy < 0 )
    {
      row1 = Crn1->image + wc * ( r - offy );
      row2 = Crn2->image + wc * r;
    }
    else
    {
      row1 = Crn1->image + wc * r;
      row2 = Crn2->image + wc * ( r + offy );
    }
    d = magnitude + ( wc - cutx ) * r;
    for( c = 0; c < wc - cutx; c++ )
    {
      if( offx < 0 )
      {
        s1 = row1 + c - offx;
        s2 = row2 + c;
      }
      else
      {
        s1 = row1 + c;
        s2 = row2 + c + offx;
      }
      *d++ += *s1 + *s2;
    }
  }
}

void
normalize( int w, int h, short *src, u8 *dst )
{
  int           i;
  short         min_val, max_val;
  short        *s;
  min_val = -1;
  max_val = 0;
  for( s = src, i = 0; i < w * h; i++, s++ )
  {
    if( min_val > *s )
      min_val = *s;
    if( max_val < *s )
      max_val = *s;
  }
  if( min_val == max_val )
    return;
  // normalize to 1 byte
  max_val -= min_val;
  for( i = 0; i < w * h; i++, src++, dst++ )
  {
    *dst = 255 * ( *src - min_val ) / max_val;
  }
}

static void
crop_overlayed_logo( int wc, int hc, int *wm, int *hm,
    struct crn *Crn, struct crop *bbox )
{
  int           r, c, len;
  u8           *img_bbox, *s, *d0, *d3, mask;

  *wm = bbox->right + 1 - bbox->left;
  *hm = bbox->bottom + 1 - bbox->top;
  len = wc * hc * 4 / 8;
  if( ( img_bbox = malloc( len ) ) == NULL )
    return;
  memset( img_bbox, 0xff, len );
  for( r = bbox->top; r <= bbox->bottom; r++ )
  {
    // plane 0
    d0 = img_bbox + r * 4 * wc / 8;
    s = Crn->image + r * wc / 8;
    memcpy( d0, s, wc / 8 );
    // plane 3 (alpha)
    d3 = img_bbox + ( r * 4 + 3 ) * wc / 8;
    for( c = bbox->left; c <= bbox->right; c++ )
    {
      mask = 0x80 >> ( c & 7 );
      *( d3 + c / 8 ) &= ~mask;
    }
  }
  save_image( "delogo_j", 0, 1, 4, wc, hc, img_bbox );
  free( img_bbox );
}

static void
threshold_single_frame( int w, int h, u8 *magimg, u8 *image )
{
  int           wB, r, c;
  u8           *p, mask;

  wB = ( w + 7 ) / 8;
  for( r = 0; r < h; r++ )
  {
    p = image + wB * r;
    for( c = 0; c < w; c++ )
    {
      mask = 0x80 >> ( c % 8 );
      if( magimg[w * r + c] > sw.canny_threshold )
        *( p + c / 8 ) &= ~mask;
    }
  }
}

static u8    *
crop_match_image( int w, int h, int lw, int lh, u8 *s, struct crop *bbox )
{
  u8           *match, *d;
  int           r;

  if( ( match = malloc( lw * lh ) ) == NULL )
    return NULL;
  d = match;
  s += w * bbox->top + bbox->left;
  for( r = bbox->top; r <= bbox->bottom; r++ )
  {
    memcpy( d, s, lw );
    s += w;
    d += lw;
  }
  return match;
}

u8           *
convolve_corners( int w, int h, int ow, int oh, int wc, int hc,
    struct crn *Corners, struct crop *Crop )
{
  int           sd, corner, wB, wr, hr, lw, lh, cutx, cuty;
  int           offx[SIDES], offy[SIDES];
  short        *magnitude;
  u8           *magimg, *img_match;
  enum direction dir;
  struct crn    Edge;
  struct crn   *Crn1, *Crn2;
  struct crop   bbox;

  cutx = cuty = 0;
  for( sd = 0; sd < SIDES; sd++ )
  {
    select_two_corners( sd, Corners, &Crn1, &Crn2, &dir );
    offx[sd] = offy[sd] = 0;
    if( convolve_two( wc, hc, Crn1, Crn2, dir, &offx[sd], &offy[sd] ) )
    {
      if( abs( offx[sd] ) > cutx )
        cutx = abs( offx[sd] );
      if( abs( offy[sd] ) > cuty )
        cuty = abs( offy[sd] );
#ifdef DEBUG
      printf( "dir %d offx %d offy %d\n", sd, offx[sd], offy[sd] );
#endif
    }
  }
#ifdef DEBUG
  for( corner = 0; corner < CORNERS; corner++ )
    if( Corners[corner].image )
      save_image( "corner%x", corner, 8, 1, wc, hc, Corners[corner].image );
#endif
  wr = wc - cutx;
  hr = hc - cuty;
  magnitude = ( short * ) calloc( wr * hr, sizeof( short ) );
  magimg = ( u8 * ) calloc( wr * hr, sizeof( u8 * ) );
  for( sd = 0; sd < SIDES; sd++ )
  {
    select_two_corners( sd, Corners, &Crn1, &Crn2, &dir );
    if( offx[sd] || offy[sd] )
      overlap( wc, hc, cutx, cuty, offx[sd], offy[sd],
          Crn1, Crn2, magnitude );
  }
  normalize( wr, hr, magnitude, magimg );
#ifdef DEBUG
  save_image( "corner_j", 0, 8, 1, wr, hr, magimg );
#endif
  wB = ( wr + 7 ) / 8;
  Edge.image = malloc( wB * hr );
  memset( Edge.image, 0xff, wB * hr );
  threshold_single_frame( wr, hr, magimg, Edge.image );
  bounding_box( wB * 8, hr, &Edge, &bbox );
  crop_overlayed_logo( wB * 8, hr, &lw, &lh, &Edge, &bbox );
  for( corner = 0; corner < CORNERS; corner++ )
    if( Corners[corner].image )
      delogo_box( corner, w, h, ow, oh, wc, hc, &Corners[corner],
          Crop, &bbox );
  img_match = crop_match_image( wr, hr, lw, lh, magimg, &bbox );
#ifdef DEBUG
  if( img_match )
    save_image( "match_j", 0, 8, 1, lw, lh, img_match );
#endif
  free( Edge.image );
  free( magimg );
  free( magnitude );
  return img_match;
}

static int
convolve_match( u8 *img_match, struct shot *Shot )
{
  return 1;
}

int
find_which_corner( u8 *img_match, struct crn *Corners, struct shot *Shot )
{
  int           corner, best_corner, mag, best_mag;
  struct crn   *Crn;

  best_corner = CORNERS;
  best_mag = 0;
  for( corner = 0; corner < CORNERS; corner++ )
  {
    Crn = &Corners[corner];
    if( Crn->image )
    {
      mag = convolve_match( img_match, Shot );
      if( mag > best_mag )
      {
        best_corner = corner;
        best_mag = mag;
      }
    }
  }
  if( best_corner != CORNERS && best_mag > 45 )
    Shot->corner = best_corner;
  return 0;
}
