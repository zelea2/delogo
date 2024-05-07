#ifndef SPNG_H
#define SPNG_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define SPNG_VERSION_MAJOR 0
#define SPNG_VERSION_MINOR 7
#define SPNG_VERSION_PATCH 4

enum spng_errno
{
  SPNG_IO_ERROR = -2,
  SPNG_IO_EOF = -1,
  SPNG_OK = 0,
  SPNG_EINVAL,
  SPNG_EMEM,
  SPNG_EOVERFLOW,
  SPNG_ESIGNATURE,
  SPNG_EWIDTH,
  SPNG_EHEIGHT,
  SPNG_EUSER_WIDTH,
  SPNG_EUSER_HEIGHT,
  SPNG_EBIT_DEPTH,
  SPNG_ECOLOR_TYPE,
  SPNG_ECOMPRESSION_METHOD,
  SPNG_EFILTER_METHOD,
  SPNG_EINTERLACE_METHOD,
  SPNG_EIHDR_SIZE,
  SPNG_ENOIHDR,
  SPNG_ECHUNK_POS,
  SPNG_ECHUNK_SIZE,
  SPNG_ECHUNK_CRC,
  SPNG_ECHUNK_TYPE,
  SPNG_ECHUNK_UNKNOWN_CRITICAL,
  SPNG_EDUP_PLTE,
  SPNG_EDUP_CHRM,
  SPNG_EDUP_SBIT,
  SPNG_EDUP_SRGB,
  SPNG_EDUP_BKGD,
  SPNG_EDUP_HIST,
  SPNG_EDUP_TRNS,
  SPNG_EDUP_PHYS,
  SPNG_EDUP_TIME,
  SPNG_EDUP_OFFS,
  SPNG_ECHRM,
  SPNG_EPLTE_IDX,
  SPNG_ETRNS_COLOR_TYPE,
  SPNG_ETRNS_NO_PLTE,
  SPNG_ESBIT,
  SPNG_ESRGB,
  SPNG_EBKGD_NO_PLTE,
  SPNG_EBKGD_PLTE_IDX,
  SPNG_EHIST_NO_PLTE,
  SPNG_EPHYS,
  SPNG_ESPLT_NAME,
  SPNG_ESPLT_DUP_NAME,
  SPNG_ESPLT_DEPTH,
  SPNG_ETIME,
  SPNG_EOFFS,
  SPNG_EIDAT_TOO_SHORT,
  SPNG_EIDAT_STREAM,
  SPNG_EZLIB,
  SPNG_EFILTER,
  SPNG_EBUFSIZ,
  SPNG_EIO,
  SPNG_EOF,
  SPNG_EBUF_SET,
  SPNG_EBADSTATE,
  SPNG_EFMT,
  SPNG_EFLAGS,
  SPNG_ECHUNKAVAIL,
  SPNG_ENCODE_ONLY,
  SPNG_EOI,
  SPNG_ENOPLTE,
  SPNG_ECHUNK_LIMITS,
  SPNG_EZLIB_INIT,
  SPNG_ECHUNK_STDLEN,
  SPNG_EINTERNAL,
  SPNG_ECTXTYPE,
  SPNG_ENOSRC,
  SPNG_ENODST,
  SPNG_EOPSTATE,
  SPNG_ENOTFINAL,
};

enum spng_color_type
{
  SPNG_COLOR_TYPE_GRAYSCALE = 0,
  SPNG_COLOR_TYPE_TRUECOLOR = 2,
  SPNG_COLOR_TYPE_INDEXED = 3,
  SPNG_COLOR_TYPE_GRAYSCALE_ALPHA = 4,
  SPNG_COLOR_TYPE_TRUECOLOR_ALPHA = 6
};

enum spng_filter
{
  SPNG_FILTER_NONE = 0,
  SPNG_FILTER_SUB = 1,
  SPNG_FILTER_UP = 2,
  SPNG_FILTER_AVERAGE = 3,
  SPNG_FILTER_PAETH = 4
};

enum spng_filter_choice
{
  SPNG_DISABLE_FILTERING = 0,
  SPNG_FILTER_CHOICE_NONE = 8,
  SPNG_FILTER_CHOICE_SUB = 16,
  SPNG_FILTER_CHOICE_UP = 32,
  SPNG_FILTER_CHOICE_AVG = 64,
  SPNG_FILTER_CHOICE_PAETH = 128,
  SPNG_FILTER_CHOICE_ALL = ( 8 | 16 | 32 | 64 | 128 )
};

enum spng_interlace_method
{
  SPNG_INTERLACE_NONE = 0,
  SPNG_INTERLACE_ADAM7 = 1
};

/* Channels are always in byte-order */
enum spng_format
{
  SPNG_FMT_RGBA8 = 1,
  SPNG_FMT_RGBA16 = 2,
  SPNG_FMT_RGB8 = 4,

  /*
   * Partially implemented, see documentation 
   */
  SPNG_FMT_GA8 = 16,
  SPNG_FMT_GA16 = 32,
  SPNG_FMT_G8 = 64,

  /*
   * No conversion or scaling 
   */
  SPNG_FMT_PNG = 256,
  SPNG_FMT_RAW = 512		/* big-endian (everything else is
				 * host-endian) */
};

enum spng_ctx_flags
{
  SPNG_CTX_IGNORE_ADLER32 = 1,	/* Ignore checksum in DEFLATE streams */
  SPNG_CTX_ENCODER = 2		/* Create an encoder context */
};

enum spng_decode_flags
{
  SPNG_DECODE_USE_TRNS = 1,	/* Deprecated */
  SPNG_DECODE_USE_GAMA = 2,	/* Deprecated */
  SPNG_DECODE_USE_SBIT = 8,	/* Undocumented */

  SPNG_DECODE_TRNS = 1,		/* Apply transparency */
  SPNG_DECODE_GAMMA = 2,	/* Apply gamma correction */
  SPNG_DECODE_PROGRESSIVE = 256	/* Initialize for progressive reads */
};

enum spng_crc_action
{
  /*
   * Default for critical chunks 
   */
  SPNG_CRC_ERROR = 0,

  /*
   * Discard chunk, invalid for critical chunks. Since v0.6.2: default for
   * ancillary chunks 
   */
  SPNG_CRC_DISCARD = 1,

  /*
   * Ignore and don't calculate checksum. Since v0.6.2: also ignores
   * checksums in DEFLATE streams 
   */
  SPNG_CRC_USE = 2
};

enum spng_encode_flags
{
  SPNG_ENCODE_PROGRESSIVE = 1,	/* Initialize for progressive writes */
  SPNG_ENCODE_FINALIZE = 2,	/* Finalize PNG after encoding image */
};

struct spng_ihdr
{
  uint32_t      width;
  uint32_t      height;
  uint8_t       bit_depth;
  uint8_t       color_type;
  uint8_t       compression_method;
  uint8_t       filter_method;
  uint8_t       interlace_method;
};

struct spng_plte_entry
{
  uint8_t       red;
  uint8_t       green;
  uint8_t       blue;

  uint8_t       alpha;		/* Reserved for internal use */
};

struct spng_plte
{
  uint32_t      n_entries;
  struct spng_plte_entry entries[256];
};

struct spng_trns
{
  uint16_t      gray;

  uint16_t      red;
  uint16_t      green;
  uint16_t      blue;

  uint32_t      n_type3_entries;
  uint8_t       type3_alpha[256];
};

struct spng_chrm_int
{
  uint32_t      white_point_x;
  uint32_t      white_point_y;
  uint32_t      red_x;
  uint32_t      red_y;
  uint32_t      green_x;
  uint32_t      green_y;
  uint32_t      blue_x;
  uint32_t      blue_y;
};

struct spng_chrm
{
  double        white_point_x;
  double        white_point_y;
  double        red_x;
  double        red_y;
  double        green_x;
  double        green_y;
  double        blue_x;
  double        blue_y;
};

struct spng_sbit
{
  uint8_t       grayscale_bits;
  uint8_t       red_bits;
  uint8_t       green_bits;
  uint8_t       blue_bits;
  uint8_t       alpha_bits;
};

struct spng_bkgd
{
  uint16_t      gray;		/* Only for gray/gray alpha */
  uint16_t      red;
  uint16_t      green;
  uint16_t      blue;
  uint16_t      plte_index;	/* Only for indexed color */
};

struct spng_phys
{
  uint32_t      ppu_x, ppu_y;
  uint8_t       unit_specifier;
};

struct spng_splt_entry
{
  uint16_t      red;
  uint16_t      green;
  uint16_t      blue;
  uint16_t      alpha;
  uint16_t      frequency;
};

struct spng_splt
{
  char          name[80];
  uint8_t       sample_depth;
  uint32_t      n_entries;
  struct spng_splt_entry *entries;
};

struct spng_time
{
  uint16_t      year;
  uint8_t       month;
  uint8_t       day;
  uint8_t       hour;
  uint8_t       minute;
  uint8_t       second;
};

struct spng_offs
{
  int32_t       x, y;
  uint8_t       unit_specifier;
};

struct spng_exif
{
  size_t        length;
  char         *data;
};

struct spng_chunk
{
  size_t        offset;
  uint32_t      length;
  uint8_t       type[4];
  uint32_t      crc;
};

enum spng_location
{
  SPNG_AFTER_IHDR = 1,
  SPNG_AFTER_PLTE = 2,
  SPNG_AFTER_IDAT = 8,
};

struct spng_unknown_chunk
{
  uint8_t       type[4];
  size_t        length;
  void         *data;
  enum spng_location location;
};

enum spng_option
{
  SPNG_KEEP_UNKNOWN_CHUNKS = 1,

  SPNG_IMG_COMPRESSION_LEVEL,
  SPNG_IMG_WINDOW_BITS,
  SPNG_IMG_MEM_LEVEL,
  SPNG_IMG_COMPRESSION_STRATEGY,

  SPNG_TEXT_COMPRESSION_LEVEL,
  SPNG_TEXT_WINDOW_BITS,
  SPNG_TEXT_MEM_LEVEL,
  SPNG_TEXT_COMPRESSION_STRATEGY,

  SPNG_FILTER_CHOICE,
  SPNG_CHUNK_COUNT_LIMIT,
  SPNG_ENCODE_TO_BUFFER,
};

typedef void *spng_malloc_fn( size_t size );
typedef void *spng_realloc_fn( void *ptr, size_t size );
typedef void *spng_calloc_fn( size_t count, size_t size );
typedef void  spng_free_fn( void *ptr );

struct spng_alloc
{
  spng_malloc_fn *malloc_fn;
  spng_realloc_fn *realloc_fn;
  spng_calloc_fn *calloc_fn;
  spng_free_fn *free_fn;
};

struct spng_row_info
{
  uint32_t      scanline_idx;
  uint32_t      row_num;	/* deinterlaced row index */
  int           pass;
  uint8_t       filter;
};

typedef struct spng_ctx spng_ctx;

typedef int   spng_read_fn( spng_ctx * ctx, void *user, void *dest,
    size_t length );
typedef int   spng_write_fn( spng_ctx * ctx, void *user, void *src,
    size_t length );

typedef int   spng_rw_fn( spng_ctx * ctx, void *user, void *dst_src,
    size_t length );

spng_ctx     *spng_ctx_new( int flags );
spng_ctx     *spng_ctx_new2( struct spng_alloc *alloc, int flags );
void          spng_ctx_free( spng_ctx * ctx );
int           spng_encode_chunks( spng_ctx * ctx );
int           spng_set_png_file( spng_ctx * ctx, FILE * file );
/* Encode */
int           spng_encode_image( spng_ctx * ctx, const void *img,
    size_t len, int fmt, int flags );
/* Progressive encode */
int           spng_encode_row( spng_ctx * ctx, const void *row, size_t len );
/* Official extensions */
int           spng_set_ihdr( spng_ctx * ctx, struct spng_ihdr *ihdr );
int           spng_set_plte( spng_ctx * ctx, struct spng_plte *plte );
const char   *spng_strerror( int err );
const char   *spng_version_string( void );

#endif				/* SPNG_H */
