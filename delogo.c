/*
 * Author: Emil *
 */
#include <getopt.h>
#include <libgen.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "delogo.h"

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
static int    video_stream_index = -1;

struct swopt  sw;

static int
open_input_file( const char *filename )
{
  const AVCodec *dec;
  int           ret;

  if( ( ret = avformat_open_input( &fmt_ctx, filename, NULL, NULL ) ) < 0 )
  {
    av_log( NULL, AV_LOG_ERROR, "Cannot open input file\n" );
    return ret;
  }

  if( ( ret = avformat_find_stream_info( fmt_ctx, NULL ) ) < 0 )
  {
    av_log( NULL, AV_LOG_ERROR, "Cannot find stream information\n" );
    return ret;
  }

  // select the video stream
  ret = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0 );
  if( ret < 0 )
  {
    av_log( NULL, AV_LOG_ERROR,
        "Cannot find a video stream in the input file\n" );
    return ret;
  }
  video_stream_index = ret;

  // create decoding context
  dec_ctx = avcodec_alloc_context3( dec );
  if( !dec_ctx )
    return AVERROR( ENOMEM );
  avcodec_parameters_to_context( dec_ctx,
      fmt_ctx->streams[video_stream_index]->codecpar );

  // init the video decoder
  if( ( ret = avcodec_open2( dec_ctx, dec, NULL ) ) < 0 )
  {
    av_log( NULL, AV_LOG_ERROR, "Cannot open video decoder\n" );
    return ret;
  }

  av_dump_format( fmt_ctx, 0, filename, 0 );
  return 0;
}

static struct option long_options[] = {
  {"help", no_argument, 0, '?'},
  {"info", no_argument, 0, 'i'},
  {"canny", no_argument, 0, 'y'},
  {"two_rectangles", no_argument, 0, '2'},
  {"removelogo", no_argument, 0, 'm'},
  {"pixel_ratio", required_argument, 0, 'r'},
  {"aggressive_crop", required_argument, 0, 'a'},
  {"corner", required_argument, 0, 'c'},
  {"jump_corner", no_argument, 0, 'j'},
  {"frames", required_argument, 0, 'f'},
  {"width_bands", required_argument, 0, 'W'},
  {"height_bands", required_argument, 0, 'H'},
  {"level_black", required_argument, 0, 'b'},
  {"level_delta", required_argument, 0, 'd'},
  {"border_black", required_argument, 0, 'k'},
  {"border_band", required_argument, 0, 'z'},
  {"threshold", required_argument, 0, 't'},
  {"sigma", required_argument, 0, 's'},
  {"tlow", required_argument, 0, 'l'},
  {"thigh", required_argument, 0, 'h'},
  {"save_as_pcx", no_argument, 0, 'X'},
  {0, 0, 0, 0}
};

struct option_description
{
  char         *description;
  void         *vdefault;
};

static struct option_description opdsc[] = {
  {"show options help", NULL},
  {"show video info only", NULL},
  {"force canny edge detection for logo", NULL},
  {"cover the logo with two overlaping rectangles", NULL},
  {"generate a mask of the logo instead of a rectangle", NULL},
  {"switch to mask mode if pixel percentage inside the logo\n"
        "\t\t\t\tbounding box is bellow (default %d%%)", &sw.pixel_ratio},
  {"crop video aggressively\n"
    "\t\t\t\tignoring some percentage of non-black pixels", NULL},
  {"fix logo corner [0-3] (NW,NE,SW,SE)", NULL},
  {"logo jumps from corner to corner", NULL},
  {"number of frames to process (default %d)", &sw.smpl_frames},
  {"width percentage search band (default %d%%)", &sw.w_band_perc},
  {"height percentage search band (default %d%%)", &sw.h_band_perc},
  {"maximum black level (default %d)", &sw.level_black},
  {"maximum luminance distace for logo detection (default %d)",
      &sw.level_delta},
  {"border thickness around logo (default %d)", &sw.border_black},
  {"additional border band (default %d)", &sw.border_band},
  {"canny edge detection threshold 0-255 (default %d)", &sw.canny_threshold},
  {"canny sigma (default %.1f)", &sw.canny_sigma},
  {"canny tlow (default %.1f)", &sw.canny_tlow},
  {"canny thigh (default %.1f)", &sw.canny_thigh},
  {"save images as PCX rather than PNG", NULL},
};

static void
usage( char *pgmname )
{
  struct option *op = long_options;
  struct option_description *od = opdsc;
  char          format[128], *p;
  int           l, long_len = 0;

  while( op->name )
  {
    l = strlen( op++->name );
    if( l > long_len )
      long_len = l;
  }
  long_len++;
  op = long_options;
  printf( "Usage: %s [options] video_file\n",
      pgmname );
  while( op->name )
  {
    l = strlen( op->name );
    sprintf( format, "\t-%c,--%s%*c %s\n", op->val, op->name, long_len - l,
        ' ', od->description );
    if( ( p = strchr( format, '%' ) ) == NULL )
      printf( format );
    else
    {
      if( strchr( p, 'd' ) )
        printf( format, *( int * ) od->vdefault );
      else if( strchr( p, 'f' ) )
        printf( format, *( float * ) od->vdefault );
    }
    op++, od++;
  }
  exit( 0 );
}

void
program_options( int argc, char **argv )
{
  int           c, corner;

  sw.corner_mask = ( 1 << CORNERS ) - 1;
  sw.aggressive_crop = DEFAULT_AGGRESSIVE_CROP;
  sw.smpl_frames = DEFAULT_SMPL_FRAMES;
  sw.w_band_perc = DEFAULT_W_BAND_PERC;
  sw.h_band_perc = DEFAULT_H_BAND_PERC;
  sw.level_black = DEFAULT_LEVEL_BLACK;
  sw.level_delta = DEFAULT_LEVEL_DELTA;
  sw.border_black = DEFAULT_BORDER_BLACK;
  sw.border_band = DEFAULT_BORDER_BAND;
  sw.canny_threshold = DEFAULT_CANNY_THRESHOLD;
  sw.canny_sigma = DEFAULT_CANNY_SIGMA;
  sw.canny_tlow = DEFAULT_CANNY_TLOW;
  sw.canny_thigh = DEFAULT_CANNY_THIGH;
  sw.pixel_ratio = DEFAULT_PIXEL_RATIO;

  while( 1 )
  {
    int           option_index = 0;

    c = getopt_long( argc, argv, ":ijy2Xma:c:r:f:W:H:b:d:k:z:t:s:l:h:",
        long_options, &option_index );
    // Detect the end of the options. 
    if( c == -1 )
      break;
    switch ( c )
    {
      case 'i':
        sw.info = 1;
        sw.smpl_frames = 1;
        break;
      case 'j':
        sw.jump_corner = 1;
        sw.smpl_frames = 2048;
        sw.canny = 1;
        break;
      case 'y':
        sw.canny = 1;
        break;
      case '2':
        sw.tworect = 1;
        break;
      case 'X':
        sw.pcx = 1;
        break;
      case 'm':
        sw.logomask = 1;
        break;
      case 'r':
        sw.pixel_ratio = atoi( optarg );
        if( sw.pixel_ratio < 0 || sw.pixel_ratio > 100 )
          sw.pixel_ratio = DEFAULT_PIXEL_RATIO;
        break;
      case 'a':
        sw.aggressive_crop = atoi( optarg );
        if( sw.aggressive_crop < 0 || sw.aggressive_crop > 100 )
          sw.aggressive_crop = DEFAULT_AGGRESSIVE_CROP;
        break;
      case 'c':
        corner = atoi( optarg );
        if( corner >= 0 && corner < CORNERS )
          sw.corner_mask = 1 << corner;
	else
	  sw.corner_mask = 0x80; // ignore logo
        break;
      case 'f':
        sw.smpl_frames = atoi( optarg );
        if( sw.smpl_frames < 5 || sw.smpl_frames > 500 )
          sw.smpl_frames = DEFAULT_SMPL_FRAMES;
        break;
      case 'W':
        sw.w_band_perc = atoi( optarg );
        if( sw.w_band_perc > 48 || sw.w_band_perc < 3 )
          sw.w_band_perc = DEFAULT_W_BAND_PERC;
        break;
      case 'H':
        sw.h_band_perc = atoi( optarg );
        if( sw.h_band_perc > 48 || sw.h_band_perc < 3 )
          sw.h_band_perc = DEFAULT_H_BAND_PERC;
        break;
      case 'b':
        sw.level_black = atoi( optarg );
        if( sw.level_black < 1 || sw.level_black > 255 )
          sw.level_black = DEFAULT_LEVEL_BLACK;
        break;
      case 'd':
        sw.level_delta = atoi( optarg );
        if( sw.level_delta < 1 || sw.level_delta > 255 )
          sw.level_delta = DEFAULT_LEVEL_DELTA;
        break;
      case 'k':
        sw.border_black = atoi( optarg );
        if( sw.border_black > 100 || sw.border_black < 0 )
          sw.border_black = DEFAULT_BORDER_BLACK;
        break;
      case 'z':
        sw.border_band = atoi( optarg );
        if( sw.border_band > 20 || sw.border_band < 0 )
          sw.border_band = DEFAULT_BORDER_BAND;
        break;
      case 't':
        sw.canny_threshold = atoi( optarg );
        if( sw.canny_threshold > 255 || sw.canny_threshold < 0 )
          sw.canny_threshold = DEFAULT_CANNY_THRESHOLD;
        break;
      case 's':
        sw.canny_sigma = atof( optarg );
        if( sw.canny_sigma < 0.0 )
          sw.canny_sigma = DEFAULT_CANNY_SIGMA;
        break;
      case 'l':
        sw.canny_tlow = atof( optarg );
        if( sw.canny_tlow > 1.0 || sw.canny_tlow <= 0 )
          sw.canny_tlow = DEFAULT_CANNY_TLOW;
        break;
      case 'h':
        sw.canny_thigh = atof( optarg );
        if( sw.canny_thigh > 1.0 || sw.canny_thigh <= 0 )
          sw.canny_thigh = DEFAULT_CANNY_THIGH;
        break;
      case '?':
      case ':':
        usage( basename( argv[0] ) );
      default:
        abort(  );
    }
  }

  if( optind == argc )
    usage( basename( argv[0] ) );
  sw.fnamein = strdup( argv[optind++] );
}

int
main( int argc, char **argv )
{
  int           ret;
  AVPacket     *packet = NULL;
  AVFrame      *frame = NULL;
  AVStream     *stream = NULL;
  struct shot  *Shots;
  unsigned int  i, shot_w, shot_h;
  unsigned int  stream_index;
  int           fridx, key_frame;
  int64_t       ts;
  char		codec[16];
  char          line[256];

  program_options( argc, argv );
  frame = av_frame_alloc(  );
  packet = av_packet_alloc(  );
  if( !frame || !packet )
    return 0;
  if( ( ret = open_input_file( sw.fnamein ) ) < 0 )
    goto end;
  strncpy( line, sw.fnamein, 255 );
  line[255] = 0;

  if( dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P  ||
      dec_ctx->pix_fmt == AV_PIX_FMT_YUVJ420P ||
      dec_ctx->pix_fmt == AV_PIX_FMT_YUV422P  ||
      dec_ctx->pix_fmt == AV_PIX_FMT_YUVJ422P )
  {
    dec_ctx->skip_frame = AVDISCARD_NONKEY;
    Shots = calloc( sw.smpl_frames, sizeof( struct shot ) );
    shot_w = dec_ctx->width * sw.w_band_perc / 100;
    shot_w <<= 1;
    shot_h = dec_ctx->height * sw.h_band_perc / 100;
    shot_h <<= 1;
    if( sw.info && sw.fnamein )
      printf( "%s", sw.fnamein );
    else
      printf( "Key Frame: " );
    for( fridx = 1; fridx <= sw.smpl_frames; fridx++ )
    {
      if( !sw.jump_corner )
      {
        ts = fmt_ctx->duration * fridx / ( sw.smpl_frames + 1 );
        ret = av_seek_frame( fmt_ctx, -1, ts, AVSEEK_FLAG_BACKWARD );
      }
      // read all packets
      while( 1 )
      {
        if( ( ret = av_read_frame( fmt_ctx, packet ) ) < 0 )
	{
	  if( ret == AVERROR_EOF )
	  {
	    sw.smpl_frames = fridx - 1;
	    goto last_frame;
	  }
          break;
	}
        stream_index = packet->stream_index;
        if( stream_index != video_stream_index )
        {
          av_packet_unref( packet );
          continue;
        }
        stream = fmt_ctx->streams[stream_index];
        av_log( NULL, AV_LOG_DEBUG,
            "Demuxer gave frame of stream_index %d\n", stream_index );
        ret = avcodec_send_packet( dec_ctx, packet );
        if( ret < 0 )
        {
          av_log( NULL, AV_LOG_ERROR,
              "Error while sending a packet to the decoder\n" );
          goto end;
        }
        key_frame = 0;
        while( !key_frame )
        {
          ret = avcodec_receive_frame( dec_ctx, frame );
          if( ret == AVERROR( EAGAIN ) || ret == AVERROR_EOF )
          {
            break;
          }
          else if( ret < 0 )
          {
            av_log( NULL, AV_LOG_ERROR, "Error while decoding a frame\n" );
            goto end;
          }
          frame->pts = frame->best_effort_timestamp;
          if( frame->key_frame )
          {
	    int		  secs, mins, hours;
            int           shotsize = shot_w * shot_h;
            u8           *s = frame->data[0];   // Y plane
            u8           *d = malloc( shotsize );
#ifdef DEBUG
            int           dts = frame->pkt_dts * 1000 *
                stream->time_base.num / stream->time_base.den;
            printf( "D%d ", dts );
#endif
            if( !sw.info )
              printf( "%d ", fridx );
            fflush( stdout );
            Shots[fridx - 1].timestamp_ms = frame->best_effort_timestamp *
              stream->time_base.num / stream->time_base.den/ 1000;
            Shots[fridx - 1].image = d;
            Shots[fridx - 1].corner = CORNERS; // no corner detected yet
            for( i = 0; i < dec_ctx->height; i++ )
            {
              if( i < shot_h / 2 || i >= dec_ctx->height - shot_h / 2 )
              {
                memcpy( d, s, shot_w / 2 );
                d += shot_w / 2;
                memcpy( d, s + dec_ctx->width - shot_w / 2, shot_w / 2 );
                d += shot_w / 2;
              }
              s += frame->linesize[0];
            }
// #define DEBUG
#ifdef DEBUG
            save_image( "sample%02d", fridx, 8, 1,
                shot_w, shot_h, Shots[fridx - 1].image );
#endif
	    if( fridx == sw.smpl_frames )
            {
last_frame:
              printf( "\n" );
              printf( "Size: %dx%d\n", dec_ctx->width, dec_ctx->height );
              if( !sw.info )
                crop_delogo( shot_w, shot_h, dec_ctx->width, dec_ctx->height,
                    Shots );
	      secs = ( fmt_ctx->duration + 5e5 ) / 1e6;
	      mins = hours = 0;	
	      if( secs >= 60 )
	      {
		mins = secs / 60;
		secs %= 60;
	      }
	      if( mins >= 60 )
	      {
		hours = mins / 60;
		mins %= 60;
	      }
	      switch( stream->codecpar->codec_id )
	      {
		case AV_CODEC_ID_MPEG1VIDEO:
		  strcpy(codec, "MPEG1");
		  break;
		case AV_CODEC_ID_MPEG2VIDEO:
		  strcpy(codec, "MPEG2");
		  break;
	        case AV_CODEC_ID_MPEG4:
		  strcpy(codec, "MPEG4");
		  break;
		case AV_CODEC_ID_H264:
		  strcpy(codec, "H264");
		  break;
		case AV_CODEC_ID_HEVC:
		  strcpy(codec, "HEVC");
		  break;
		case AV_CODEC_ID_VVC:
		  strcpy(codec, "VVC");
		  break;
		case AV_CODEC_ID_WMV1:	
		case AV_CODEC_ID_WMV2:
		case AV_CODEC_ID_WMV3:
		  strcpy(codec, "WMV");
		  break;
		case AV_CODEC_ID_FLV1:	
		  strcpy(codec, "FLV");
		  break;
		case AV_CODEC_ID_AV1:	
		  strcpy(codec, "AV1");
		  break;
		case AV_CODEC_ID_VP9:	
		  strcpy(codec, "VP9");
		  break;
		default:
		  sprintf( codec, "Unk (%d)", stream->codecpar->codec_id );
		  break;
	      }
              printf( "Duration: %.3f (%02d:%02d:%02d) BitRate: %ld "
		  "fps: %.2f Codec: %s\n",
                  fmt_ctx->duration / 1e6, hours, mins, secs, 
		  fmt_ctx->bit_rate, av_q2d(stream->r_frame_rate), codec );
              for( i = 0; i < sw.smpl_frames; i++ )
		if( Shots[i].image )
		{
                  free( Shots[i].image );
		  Shots[i].image = NULL;
		}
            }
            key_frame = 1;
          }
          av_frame_unref( frame );
          break;
        }
        av_packet_unref( packet );
        if( key_frame )
          break;
      }
    }
    free( Shots );
    Shots = NULL;
  }
  // rewind
  ret = av_seek_frame( fmt_ctx, -1, 0LL, 0 );
  av_log( NULL, AV_LOG_ERROR, "Finished\n" );
end:
  avcodec_free_context( &dec_ctx );
  avformat_close_input( &fmt_ctx );
  av_frame_free( &frame );
  av_packet_free( &packet );
  if( ret < 0 && ret != AVERROR_EOF )
  {
    fprintf( stderr, "Error occurred: %s\n", av_err2str( ret ) );
    exit( 1 );
  }
  return 0;
}
