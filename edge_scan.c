/*******************************************************************************
* "Canny" edge detector 
* 
* The processing steps are as follows:
*
*   1) Convolve the image with a separable gaussian filter.
*   2) Take the dx and dy the first derivatives using [-1,0,1] and [1,0,-1]'.
*   3) Compute the magnitude: sqrt(dx*dx+dy*dy).
*   4) Perform non-maximal suppression.
*   5) Perform hysteresis.
*
* The user must input three parameters. These are as follows:
*
*   sigma = The standard deviation of the gaussian smoothing filter.
*   tlow  = Specifies the low value to use in hysteresis. This is a 
*           fraction (0-1) of the computed high threshold edge strength value.
*   thigh = Specifies the high value to use in hysteresis. This fraction (0-1)
*           specifies the percentage point in a histogram of the gradient of
*           the magnitude. Magnitude values of zero are not counted in the
*           histogram.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "delogo.h"

#define BOOSTBLURFACTOR 90.0

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Compute the angle of a vector with components x and
// y. Return this angle in radians with the answer being in the range
// 0 <= angle <2*PI.
static double
angle_radians( double x, double y )
{
  double        xu, yu, ang;

  xu = fabs( x );
  yu = fabs( y );

  if( ( xu == 0 ) && ( yu == 0 ) )
    return ( 0 );

  ang = atan( yu / xu );

  if( x >= 0 )
  {
    if( y >= 0 )
      return ( ang );
    else
      return ( 2 * M_PI - ang );
  }
  else
  {
    if( y >= 0 )
      return ( M_PI - ang );
    else
      return ( M_PI + ang );
  }
}

// Compute the magnitude of the gradient. This is the square root of
// the sum of the squared derivative values.
static void
magnitude_x_y( short int *delta_x, short int *delta_y, int rows, int cols,
    short int *magnitude )
{
  int           r, c, pos, sq1, sq2;

  for( r = 0, pos = 0; r < rows; r++ )
  {
    for( c = 0; c < cols; c++, pos++ )
    {
      sq1 = ( int ) delta_x[pos] * ( int ) delta_x[pos];
      sq2 = ( int ) delta_y[pos] * ( int ) delta_y[pos];
      magnitude[pos] =
          ( short ) ( 0.5 + sqrt( ( float ) sq1 + ( float ) sq2 ) );
    }
  }

}

// Compute the first derivative of the image in both the x any y
// directions. The differential filters that are used are:
//
//                                        -1
//       dx =  -1 0 +1     and       dy =  0
//                                        +1
static void
derrivative_x_y( short int *smoothedim, int rows, int cols,
    short int *delta_x, short int *delta_y )
{
  int           r, c, pos;

  // Compute the x-derivative. Adjust the derivative at the borders to avoid
  // losing pixels.
  for( r = 0; r < rows; r++ )
  {
    pos = r * cols;
    delta_x[pos] = smoothedim[pos + 1] - smoothedim[pos];
    pos++;
    for( c = 1; c < ( cols - 1 ); c++, pos++ )
    {
      delta_x[pos] = smoothedim[pos + 1] - smoothedim[pos - 1];
    }
    delta_x[pos] = smoothedim[pos] - smoothedim[pos - 1];
  }

  // Compute the y-derivative. Adjust the derivative at the borders to avoid
  // losing pixels.
  for( c = 0; c < cols; c++ )
  {
    pos = c;
    delta_y[pos] = smoothedim[pos + cols] - smoothedim[pos];
    pos += cols;
    for( r = 1; r < ( rows - 1 ); r++, pos += cols )
    {
      delta_y[pos] = smoothedim[pos + cols] - smoothedim[pos - cols];
    }
    delta_y[pos] = smoothedim[pos] - smoothedim[pos - cols];
  }
}

// Create a one dimensional gaussian kernel.
static void
make_gaussian_kernel( float sigma, float **kernel, int *windowsize )
{
  int           i, center;
  float         x, fx, sum = 0.0;

  *windowsize = 1 + 2 * ceil( 2.5 * sigma );
  center = ( *windowsize ) / 2;

  if( ( *kernel =
      ( float * ) calloc( ( *windowsize ), sizeof( float ) ) ) == NULL )
  {
    fprintf( stderr, "Error callocing the gaussian kernel array.\n" );
    exit( 1 );
  }

  for( i = 0; i < ( *windowsize ); i++ )
  {
    x = ( float ) ( i - center );
    fx = pow( 2.71828,
        -0.5 * x * x / ( sigma * sigma ) ) / ( sigma * sqrt( 6.2831853 ) );
    ( *kernel )[i] = fx;
    sum += fx;
  }

  for( i = 0; i < ( *windowsize ); i++ )
    ( *kernel )[i] /= sum;
}

// Blur an image with a gaussian filter.
static void
gaussian_smooth( u8 *image, int w, int wc, int hc,
    int windowsize, float *kernel, float *tempim, short int *smoothedim )
{
  unsigned      r, c;
  int           rr, cc, center;
  float         dot,            // Dot product summing variable.
                sum;            // Sum of the kernel weights variable.

  center = windowsize / 2;
  // Blur in the x - direction.
  for( r = 0; r < hc; r++ )
  {
    for( c = 0; c < wc; c++ )
    {
      dot = 0.0;
      sum = 0.0;
      for( cc = ( -center ); cc <= center; cc++ )
      {
        if( ( ( c + cc ) >= 0 ) && ( ( c + cc ) < wc ) )
        {
          dot += ( float ) image[r * w + ( c + cc )] * kernel[center + cc];
          sum += kernel[center + cc];
        }
      }
      tempim[r * wc + c] = dot / sum;
    }
  }

  // Blur in the y - direction.
  for( c = 0; c < wc; c++ )
  {
    for( r = 0; r < hc; r++ )
    {
      sum = 0.0;
      dot = 0.0;
      for( rr = ( -center ); rr <= center; rr++ )
      {
        if( ( ( r + rr ) >= 0 ) && ( ( r + rr ) < hc ) )
        {
          dot += tempim[( r + rr ) * wc + c] * kernel[center + rr];
          sum += kernel[center + rr];
        }
      }
      smoothedim[r * wc + c] =
          ( short int ) ( dot * BOOSTBLURFACTOR / sum + 0.5 );
    }
  }
}

// Perform canny edge detection.
int
canny_logo( int w, int h, u32 wc, u32 hc, unsigned offs,
    struct shot *Shots, struct crn *Crn, int corner )
{
  u32		wB;
  u8           *edge, *nms;     // Points that are local maximal magnitude.
  u8           *p, mask;
  unsigned      r, c;
  short        *smoothedim,     // The image after gaussian smoothing. 
               *delta_x,        // The first devivative image, x-direction.
               *delta_y,        // The first derivative image, y-direction.
               *magnitude;      // The magnitude of the gadient image.
  int          *edge_probability;
  int           brightness;
  unsigned      min_probability, max_probability;
  int           active_frames;
  int           i, si,          // Counters 
                windowsize;     // Dimension of the gaussian kernel.
  float        *kernel,         // A one dimensional gaussian kernel.
               *tempim;         // Buffer for separable filter gaussian smoothing.

  // Allocate a temporary buffer image and the smoothed image.
  if( ( tempim = ( float * ) malloc( hc * wc * sizeof( float ) ) ) == NULL )
    return -1;
  if( ( smoothedim =
      ( short * ) malloc( hc * wc * sizeof( short ) ) ) == NULL )
    return -2;
  // Allocate images to store the derivatives.
  if( ( delta_x = ( short * ) malloc( hc * wc * sizeof( short ) ) ) == NULL )
    return -3;
  if( ( delta_y = ( short * ) malloc( hc * wc * sizeof( short ) ) ) == NULL )
    return -4;
  // Allocate an image to store the magnitude of the gradient.
  if( ( magnitude =
      ( short * ) malloc( hc * wc * sizeof( short ) ) ) == NULL )
    return -5;
  if( ( nms = malloc( hc * wc * sizeof( u8 ) ) ) == NULL )
    return -6;
  if( ( edge = malloc( hc * wc * sizeof( u8 ) ) ) == NULL )
    return -7;
  if( ( edge_probability = calloc( hc * wc, sizeof( int ) ) ) == NULL )
    return -8;

  // Create a 1-dimensional gaussian smoothing kernel.
  make_gaussian_kernel( sw.canny_sigma, &kernel, &windowsize );

  active_frames = 0;
  for( si = 0; si < sw.smpl_frames; si++ )
  {
    active_frames++;
    // Perform gaussian smoothing on the image 
    // using the input standard deviation.
    gaussian_smooth( Shots[si].image + offs, w, wc, hc, windowsize,
        kernel, tempim, smoothedim );
    // Compute the first derivative in the x and y directions.
    derrivative_x_y( smoothedim, hc, wc, delta_x, delta_y );
    // Compute the magnitude of the gradient.
    magnitude_x_y( delta_x, delta_y, hc, wc, magnitude );
    if( sw.jump_corner )
    {
      // Add to edge probability 
      for( i = 0; i < hc * wc; i++ )
        edge_probability[i] += magnitude[i];
    }
    else
    {
      // Perform non-maximal suppression.
      non_max_supp( magnitude, delta_x, delta_y, hc, wc, nms );
      // Use hysteresis to mark the edge pixels.
      apply_hysteresis( magnitude, nms, hc, wc, sw.canny_tlow, sw.canny_thigh,
          edge );
      // Add to edge probability 
      for( i = 0; i < hc * wc; i++ )
        edge_probability[i] += edge[i];
    }
  }
  if( sw.jump_corner )
  {
    min_probability = -1;
    max_probability = 0;
    for( i = 0; i < hc * wc; i++ )
    {
      if( min_probability > edge_probability[i] )
        min_probability = edge_probability[i];
      if( max_probability < edge_probability[i] )
        max_probability = edge_probability[i];
    }
    if( min_probability == max_probability )
    {
      if( Crn->image )
      {
        free( Crn->image );
        Crn->image = NULL;
      }
    }
    else
    {
      // normalize to 1 byte
      max_probability -= min_probability;
      brightness = 0;
      for( i = 0; i < hc * wc; i++ )
      {
        Crn->image[i] = 255 *
            ( edge_probability[i] - min_probability ) / max_probability;
        brightness += Crn->image[i];
      }
      brightness /= hc * wc;
      // printf( "brightness %d\n",brightness);
      if( brightness > 48 && Crn->image )
      {
        free( Crn->image );
        Crn->image = NULL;
      }
    }
  }
  else                          // sw.jump_corner
  {
    wB = ( wc + 7 ) / 8;
    for( r = 0; r < hc; r++ )
    {
      p = Crn->image + wB * r;
      for( c = 0; c < wc; c++ )
      {
        mask = 0x80 >> ( c % 8 );
        if( edge_probability[wc * r + c] > active_frames * sw.canny_threshold )
          *( p + c / 8 ) |= mask;
        else
          *( p + c / 8 ) &= ~mask;
      }
    }
  }
  // Free all of the memory that we allocated
  free( kernel );
  free( edge_probability );
  free( edge );
  free( nms );
  free( magnitude );
  free( delta_y );
  free( delta_x );
  free( smoothedim );
  free( tempim );
  return 0;
}

