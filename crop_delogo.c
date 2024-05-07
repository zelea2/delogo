#include "delogo.h"

static u8     zeroes[256] = {
  8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4, 7, 6, 6, 5, 6, 5, 5, 4, 6,
  5, 5, 4, 5, 4, 4, 3, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 6,
  5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 7, 6, 6, 5, 6, 5, 5, 4, 6,
  5, 5, 4, 5, 4, 4, 3, 6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 6,
  5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 4,
  3, 3, 2, 3, 2, 2, 1, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 6,
  5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 6, 5, 5, 4, 5, 4, 4, 3, 5,
  4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1, 6,
  5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 4,
  3, 3, 2, 3, 2, 2, 1, 5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1, 4,
  3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0
};

static int
evaluate_corner( u32 wc, u32 hc, u8 *p )
{
  unsigned int  i, cnt;

  for( cnt = i = 0; i < hc * wc / 8; i++ )
    cnt += zeroes[*p++];
  // printf( "Count %4d\n", cnt );
  // sanity check for logo size
  if( cnt > hc * wc / 150 && cnt < hc * wc * 5 / 4 )
    return cnt;
  else
    return 0;
}

static int
hratio( u32 wc, u32 hc, unsigned int divider, u8 *p )
{
  int           r, c, divcol, cntl = 0, cntr = 0;
  u8            b, mask, divmask;

  divcol = divider / 8;
  divmask = 0x80 >> ( divider % 8 );
  for( r = 0; r < hc; r++ )
    for( c = 0; c < wc / 8; c++ )
    {
      if( ( b = *p++ ) == 0xff )
        continue;
      if( c != divcol )
      {
        if( c < divcol )
          cntl += zeroes[b];
        else
          cntr += zeroes[b];
      }
      else                      // divider in the middle of the byte
      {
        for( mask = 0x80; mask; mask >>= 1 )
          if( !( b & mask ) )
          {
            if( mask > divmask )
              cntl++;
            else
              cntr++;
          }
      }
    }
  return 1000 * cntl / ( cntl + cntr );
}

static int
vratio( u32 wc, u32 hc, int divider, u8 *p )
{
  int           r, c, cntt = 0, cntb = 0;

  for( r = 0; r < divider; r++ )
    for( c = 0; c < wc / 8; c++ )
      cntt += zeroes[*p++];
  for( ; r < hc; r++ )
    for( c = 0; c < wc / 8; c++ )
      cntb += zeroes[*p++];
  return 1000 * cntt / ( cntt + cntb );
}

static void
bbox_sides( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  int           pd, divider, lmin, lmax, ratio;

  // LEFT side
  pd = 0, lmin = 0, lmax = wc - 1;
  while( 1 )
  {
    divider = ( lmin + lmax ) / 2;
    ratio = hratio( wc, hc, divider, corner );
    if( ratio <= 250 )          // 1:3 ratio
      lmin = divider;
    else
      lmax = divider;
    if( pd == divider )
      break;
    pd = divider;
  }
  box->left = divider;
  // RIGHT side
  pd = 0, lmin = 0, lmax = wc - 1;
  while( 1 )
  {
    divider = ( lmin + lmax ) / 2;
    ratio = hratio( wc, hc, divider, corner );
    if( ratio >= 750 )          // 3:1 ratio
      lmax = divider;
    else
      lmin = divider;
    if( pd == divider )
      break;
    pd = divider;
  }
  box->right = divider;
  // TOP side
  pd = 0, lmin = 0, lmax = hc - 1;
  while( 1 )
  {
    divider = ( lmin + lmax ) / 2;
    ratio = vratio( wc, hc, divider, corner );
    if( ratio <= 250 )          // 1:3 ratio
      lmin = divider;
    else
      lmax = divider;
    if( pd == divider )
      break;
    pd = divider;
  }
  box->top = divider;
  // BOTTOM side
  pd = 0, lmin = 0, lmax = hc - 1;
  while( 1 )
  {
    divider = ( lmin + lmax ) / 2;
    ratio = vratio( wc, hc, divider, corner );
    if( ratio >= 750 )          // 3:1 ratio
      lmax = divider;
    else
      lmin = divider;
    if( pd == divider )
      break;
    pd = divider;
  }
  box->bottom = divider;
}

static int
adjust_left( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  u8           *p, b, mask;
  u32           r, c;
  int           left = box->left - sw.border_black;

  if( left < 0 )
    left = 0;
  if( left == box->left )
    return 0;
  for( r = box->top; r <= box->bottom; r++ )
    for( c = left; c < box->left; c++ )
    {
      b = corner[( r * wc + c ) / 8];
      mask = 0x80 >> ( c % 8 );
      if( !( b & mask ) )
      {
        box->left--;
        return 1;
      }
    }
  p = corner + ( box->top * wc + box->left ) / 8;
  mask = 0x80 >> ( box->left % 8 );
  for( r = box->top; r <= box->bottom; r++ )
  {
    if( !( *p & mask ) )
      break;
    p += wc / 8;
  }
  if( r > box->bottom )
  {
    box->left++;
    return 1;
  }
  return 0;
}

static int
adjust_right( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  u8           *p, b, mask;
  u32           r, c;
  int           right = box->right + sw.border_black;

  if( right >= wc )
    right = wc - 1;
  if( right == box->right )
    return 0;
  for( r = box->top; r <= box->bottom; r++ )
    for( c = box->right + 1; c <= right; c++ )
    {
      b = corner[( r * wc + c ) / 8];
      mask = 0x80 >> ( c % 8 );
      if( !( b & mask ) )
      {
        box->right++;
        return 1;
      }
    }
  p = corner + ( box->top * wc + box->right ) / 8;
  mask = 0x80 >> ( box->right % 8 );
  for( r = box->top; r <= box->bottom; r++ )
  {
    if( !( *p & mask ) )
      break;
    p += wc / 8;
  }
  if( r > box->bottom )
  {
    box->right--;
    return 1;
  }
  return 0;
}

static int
adjust_top( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  u8            b, mask;
  u32           r, c;
  int           top = box->top - sw.border_black;

  if( top < 0 )
    top = 0;
  if( top == box->top )
    return 0;
  for( r = top; r < box->top; r++ )
    for( c = box->left; c <= box->right; c++ )
    {
      b = corner[( r * wc + c ) / 8];
      mask = 0x80 >> ( c % 8 );
      if( !( b & mask ) )
      {
        box->top--;
        return 1;
      }
    }
  for( c = box->left; c <= box->right; c++ )
  {
    b = corner[( box->top * wc + c ) / 8];
    mask = 0x80 >> ( c % 8 );
    if( !( b & mask ) )
      break;
  }
  if( c > box->right )
  {
    box->top++;
    return 1;
  }
  return 0;
}

static int
adjust_bottom( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  u8            b, mask;
  u32           r, c;
  int           bottom = box->bottom + sw.border_black;

  if( bottom >= hc )
    bottom = hc - 1;
  if( bottom == box->bottom )
    return 0;
  for( r = box->bottom + 1; r <= bottom; r++ )
    for( c = box->left; c <= box->right; c++ )
    {
      b = corner[( r * wc + c ) / 8];
      mask = 0x80 >> ( c % 8 );
      if( !( b & mask ) )
      {
        box->bottom++;
        return 1;
      }
    }
  for( c = box->left; c <= box->right; c++ )
  {
    b = corner[( box->bottom * wc + c ) / 8];
    mask = 0x80 >> ( c % 8 );
    if( !( b & mask ) )
      break;
  }
  if( c > box->right )
  {
    box->bottom--;
    return 1;
  }
  return 0;
}

static int
join_pixels_hv( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  int           r, c, s, e, ret, pixcount, pixbbox;
  u8            mask, *logomask;

  ret = 0;
  logomask = malloc( hc * wc / 8 );
  memcpy( logomask, corner, hc * wc / 8 );
  pixcount = 0;
  pixbbox = ( box->right - box->left + 1 ) * ( box->bottom - box->top + 1 );
  // Horizontal
  for( r = box->top; r <= box->bottom; r++ )
  {
    s = e = -1;
    for( c = box->left; c <= box->right; c++ )
    {
      mask = 0x80 >> ( c % 8 );
      if( ( logomask[( r * wc + c ) / 8] & mask ) == 0 )
      {
        e = c;
        if( s == -1 )
          s = c;
      }
    }
    if( s >= 0 && e >= 0 && s < e )
    {
      for( c = s + 1; c < e; c++ )      // unite the horizontal line end
        // pixels
      {
        mask = 0x80 >> ( c % 8 );
        logomask[( r * wc + c ) / 8] &= ~mask;
      }
    }
  }
  // Vertical
  for( c = box->left; c <= box->right; c++ )
  {
    s = e = -1;
    mask = 0x80 >> ( c % 8 );
    for( r = box->top; r <= box->bottom; r++ )
    {
      if( ( logomask[( r * wc + c ) / 8] & mask ) == 0 )
      {
        e = r;
        if( s == -1 )
          s = r;
      }
    }
    if( s >= 0 && e >= 0 && s < e )
    {
      pixcount += e - s + 1;
      for( r = s + 1; r < e; r++ )      // unite the vertical line end pixels
        logomask[( r * wc + c ) / 8] &= ~mask;
    }
  }
  // printf( "Pixels lit %d%%\n", 100 * pixcount / pixbbox );
  if( sw.logomask || 100 * pixcount / pixbbox < sw.pixel_ratio )
  {
    memcpy( corner, logomask, hc * wc / 8 );
    sw.logomask = 1;
    ret = 1;
  }
  free( logomask );
  return ret;
}

static void
clean_pixels_outside_bbox( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  int           r, c;
  u8            mask;

  for( r = 0; r < box->top; r++ )
    memset( corner + r * wc / 8, 0xff, wc / 8 );
  for( r = box->top; r <= box->bottom; r++ )
  {
    for( c = 0; c < wc; c += 8 )
    {
      mask = 0;
      if( c + 7 < box->left || c > box->right )
        mask = 0xff;
      else if( box->left - c > 0 && box->left - c <= 7 )        // border
        // case left
        mask = 0xff << ( 8 - ( box->left - c ) );
      else if( box->right - c >= 0 && box->right - c < 7 )      // border
        // case right
        mask = 0xff >> ( box->right - c + 1 );
      corner[( r * wc + c ) / 8] |= mask;
    }
  }
  for( r = box->bottom + 1; r < hc; r++ )
    memset( corner + r * wc / 8, 0xff, wc / 8 );
}

static int
adjust_bbox( u32 wc, u32 hc, u8 *corner, struct crop *box )
{
  int           i, adjust;

  do
  {
    adjust = 0;
    if( adjust_left( wc, hc, corner, box ) )
      adjust = 1;
    if( adjust_right( wc, hc, corner, box ) )
      adjust = 1;
    if( adjust_top( wc, hc, corner, box ) )
      adjust = 1;
    if( adjust_bottom( wc, hc, corner, box ) )
      adjust = 1;
  }
  while( adjust );
  // minimum box sizes for logo
  if( box->right - box->left < 5 || box->bottom - box->top < 5 )
    return 0;
// printf( "Box %d:%d %d:%d\n", box->left, box->right, box->top, box->bottom );
  for( i = 0; i < sw.border_band; i++ )
  {
    if( box->left > 0 )
      box->left--;
    if( box->right < wc - 2 )
      box->right++;
    if( box->top > 0 )
      box->top--;
    if( box->bottom < hc - 2 )
      box->bottom++;
  }
  clean_pixels_outside_bbox( wc, hc, corner, box );
  join_pixels_hv( wc, hc, corner, box );
  return 1;
}

int
bounding_box( u32 wc, u32 hc, struct crn *Crn, struct crop *box )
{
  // first get the bbox so that each line 
  // divides the number of pixels in a 1:3 ratio
  // to get roughly the center of the bounding box
  bbox_sides( wc, hc, Crn->image, box );
  // adjust bbox to enclose it in a black rectangle
  if( !adjust_bbox( wc, hc, Crn->image, box ) )
    return 0;
#if 0
  printf( "BBox: %d-%d,%d-%d\n",
      box->left, box->right, box->top, box->bottom );
#endif
  return 1;
}

static int
clean_rows( u32 wc, u8 *p )
{
  int           i, j, dots;

  dots = 0;
  for( i = 0; i < wc / 8; i++ )
  {
    if( *p == 0 )
      dots += 8;
    else
    {
      for( j = 0; j < 8; j++ )
        if( ( *p & ( 1 << j ) ) == 0 )
          dots++;
    }
    p++;
  }
  if( dots > wc * 9 / 10 )
  {
    p -= wc / 8;
    for( i = 0; i < wc / 8; i++ )
      *p++ = 0xff;
    return 1;
  }
  return 0;
}

static int
clean_columns( u32 wc, u32 hc, u8 mask, u8 *p )
{
  int           i, dots;

  dots = 0;
  for( i = 0; i < hc; i++ )
  {
    if( ( *p & mask ) == 0 )
      dots++;
    p += wc / 8;
  }
  if( dots > hc * 9 / 10 )
  {
    p -= hc * wc / 8;
    for( i = 0; i < hc; i++ )
    {
      *p |= mask;
      p += wc / 8;
    }
    return 1;
  }
  return 0;
}

static int
clean_edges( u32 wc, u32 hc, u8 *b )
{
  int           ret, r, c;

  ret = 0;
  for( r = 0; r < hc / 20; r++ )
  {
    // top rows
    ret += clean_rows( wc, b + r * wc / 8 );
    // bottom rows
    ret += clean_rows( wc, b + ( hc - r - 1 ) * wc / 8 );
  }
  for( c = 0; c < wc / 20; c++ )
  {
    // left colums
    ret += clean_columns( wc, hc, 0x80 >> ( c & 7 ), b + c / 8 );
    // right colums
    ret += clean_columns( wc, hc, 0x80 >> ( ( wc - c - 1 ) & 7 ),
        b + ( wc - c - 1 ) / 8 );
  }
  return ret;
}

static int
ascend_int( const void *a, const void *b )
{
  return ( *( int * ) a < *( int * ) b );
}

static void
crop_frames( int w, int h, int ow, int oh, struct shot *Shots,
    struct crop *Crop )
{
  struct crop   Crops[sw.smpl_frames];
  int           limit[4][sw.smpl_frames];
  int           sh;
  int           line, column;
  int           non_black;
  u32           r, c;
  u8           *p;

  for( sh = 0; sh < sw.smpl_frames; sh++ )
  {
    // find top
    line = h / 2;
    Crops[sh].top = 0;
    while( line >= 0 )
    {
      p = Shots[sh].image + line * w;
      for( non_black = c = 0; c < w; c++ )
      {
        if( *p > sw.level_black )
        {
          if( !sw.aggressive_crop )
            break;
          non_black++;
        }
        p++;
      }
      if( c == w && 100 * non_black / w < sw.aggressive_crop )  // black line
      {
        Crops[sh].top = line + 1;
        break;
      }
      line--;
    }
    // find bottom
    line = h / 2 + 1;
    Crops[sh].bottom = h;
    while( line < h )
    {
      p = Shots[sh].image + line * w;
      for( non_black = c = 0; c < w; c++ )
      {
        if( *p > sw.level_black )
        {
          if( !sw.aggressive_crop )
            break;
          non_black++;
        }
        p++;
      }
      if( c == w && 100 * non_black / w < sw.aggressive_crop )  // black line
      {
        Crops[sh].bottom = line;
        break;
      }
      line++;
    }
    // find left
    column = w / 2;
    Crops[sh].left = 0;
    while( column >= 0 )
    {
      p = Shots[sh].image + column;
      for( non_black = r = 0; r < h; r++ )
      {
        if( *p > sw.level_black )
        {
          if( !sw.aggressive_crop )
            break;
          non_black++;
        }
        p += w;
      }
      if( r == h && 100 * non_black / h < sw.aggressive_crop )  // black column
      {
        Crops[sh].left = column + 1;
        break;
      }
      column--;
    }
    // find right
    column = w / 2 + 1;
    Crops[sh].right = w;
    while( column < w )
    {
      p = Shots[sh].image + column;
      for( non_black = r = 0; r < h; r++ )
      {
        if( *p > sw.level_black )
        {
          if( !sw.aggressive_crop )
            break;
          non_black++;
        }
        p += w;
      }
      if( r == h && 100 * non_black / h < sw.aggressive_crop )  // black column
      {
        Crops[sh].right = column;
        break;
      }
      column++;
    }
  }
  for( sh = 0; sh < sw.smpl_frames; sh++ )
  {
    limit[0][sh] = Crops[sh].top;
    limit[1][sh] = Crops[sh].bottom;
    limit[2][sh] = Crops[sh].left;
    limit[3][sh] = Crops[sh].right;
  }
  qsort( limit[0], sw.smpl_frames, sizeof( int ), ascend_int );
  qsort( limit[1], sw.smpl_frames, sizeof( int ), ascend_int );
  qsort( limit[2], sw.smpl_frames, sizeof( int ), ascend_int );
  qsort( limit[3], sw.smpl_frames, sizeof( int ), ascend_int );
  // crop is valid if 2/3 of the frames are inside the region 
  sh = sw.smpl_frames / 3;
  Crop->top = limit[0][sh];
  Crop->left = limit[2][sh];
  sh = 2 * sw.smpl_frames / 3;
  Crop->bottom = limit[1][sh];
  Crop->right = limit[3][sh];
  // make w and h divisible by 2
  if( ( Crop->right - Crop->left ) & 1 )
    Crop->right--;
  if( ( Crop->bottom - Crop->top ) & 1 )
    Crop->bottom--;
  if( Crop->left != 0 || Crop->right != w ||
      Crop->top != 0 || Crop->bottom != h )
    printf( "crop=%d:%d:%d:%d\n",
        Crop->right - Crop->left + ow - w,
        Crop->bottom - Crop->top + oh - h, Crop->left, Crop->top );
}

static int
corner_offset( int w, int h, u32 wc, u32 hc, struct crop *Crop, int corner )
{
  int           offs = 0;

  switch ( corner )
  {
    case NW:
      offs = Crop->top * w + Crop->left;
      break;
    case NE:
      offs = Crop->top * w + Crop->right - wc;
      break;
    case SW:
      offs = ( Crop->bottom - hc ) * w + Crop->left;
      break;
    case SE:
      offs = ( Crop->bottom - hc ) * w + Crop->right - wc;
      break;
  }
  return offs;
}

static int
distance_logo( int w, int h, u32 wc, u32 hc, unsigned offs,
    struct shot *Shots, struct crn *Crn, int corner )
{
  int           si, sj, ldiff;
  int           offd, off;
  u32           r, c;
  u8            mask;
  u8           *p, *q;

  for( si = 0; si < sw.smpl_frames - 1; si++ )
  {
    for( sj = si + 1; sj < sw.smpl_frames; sj++ )
    {
      for( off = offs, r = 0; r < hc; r++, off += w )
      {
        p = Shots[si].image + off;
        q = Shots[sj].image + off;
        for( c = 0; c < wc; c++, p++, q++ )
        {
          offd = ( r * wc + c ) / 8;
          mask = 0x80 >> ( c & 7 );
          if( ( *( Crn->image + offd ) & mask ) == 0 )
          {
            ldiff = *p - *q;
            if( ldiff > sw.level_delta || ldiff < -sw.level_delta )
              *( Crn->image + offd ) |= mask;
          }
        }
      }
    }
  }
  return 0;
}

void
delogo_box( int corner, int w, int h, int ow, int oh, int wc, int hc,
    struct crn *Crn, struct crop *Crop, struct crop *box )
{
  int           lx, ly, lw, lh;

  // printf( "WH %d %d OWH %d %d WHC %d %d\n", w, h, ow, oh, wc, hc );
  // printf( "Crop %d %d - %d %d\n", Crop->left, Crop->right, Crop->top, Crop->bottom );
  lx = ly = 0;
  lw = box->right - box->left + 1;
  lh = box->bottom - box->top + 1;
  switch ( corner )
  {
    case NW:
      lx = Crop->left + box->left;
      ly = Crop->top + box->top;
      break;
    case NE:
      lx = Crop->right - wc + box->left;
      ly = Crop->top + box->top;
      break;
    case SW:
      lx = Crop->left + box->left;
      ly = Crop->bottom - hc + box->top;
      break;
    case SE:
      lx = Crop->right - wc + box->left;
      ly = Crop->bottom - hc + box->top;
      break;
  }
  lx += Crn->offx;
  ly += Crn->offy;
  if( lx <= 0 )
    lx = 1;
  if( ly <= 0 )
    ly = 1;
  if( lx + lw > w )
    lw = w - lx;
  if( ly + lh > h )
    lh = h - ly;
  switch ( corner )
  {
    case NE:
      lx += ow - w;
      break;
    case SW:
      ly += oh - h;
      break;
    case SE:
      lx += ow - w;
      ly += oh - h;
      break;
  }
  Crn->lx = lx;
  Crn->ly = ly;
  Crn->lw = lw;
  Crn->lh = lh;
  printf( "delogo=x=%d:y=%d:w=%d:h=%d corner %d\n", lx, ly, lw, lh, corner );
}

static char  *
cr_str( int corner )
{
  switch ( corner )
  {
    case NW:
      return "NW";
    case NE:
      return "NE";
    case SW:
      return "SW";
    case SE:
      return "SE";
  }
  return "XX";
}

static void
crop_corner( int w, int h, int ow, int oh, unsigned wc, unsigned hc,
    struct crn *Crn, struct crop *bbox, struct crop *Crop, int corner )
{
  int           r, c, len;
  u8           *img_bbox, *img_mask, *s, *d0, *d3, mask;
  unsigned      r_off, c_off;
  struct crop   bbox1, bbox2;

  memset( &bbox1, 0, sizeof( struct crop ) );
  memset( &bbox2, 0, sizeof( struct crop ) );
  if( sw.logomask )
  {
    r_off = c_off = 0;
    switch ( corner )
    {
      case NW:
        r_off = Crop->top;
        c_off = Crop->left;
        break;
      case NE:
        r_off = Crop->top;
        c_off = Crop->right - wc;
        break;
      case SW:
        r_off = Crop->bottom - hc;
        c_off = Crop->left;
        break;
      case SE:
        r_off = Crop->bottom - hc;
        c_off = Crop->right - wc;
        break;
    }
    len = w * h * 4 / 8;
    if( ( img_mask = malloc( len ) ) == NULL )
      return;
    memset( img_mask, 0xff, len );
    for( r = bbox->top; r <= bbox->bottom; r++ )
    {
      // plane 0 && 3 (alpha)
      d0 = img_mask + ( ( ( r + r_off ) * 4 + 0 ) * w + c_off ) / 8;
      d3 = img_mask + ( ( ( r + r_off ) * 4 + 3 ) * w + c_off ) / 8;
      s = Crn->image + r * wc / 8;
      memcpy( d0, s, wc / 8 );
      memcpy( d3, s, wc / 8 );
    }
    save_image( "removelogo_%d", corner, 1, 4, w, h, img_mask );
    free( img_mask );
    printf( "removelogo=removelogo_%d.%s\n", corner, sw.pcx ? "pcx" : "png" );
  }
  else
  {
    if( sw.tworect )
    {
      overlap_tworect( Crn->image, wc, hc, bbox, &bbox1, &bbox2 );
    }
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
      if( sw.tworect )
      {
        if( r >= bbox1.top && r <= bbox1.bottom )
          for( c = bbox1.left; c <= bbox1.right; c++ )
          {
            mask = 0x80 >> ( c & 7 );
            *( d3 + c / 8 ) &= ~mask;
          }
        if( r >= bbox2.top && r <= bbox2.bottom )
          for( c = bbox2.left; c <= bbox2.right; c++ )
          {
            mask = 0x80 >> ( c & 7 );
            *( d3 + c / 8 ) &= ~mask;
          }
      }
      else
      {
        for( c = bbox->left; c <= bbox->right; c++ )
        {
          mask = 0x80 >> ( c & 7 );
          *( d3 + c / 8 ) &= ~mask;
        }
      }
    }
    save_image( "delogo_%d", corner, 1, 4, wc, hc, img_bbox );
    free( img_bbox );
    if( sw.tworect )
    {
      delogo_box( corner, w, h, ow, oh, wc, hc, Crn, Crop, &bbox1 );
      delogo_box( corner, w, h, ow, oh, wc, hc, Crn, Crop, &bbox2 );
    }
    else
      delogo_box( corner, w, h, ow, oh, wc, hc, Crn, Crop, bbox );
  }
}

void
crop_delogo( int w, int h, int ow, int oh, struct shot *Shots )
{
  struct crn    Corner[CORNERS];
  struct crop   Crop, bbox[CORNERS];
  u8            valid_corners;
  u8           *img_match;
  int           cp, sh, maxcnt, maxcp;
  u32           wc, hc, offset;
  int           ( *logo ) ( int w, int h, u32 wc,
      u32 hc, unsigned offs, struct shot * Shots, struct crn * Crn,
      int corner );

  crop_frames( w, h, ow, oh, Shots, &Crop );
  wc = ( Crop.right - Crop.left ) / 2;
  wc = ( wc + 7 ) / 8 * 8;      // round to byte
  hc = ( Crop.bottom - Crop.top ) / 2;
  logo = sw.canny ? canny_logo : distance_logo;
  maxcp = -1;
  memset( Corner, 0, CORNERS * sizeof( struct crn ) );
  if( !( sw.corner_mask & 0x80 ) )
    for( maxcnt = cp = 0; cp < CORNERS; cp++ )
    {
      if( !( sw.corner_mask & 1 << cp ) )
        continue;
      Corner[cp].image = calloc( sw.jump_corner ? hc * wc : hc * wc / 8, 1 );
      offset = corner_offset( w, h, wc, hc, &Crop, cp );
      if( logo( w, h, wc, hc, offset, Shots, &Corner[cp], cp ) != 0 )   // error
        return;
      if( sw.jump_corner )
        continue;
      clean_edges( wc, hc, Corner[cp].image );
      Corner[cp].count = evaluate_corner( wc, hc, Corner[cp].image );
      if( Corner[cp].count > maxcnt )
      {
        maxcnt = Corner[cp].count;
        maxcp = cp;
      }
    }
  if( sw.jump_corner )
  {
    img_match = convolve_corners( w, h, ow, oh, wc, hc, Corner, &Crop );
    if( img_match )
    {
      // find corner match in each frame
      for( sh = 0; sh < sw.smpl_frames; sh++ )
        find_which_corner( img_match, Corner, &Shots[sh] );
      free( img_match );
    }
  }
  else
  {
    valid_corners = 0;
    for( cp = 0; cp < CORNERS; cp++ )
    {
      if( !Corner[cp].count )
        continue;
      if( !sw.jump_corner && cp != maxcp )
        continue;
      valid_corners |= bounding_box( wc, hc, &Corner[cp], &bbox[cp] ) << cp;
    }
    for( cp = 0; cp < CORNERS; cp++ )
      if( valid_corners & 1 << cp )
        crop_corner( w, h, ow, oh, wc, hc, &Corner[cp], &bbox[cp], &Crop,
            cp );
  }
  for( cp = 0; cp < CORNERS; cp++ )
    if( Corner[cp].image )
    {
      free( Corner[cp].image );
      Corner[cp].image = NULL;
    }
}
