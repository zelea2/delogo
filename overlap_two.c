#include "delogo.h"

static int
best_saved_area( u32 wb, u32 hb, u32 *L, u32 *R, u32 *T, u32 *B,
    struct crop *bbox1, struct crop *bbox2 )
{
  enum sides    fixed_side, best_side;
  u32           r, c;
  u32           area, saved_area[SIDES];
  u32           b1l, b1r, b1t, b1b, b2l, b2r, b2t, b2b;
  u32           r1l[SIDES], r1r[SIDES], r1t[SIDES], r1b[SIDES];
  u32           r2l[SIDES], r2r[SIDES], r2t[SIDES], r2b[SIDES];

  // case LEFT:
  saved_area[LEFT] = 0;
  b1l = r1l[LEFT] = 0;
  b1t = r1t[LEFT] = 0;
  b1b = r1b[LEFT] = hb;
  b2l = r2l[LEFT] = 0;
  b2r = r2r[LEFT] = wb;
  for( b1r = 0; b1r <= wb - 1; b1r++ )
  {
    for( b2t = 0; b2t <= hb; b2t++ )
    {
      for( b2b = hb; b2b > b2t; b2b-- )
      {
        for( r = b2b; r <= hb; r++ )
          if( wb - b1r >= R[r] )
            goto left_side_next;
        for( r = 0; r <= b2t; r++ )
          if( wb - b1r >= R[r] )
            goto left_side_next;
        area = ( wb - b1r ) * ( b2t - 0 + hb - b2b );
        if( area > saved_area[LEFT] )
        {
          saved_area[LEFT] = area;
          r1r[LEFT] = b1r;
          r2t[LEFT] = b2t;
          r2b[LEFT] = b2b;
        }
      left_side_next:
      }
    }
  }
  // case RIGHT:
  saved_area[RIGHT] = 0;
  b1r = r1r[RIGHT] = wb;
  b1t = r1t[RIGHT] = 0;
  b1b = r1b[RIGHT] = hb;
  b2r = r2r[RIGHT] = wb;
  b2l = r2l[RIGHT] = 0;
  for( b1l = 0; b1l < wb; b1l++ )
  {
    for( b2t = 0; b2t <= hb; b2t++ )
    {
      for( b2b = hb; b2b > b2t; b2b-- )
      {
        for( r = b2b; r <= hb; r++ )
          if( b1l >= L[r] )
            goto right_side_next;
        for( r = 0; r <= b2t; r++ )
          if( b1l >= L[r] )
            goto right_side_next;
        area = ( b1l - 0 ) * ( b2t - 0 + hb - b2b );
        if( area > saved_area[RIGHT] )
        {
          saved_area[RIGHT] = area;
          r1l[RIGHT] = b1l;
          r2t[RIGHT] = b2t;
          r2b[RIGHT] = b2b;
        }
      right_side_next:
      }
    }
  }
  // case TOP:
  saved_area[TOP] = 0;
  b1t = r1t[TOP] = 0;
  b1l = r1l[TOP] = 0;
  b1r = r1r[TOP] = wb;
  b2t = r2t[TOP] = 0;
  b2b = r2b[TOP] = hb;
  for( b1b = 0; b1b <= hb - 1; b1b++ )
  {
    for( b2l = 0; b2l <= wb; b2l++ )
    {
      for( b2r = wb; b2r > b2l; b2r-- )
      {
        for( c = b2r; c <= wb; c++ )
          if( hb - b1b >= B[c] )
            goto top_side_next;
        for( c = 0; c <= b2l; c++ )
          if( hb - b1b >= B[c] )
            goto top_side_next;
        area = ( hb - b1b ) * ( b2l - 0 + wb - b2r );
        if( area > saved_area[TOP] )
        {
          saved_area[TOP] = area;
          r1b[TOP] = b1b;
          r2l[TOP] = b2l;
          r2r[TOP] = b2r;
        }
      top_side_next:
      }
    }
  }
  // case BOTTOM:
  saved_area[BOTTOM] = 0;
  b1b = r1b[BOTTOM] = hb;
  b1l = r1l[BOTTOM] = 0;
  b1r = r1r[BOTTOM] = wb;
  b2b = r2b[BOTTOM] = hb;
  b2t = r2t[BOTTOM] = 0;
  for( b1t = 0; b1t <= hb; b1t++ )
  {
    for( b2l = 0; b2l <= wb; b2l++ )
    {
      for( b2r = wb; b2r > b2l; b2r-- )
      {
        for( c = b2r; c <= wb; c++ )
          if( b1t >= T[c] )
            goto bottom_side_next;
        for( c = 0; c <= b2l; c++ )
          if( b1t >= T[c] )
            goto bottom_side_next;
        area = ( b1t - 0 ) * ( b2l - 0 + wb - b2r );
        if( area > saved_area[BOTTOM] )
        {
          saved_area[BOTTOM] = area;
          r1t[BOTTOM] = b1t;
          r2l[BOTTOM] = b2l;
          r2r[BOTTOM] = b2r;
        }
      bottom_side_next:
      }
    }
  }
  area = 0;
  best_side = SIDES;
  for( fixed_side = LEFT; fixed_side <= BOTTOM; fixed_side++ )
  {
    if( area < saved_area[fixed_side] )
    {
      area = saved_area[fixed_side];
      best_side = fixed_side;
    }
  }
  if( best_side == SIDES )
    return -1;
  // dischard savings less than 12.5%
  if( saved_area[best_side] < wb * hb / 8 )
    return -1;
  bbox1->left = r1l[best_side];
  bbox1->right = r1r[best_side];
  bbox1->top = r1t[best_side];
  bbox1->bottom = r1b[best_side];
  bbox2->left = r2l[best_side];
  bbox2->right = r2r[best_side];
  bbox2->top = r2t[best_side];
  bbox2->bottom = r2b[best_side];
  return 0;
}

int
overlap_tworect( u8 *C, unsigned wc, unsigned hc,
    struct crop *bbox, struct crop *bbox1, struct crop *bbox2 )
{
  u32           r, c, wb, hb;
  u32          *L, *R, *T, *B;
  u8            mask;
  u8           *s;
  int           ret = 0;

  wb = bbox->right - bbox->left;
  hb = bbox->bottom - bbox->top;
  L = malloc( ( hb + 1 ) * sizeof( u32 ) );
  R = malloc( ( hb + 1 ) * sizeof( u32 ) );
  T = malloc( ( wb + 1 ) * sizeof( u32 ) );
  B = malloc( ( wb + 1 ) * sizeof( u32 ) );
  mask = 0;
  for( r = 0; r <= hb; r++ )
  {
    // find left-most pixels for all rows
    s = C + ( bbox->top + r ) * wc / 8 + bbox->left / 8;
    for( c = 0; c <= wb; c++ )
    {
      mask = 0x80 >> ( ( bbox->left + c ) & 7 );
      if( !( *s & mask ) )      // pixel
        break;
      if( ( ( bbox->left + c ) & 7 ) == 7 )
        s++;
    }
    L[r] = c;

    // find right-most pixels for all rows
    s = C + ( bbox->top + r ) * wc / 8 + bbox->right / 8;
    for( c = 0; c <= wb; c++ )
    {
      mask = 1 << ( ( 7 - bbox->right + c ) & 7 );
      if( !( *s & mask ) )      // pixel
        break;
      if( ( ( 7 - bbox->right + c ) & 7 ) == 7 )
        s--;
    }
    R[r] = c;
    // printf( "L %3d R %3d\n", L[r], R[r] );
  }
  for( c = 0; c <= wb; c++ )
  {
    // find top-most pixels for all colums
    mask = 0x80 >> ( ( bbox->left + c ) & 7 );
    s = C + bbox->top * wc / 8 + ( bbox->left + c ) / 8;
    for( r = 0; r <= hb; r++ )
    {
      if( !( *s & mask ) )      // pixel
        break;
      s += wc / 8;
    }
    T[c] = r;
    // find bottom-most pixels for all colums
    s = C + bbox->bottom * wc / 8 + ( bbox->left + c ) / 8;
    for( r = 0; r <= hb; r++ )
    {
      if( !( *s & mask ) )      // pixel
        break;
      s -= wc / 8;
    }
    B[c] = r;
    // printf( "T %3d B %3d\n", U[c], D[c] );
  }
  if( ( ret = best_saved_area( wb, hb, L, R, T, B, bbox1, bbox2 ) ) == 0 )
  {
    bbox1->left += bbox->left - sw.border_band;
    bbox2->left += bbox->left - sw.border_band;
    bbox1->right += bbox->left + sw.border_band;
    bbox2->right += bbox->left + sw.border_band;
    bbox1->top += bbox->top - sw.border_band;
    bbox2->top += bbox->top - sw.border_band;
    bbox1->bottom += bbox->top + sw.border_band;
    bbox2->bottom += bbox->top + sw.border_band;
    if( bbox1->left < bbox->left )
      bbox1->left = bbox->left;
    if( bbox2->left < bbox->left )
      bbox2->left = bbox->left;
    if( bbox1->right > bbox->right )
      bbox1->right = bbox->right;
    if( bbox2->right > bbox->right )
      bbox2->right = bbox->right;
    if( bbox1->top < bbox->top )
      bbox1->top = bbox->top;
    if( bbox2->top < bbox->top )
      bbox2->top = bbox->top;
    if( bbox1->bottom > bbox->bottom )
      bbox1->bottom = bbox->bottom;
    if( bbox2->bottom > bbox->bottom )
      bbox2->bottom = bbox->bottom;
#if 0
    printf( "b1l = %d b1r = %d b1t = %d b1b = %d\n",
        bbox1->left, bbox1->right, bbox1->top, bbox1->bottom );
    printf( "b2l = %d b2r = %d b2t = %d b2b = %d\n",
        bbox2->left, bbox2->right, bbox2->top, bbox2->bottom );
#endif
  }
  free( B );
  free( T );
  free( R );
  free( L );
  if( ret < 0 )
    sw.tworect = 0;
  return ret;
}
