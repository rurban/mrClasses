//
//  Copyright (c) 2004, Gonzalo Garramuno
//
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//  *       Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//  *       Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following disclaimer
//  in the documentation and/or other materials provided with the
//  distribution.
//  *       Neither the name of Gonzalo Garramuno nor the names of
//  its other contributors may be used to endorse or promote products derived
//  from this software without specific prior written permission. 
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef mrFilters_h
#define mrFilters_h

#ifndef mrMacros_h
#include "mrMacros.h"
#endif

#ifndef mrMath_h
#include "mrMath.h"
#endif


BEGIN_NAMESPACE( mr )

BEGIN_NAMESPACE( filter )

//! @todo:  create tables for each filters to avoid calling them directly.

typedef miScalar (*function)(miScalar, miScalar, miScalar, miScalar);

enum types
{
kBox,
kTriangle,
kGaussian,
kSinc,
kDisk,
kCatmullRom,
kMitchell,
kBessel,
kLanczos2,
kLanczos3,
kHann,
kHamming,
// kBlackman,
// kKaiser,
// kKolmogorov-Zurbenko 
};



inline
miScalar box( miScalar x, miScalar y, miScalar w, miScalar h )
{
   w *= 0.5f;
   h *= 0.5f;
   if (x >= -w && x <= w && y >= -h && y <= h)
      return 1.0f;
   else return 0.0f;
}


inline
miScalar triangle( miScalar x, miScalar y, miScalar w, miScalar h )
{
   w *= 0.5f;
   h *= 0.5f;
   if (x >= -w && x <= w && y >= -h && y <= h)
      return ( ( w - math<float>::fabs(x) ) * ( h - math<float>::fabs(y) ) );
   else return 0.0f;
}



inline
miScalar gaussian( miScalar x, miScalar y, miScalar w, miScalar h )
{
//     miScalar d2 = (x*x+y*y);
//     miScalar d = math<float>::sqrt( d2 );
//     miScalar w2 = 0.5f * (w*w + h*h);
//     miScalar w = math<float>::sqrt( w2 );
//     if (d > w)
//        return 0.0f;
//     else
//        return math<float>::exp(-d2) - math<float>::exp(-w2);
   
   // The above version falls faster than the one used by the 3.2 spec
   //   PRMan and RenderDotC.  Since all three match exactly, might as
   //   well change to the code below:
   x *= 2.0f / w;
   y *= 2.0f / h;

   return math<float>::exp( -2.0f * ( x * x + y * y ) );
}


inline
miScalar sinc( miScalar x, miScalar y, miScalar w, miScalar h )
{
	//RtFloat d;
	//
	//d = sqrt(x*x+y*y);
	//
	//if(d!=0)
	//	return(sin(RI_PI*d)/(RI_PI*d));
	//else
	//	return(1.0);

	// The above is an un-windowed sinc, below is a windowed sinc
	//   function similar in shape to what PRMan 3.9 uses.
	// tburge 5-28-01

	/* Modified version of the RI Spec 3.2 sinc filter to be
	 *   windowed with a positive lobe of a cosine which is half
	 *   of a cosine period.  
	 */

	/* Uses a -PI to PI cosine window. */
	if ( x != 0.0f )
	{
	   x *= (miScalar)M_PI;
	   x = math<float>::cos( 0.5f * x / w ) * math<float>::sin( x ) / x;
	}
	else
	{
	   x = 1.0f;
	}
	if ( y != 0.0 )
	{
	   y *= (miScalar)M_PI;
	   y = math<float>::cos( 0.5f * y / h ) * math<float>::sin( y ) / y;
	}
	else
	{
	   y = 1.0f;
	}

	/* This is a square separable filter and is the 2D Fourier
	 * transform of a rectangular box outlining a lowpass bandwidth
	* filter in the frequency domain.
	*/ 
	return x*y;
}


inline
miScalar disk( miScalar x, miScalar y, miScalar w, miScalar h )
{
   miScalar xx = x * x;
   miScalar yy = y * y;
   w *= 0.5f;
   h *= 0.5f;

   miScalar d = ( xx ) / ( w * w ) + ( yy ) / ( h * h );
   if ( d < 1.0f )
   {
      return 1.0f;
   }
   else
   {
      return 0.0f;
   }
}



inline
miScalar catmullrom( miScalar x, miScalar y, miScalar w, miScalar h )
{
   miScalar d2 = x * x + y * y; /* d*d */
   miScalar d = sqrt( d2 ); /* distance from origin */
   
   if ( d < 1.0f )
      return ( 1.5f * d * d2 - 2.5f * d2 + 1.0f );
   else if ( d < 2.0f )
      return ( -d * d2 * 0.5f + 2.5f * d2 - 4.0f * d + 2.0f );
   else
      return 0.0f;
}


inline
miScalar mitchell( miScalar x, miScalar y, miScalar w, miScalar h )
{
   static const miScalar B = 1.0f / 3.0f;
   static const miScalar C = 1.0f / 3.0f;
   static const miScalar D = 1.0f / 6.0f;
   
   x /= w;
   y /= h;
   miScalar t = math<float>::sqrt(x*x + y*y);
   if (t > 2.0f) return 0.0f;
   else
   {
      if (t > 1.0f)
	 return ((( (-B - 6.0f*C) * t + (6.0f*B + 30.0f*C) ) * t +
		  (-12.0f*B - 48.0f*C) ) * t + (8.0f*B + 24.0f*C)) * D;
      else
	 return (( (12.0f - 9.0f*B - 6.0f*C) * t +
		   (-18.0f + 12.0f*B + 6.0f*C) ) * t * t +
		 (6.0f - 2.0f*B)) * D;
   }
}



inline
miScalar  bessel( miScalar x, miScalar y, miScalar w, miScalar h )
{
   miScalar xx = x * x;
   miScalar yy = y * y;
   
   w *= 0.5f;
   h *= 0.5f;
   
   miScalar t = ( xx ) / ( w * w ) + ( yy ) / ( h * h );
   if ( t < 1.0f )
   {
      miScalar d = xx + yy;
      if ( d > 0.0f )
      {
	 d = math<float>::sqrt( d );
	 /* Half cosine window. */
	 t = math<float>::cos( 0.5f * (miScalar)M_PI * math<float>::sqrt( t ) );
	 // j1() is a math.h bessel function
	 return t * 2.0f * (miScalar)j1( ((miScalar)M_PI) * d ) / d;
      }
      else
      {
	 return (miScalar)M_PI;
      }
   }
   else
   {
      return 0.0f;
   }
}

// for lanczos...
inline miScalar sinc( miScalar t )
{
   if ( t != 0.0f )
   {
      t *= (miScalar)M_PI;
      return math<float>::cos( 0.5f * t ) * math<float>::sin( t ) / t;
   }
   else
   {
      return 1.0f;
   }
}

// 2 lobbed lanczos
inline
miScalar  lanczos2( miScalar x, miScalar y, miScalar w, miScalar h )
{
   x /= w;
   y /= h;
   miScalar t = math<float>::sqrt(x*x + y*y);
   if ( t < 2 ) return sinc(t)*sinc(t/2);
   else return 0.0f;
}

// 3 lobbed lanczos
inline
miScalar  lanczos( miScalar x, miScalar y, miScalar w, miScalar h )
{
   x /= w;
   y /= h;
   miScalar t = math<float>::sqrt(x*x + y*y);
   if ( t < 3 ) return sinc(t)*sinc(t/3);
   else return 0.0f;
}


inline
miScalar  hann( miScalar x, miScalar y, miScalar w, miScalar h,
		miScalar n = 3,
		miScalar a = 0.5f )
{
   x /= w;
   y /= h;
   miScalar t = math<float>::sqrt(x*x + y*y);
   n -= 1;
   if ( t < n*0.5f )
   {
      return a + (1.0f - a) * math<float>::cos( 2 * (miScalar)M_PI * t / n );
   }
   else
   {
      return 0.0f;
   }
}


inline
miScalar  hamming( miScalar x, miScalar y, miScalar w, miScalar h,
		   miScalar n = 3 )
{
   return hann( x, y, w, h, n, 0.54f );
}



inline
function fromEnumeration( const types flt )
{
   switch( flt )
   {
      case mr::filter::kGaussian:
	 return mr::filter::gaussian;
      case mr::filter::kTriangle:
	 return mr::filter::triangle;
      case mr::filter::kMitchell:
	 return mr::filter::mitchell;
      case mr::filter::kLanczos2:
	 return mr::filter::lanczos2;
      case mr::filter::kLanczos3:
	 return mr::filter::lanczos;
      case mr::filter::kDisk:
	 return mr::filter::disk;
      case mr::filter::kSinc:
	 return mr::filter::sinc;
      case mr::filter::kCatmullRom:
	 return mr::filter::catmullrom;
      case mr::filter::kBessel:
	 return mr::filter::bessel;
      case mr::filter::kBox:
      default:
	 return mr::filter::box;
   }
}

END_NAMESPACE( filters )

END_NAMESPACE( mr )


#endif

