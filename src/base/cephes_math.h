/*
Cephes Math Library Release 2.2:  June, 1992
Copyright 1985, 1987, 1988, 1992 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/


/* Single precision circular sine
 * test interval: [-pi/4, +pi/4]
 * trials: 10000
 * peak relative error: 6.8e-8
 * rms relative error: 2.6e-8
 */


static float FOPI = 1.27323954473516;
static float PIO4F = 0.7853981633974483096;
/* Note, these constants are for a 32-bit significand: */
/*
  static float DP1 =  0.7853851318359375;
  static float DP2 =  1.30315311253070831298828125e-5;
  static float DP3 =  3.03855025325309630e-11;
  static float lossth = 65536.;
*/

/* These are for a 24-bit significand: */
static float DP1 = 0.78515625;
static float DP2 = 2.4187564849853515625e-4;
static float DP3 = 3.77489497744594108e-8;
static float lossth = 8192.;
static float T24M1 = 16777215.;

static float sincof[] = {
  -1.9515295891E-4,
  8.3321608736E-3,
  -1.6666654611E-1
};
static float coscof[] = {
  2.443315711809948E-005,
  -1.388731625493765E-003,
  4.166664568298827E-002
};

float cephes_sinf( float xx )
{
  float *p;
  float x, y, z;
  register unsigned long j;
  register int sign;

  sign = 1;
  x = xx;
  if( xx < 0 )
    {
      sign = -1;
      x = -xx;
    }
  if( x > T24M1 )
    {
      //mtherr( "sinf", TLOSS );
      return(0.0);
    }
  j = FOPI * x; /* integer part of x/(PI/4) */
  y = j;
  /* map zeros to origin */
  if( j & 1 )
    {
      j += 1;
      y += 1.0;
    }
  j &= 7; /* octant modulo 360 degrees */
  /* reflect in x axis */
  if( j > 3)
    {
      sign = -sign;
      j -= 4;
    }
  if( x > lossth )
    {
      //mtherr( "sinf", PLOSS );
      x = x - y * PIO4F;
    }
  else
    {
      /* Extended precision modular arithmetic */
      x = ((x - y * DP1) - y * DP2) - y * DP3;
    }
  /*einits();*/
  z = x * x;
  //printf("my_sinf: corrected oldx, x, y = %14.10g, %14.10g, %14.10g\n", oldx, x, y);
  if( (j==1) || (j==2) )
    {
      /* measured relative error in +/- pi/4 is 7.8e-8 */
      /*
        y = ((  2.443315711809948E-005 * z
        - 1.388731625493765E-003) * z
        + 4.166664568298827E-002) * z * z;
      */
      p = coscof;
      y = *p++;
      y = y * z + *p++;
      y = y * z + *p++;
      y *= z; y *= z;
      y -= 0.5 * z;
      y += 1.0;
    }
  else
    {
      /* Theoretical relative error = 3.8e-9 in [-pi/4, +pi/4] */
      /*
        y = ((-1.9515295891E-4 * z
        + 8.3321608736E-3) * z
        - 1.6666654611E-1) * z * x;
        y += x;
      */
      p = sincof;
      y = *p++;
      y = y * z + *p++;
      y = y * z + *p++;
      y *= z; y *= x;
      y += x;
    }
  /*einitd();*/
  //printf("my_sinf: j=%d result = %14.10g * %d\n", j, y, sign);
  if(sign < 0)
    y = -y;
  return( y);
}


/* Single precision circular cosine
 * test interval: [-pi/4, +pi/4]
 * trials: 10000
 * peak relative error: 8.3e-8
 * rms relative error: 2.2e-8
 */

float cephes_cosf( float xx )
{
  float x, y, z;
  int j, sign;

  /* make argument positive */
  sign = 1;
  x = xx;
  if( x < 0 )
    x = -x;

  if( x > T24M1 )
    {
      //mtherr( "cosf", TLOSS );
      return(0.0);
    }

  j = FOPI * x; /* integer part of x/PIO4 */
  y = j;
  /* integer and fractional part modulo one octant */
  if( j & 1 )	/* map zeros to origin */
    {
      j += 1;
      y += 1.0;
    }
  j &= 7;
  if( j > 3)
    {
      j -=4;
      sign = -sign;
    }

  if( j > 1 )
    sign = -sign;

  if( x > lossth )
    {
      //mtherr( "cosf", PLOSS );
      x = x - y * PIO4F;
    }
  else
    /* Extended precision modular arithmetic */
    x = ((x - y * DP1) - y * DP2) - y * DP3;

  //printf("xx = %g -> x corrected = %g sign=%d j=%d y=%g\n", xx, x, sign, j, y);

  z = x * x;

  if( (j==1) || (j==2) )
    {
      y = (((-1.9515295891E-4f * z
             + 8.3321608736E-3f) * z
            - 1.6666654611E-1f) * z * x)
        + x;
    }
  else
    {
      y = ((  2.443315711809948E-005f * z
              - 1.388731625493765E-003f) * z
           + 4.166664568298827E-002f) * z * z;
      y -= 0.5 * z;
      y += 1.0;
    }
  if(sign < 0)
    y = -y;
  return( y );
}
