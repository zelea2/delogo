#include "delogo.h"
#include "spng.h"

static void
pack_putc( char c, FILE *f )
{
  fwrite( &c, sizeof( char ), 1, f );
}

static void
pack_iputw( short i, FILE *f )
{
  fwrite( &i, sizeof( short ), 1, f );
}

static void
pack_area( u8 *data, int len, FILE *f )
{
  fwrite( data, 1, len, f );
}

int
save_pcx_image( FILE *f, int bpp, int planes, int w, int h, u8 *image )
{
  int           c;
  int           x, y, hlimit;
  int           runcount;
  char          runchar;
  char          ch;
  u8            data[3 * 256];
  u8           *p;

  hlimit = ( w * bpp + 7 ) / 8;
  pack_putc( 10, f );           // manufacturer 
  pack_putc( 5, f );            // version 
  pack_putc( 1, f );            // run length encoding 
  pack_putc( bpp, f );          // bits per pixel 
  pack_iputw( 0, f );           // xmin 
  pack_iputw( 0, f );           // ymin 
  pack_iputw( w - 1, f );       // xmax 
  pack_iputw( h - 1, f );       // ymax 
  pack_iputw( 320, f );         // HDpi 
  pack_iputw( 200, f );         // VDpi 
  memset( data, 0, 48 );        // all colors black except
  p = data + 6 * 3;
  *p++ = 0xff;                  // 6 white
  *p++ = 0xff;
  *p++ = 0xff;
  *p++ = 0;                     // 7 dark green
  *p++ = 0x7c;
  *p++ = 0x3c;
  pack_area( data, 48, f );
  pack_putc( 0, f );            // reserved 
  pack_putc( planes, f );       // number of planes 
  pack_iputw( hlimit, f );      // number of bytes per scanline 
  pack_iputw( 1, f );           // color palette 
  pack_iputw( w, f );           // hscreen size 
  pack_iputw( h, f );           // vscreen size 
  memset( data, 0, 54 );        // filler
  pack_area( data, 54, f );

  for( y = 0; y < h; y++ )
  {                             // for each scanline... 
    runcount = 0;
    runchar = 0;
    for( c = 0; c < planes; c++ )
    {
      for( x = 0; x < hlimit; x++ )
      {                         // for each pixel... 
        ch = *image++;
        if( runcount == 0 )
        {
          runcount = 1;
          runchar = ch;
        }
        else
        {
          if( ( ch != runchar ) || ( runcount >= 0x3f ) )
          {
            if( ( runcount > 1 ) || ( ( runchar & 0xC0 ) == 0xC0 ) )
              pack_putc( 0xC0 | runcount, f );
            pack_putc( runchar, f );
            runcount = 1;
            runchar = ch;
          }
          else
            runcount++;
        }
      }
    }
    if( ( runcount > 1 ) || ( ( runchar & 0xC0 ) == 0xC0 ) )
      pack_putc( 0xC0 | runcount, f );
    pack_putc( runchar, f );
  }

  if( bpp * planes > 4 )
  {                             // 256 color palette 
    pack_putc( 12, f );

    for( p = data, c = 0; c < 256; c++ )
    {
      *p++ = c;
      *p++ = c;
      *p++ = c;
    }
    pack_area( data, 3 * 256, f );
  }
  fclose( f );
  return 0;
}

int
save_png_image( FILE *f, int bpp, int planes, int w, int h, u8 *image )
{
  int           ret, wc, fmt, length;
  unsigned int  o1, o2, o3, x, y;
  u8            b, sh, *d, *row, *off, *mix_row;
  spng_ctx     *ctx = NULL;
  struct spng_ihdr ihdr = { 0 };        // zero-initialize to set valid defaults 
  struct spng_plte plte;

  // Creating an encoder context requires a flag 
  ctx = spng_ctx_new( SPNG_CTX_ENCODER );
  // Encode to file
  spng_set_png_file( ctx, f );
  // Set image properties, this determines the destination image format 
  ihdr.width = w;
  ihdr.height = h;
  ihdr.bit_depth = bpp * planes;
  o1 = o2 = o3 = 0;
  mix_row = NULL;
  switch ( ihdr.bit_depth )
  {
    case 4:
      ihdr.color_type = SPNG_COLOR_TYPE_INDEXED;
      length = w * h / 2;
      break;
    case 8:
      ihdr.color_type = SPNG_COLOR_TYPE_GRAYSCALE;
      length = w * h;
      break;
    default:
      ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
      length = w * h * 3;
      break;
  }
  spng_set_ihdr( ctx, &ihdr );
  if( ihdr.color_type == SPNG_COLOR_TYPE_INDEXED )
  {
    plte.n_entries = 16;
    memset( plte.entries, 0, 16 * sizeof( struct spng_plte_entry ) );
    plte.entries[6].red = 0xff; // white
    plte.entries[6].green = 0xff;
    plte.entries[6].blue = 0xff;
    plte.entries[7].red = 0;    // dark green
    plte.entries[7].green = 0x7c;
    plte.entries[7].blue = 0x3c;
    spng_set_plte( ctx, &plte );
  }
  // SPNG_FMT_PNG is a special value that matches the format in ihdr 
  fmt = SPNG_FMT_PNG;
  ret = spng_encode_image( ctx, image, length, fmt, SPNG_ENCODE_PROGRESSIVE );
  if( ret )
  {
    printf( "spng_encode_image() error: %s\n", spng_strerror( ret ) );
    goto encode_error;
  }
  wc = length / h;
  if( planes == 4 )
  {
    mix_row = malloc( wc );
    o1 = wc * 1 / 4;
    o2 = wc * 2 / 4;
    o3 = wc * 3 / 4;
  }
  for( y = 0; y < h; y++ )
  {
    row = image + wc * y;
    if( planes == 4 )
    {
      d = mix_row;
      b = 0;
      for( x = 0; x < w; x += 2 )
      {
        off = row + x / 8;
        sh = ~x & 7;
        b <<= 1;
        b |= *( off + o3 ) >> sh & 1;
        b <<= 1;
        b |= *( off + o2 ) >> sh & 1;
        b <<= 1;
        b |= *( off + o1 ) >> sh & 1;
        b <<= 1;
        b |= *( off ) >> sh & 1;
	sh &= ~1;
        b <<= 1;
        b |= *( off + o3 ) >> sh & 1;
        b <<= 1;
        b |= *( off + o2 ) >> sh & 1;
        b <<= 1;
        b |= *( off + o1 ) >> sh & 1;
        b <<= 1;
        b |= *( off ) >> sh & 1;
        *d++ = b;
      }
      row = mix_row;
    }
    ret = spng_encode_row( ctx, row, wc );
    if( ret )
      break;
  }
  if( planes == 4 )
    free( mix_row );
  if( ret != SPNG_EOI )
  {
    printf( "spng_encode_row() error: %s\n", spng_strerror( ret ) );
    goto encode_error;
  }
  spng_encode_chunks( ctx );
encode_error:
  spng_ctx_free( ctx );
  fclose( f );
  return ret;
}

int
save_image( char *format, int sequence, int bpp, int planes,
    int w, int h, u8 *image )
{
  FILE         *f;
  char          fname[128];
  sprintf( fname, format, sequence );
  strcat( fname, sw.pcx ? ".pcx" : ".png" );
  if( ( f = fopen( fname, "wb" ) ) == NULL )
    return -1;
  if( sw.pcx )
    return !save_pcx_image( f, bpp, planes, w, h, image );
  else
    return !save_png_image( f, bpp, planes, w, h, image );
}
