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

//
// A simple set of routines that encapsulate obtaining the most common
// type of derivatives.
//

#ifndef mrDerivs_h
#define mrDerivs_h

#include <set>

#ifndef mrVector_h
#include "mrVector.h"
#endif

#ifndef mrStream_h
#include "mrStream.h"
#endif



BEGIN_NAMESPACE( mr )


//! Auxiliary routine to get triangle normals and take them
//! to internal space, normalizing them also.
inline
void get_normals( const miState* const state, miVector N[3] )
{
  // And the normals...
  miVector* n[3];
  mi_tri_vectors(const_cast<miState*>(state), 'n', 0, &n[0], &n[1], &n[2]);

  // Those normals come in object space, transform to internal...
  mi_normal_from_object( const_cast<miState*>(state), &N[0], n[0] );
  mi_normal_from_object( const_cast<miState*>(state), &N[1], n[1] );
  mi_normal_from_object( const_cast<miState*>(state), &N[2], n[2] );

  mi_vector_normalize(&N[0]);
  mi_vector_normalize(&N[1]);
  mi_vector_normalize(&N[2]);
}

//! Auxiliary routine to get triangle vertices and take them
//! to internal space.
inline
void get_vertices( const miState* const state, miVector v[3] )
{
   //
   // First, obtain the points of the triangle
   //
   miVector* p[3];
   mi_tri_vectors(const_cast<miState*>(state), 'p', 0,
		  &p[0], &p[1], &p[2]);

   // Those points come in object space, transform to internal...
   mi_point_from_object( const_cast<miState*>(state), &v[0], p[0] );
   mi_point_from_object( const_cast<miState*>(state), &v[1], p[1] );
   mi_point_from_object( const_cast<miState*>(state), &v[2], p[2] );
}

//! Auxiliary routine to check barycentric coordinates to be in [0,1] range
//! for derivatives.
inline void check_bary_bounds( miScalar b[3] )
{
   float s = (1.0f - b[0] - b[1] - b[2])/3.0f;
   b[0] += s;
   b[1] += s;

   /* now clip coordinates */
   if (b[0] < 0.0f)
      b[0] = 0.0f;
   else if (b[0] > 1.0f)
      b[0] = 1.0f;
   
   if (b[1] < 0.0f)
      b[1] = 0.0f;
   else if (b[1] + b[0] > 1.0f)
      b[1] = 1.0f-b[0];
   
   /* Finally, compute the dependent z */
   b[2] = 1.0f - b[0] - b[1];
}


//! This is from comp.graphics.algorithms, old post of Rod Bogart.
//!
//! Given an arbitrary point in the plane of the triangle, calculate
//! its barycentric coordinates.
//! Returns miFALSE if point is outside triangle.
//!
//!          1         If the area of triangle 123 is A, then the area of 
//!         /|\        P23 is rA.  Area 12P is sA and area 1P3 is tA.
//!        / | \       With this image, it is obvious that r+s+t must equal
//!       /  |  \      one.  If r, s, or t go outside the range zero to one,
//!      / t | s \     P will be outside the triangle.
//!     /  _-P-_  \    
//!    / _-     -_ \   
//!   /_-    r    -_\  
//!  3---------------2 
//!  N = triangle normal = (vec(1 2) cross vec(1 3))
//!
//!       (vec(1 P) cross vec(1 3)) dot N
//!   s = -------------------------------
//!                 length N
//!
//!       (vec(1 2) cross vec(1 P)) dot N
//!   t = -------------------------------
//!                 length N
//!
//!   r = 1 - (s + t)
//!
//! Note that equation as published in the email is kind of incomplete.
//! dot N assumes that N is normalized.  Code below does that.
//!

inline miBoolean
calculate_bary(
	       miScalar b[3],        // Returned barycentric coords
	       const point& p,       // Point to check 
	       const miVector v[3]   // Vertices of triangle internal space
	       )
{
   // Calculate edge segments
   vector E12( v[0] - v[1] ), E13( v[0] - v[2] );
   vector n = E12 ^ E13;
   miScalar areaTriangle = 1.0f / n.length();

   n *= areaTriangle;  //<- NEEDED!!!  this is n.normalize()

   vector E1P( v[0] - p );
   b[0] = (( E1P ^ E13 ) % n) * areaTriangle;
   b[1] = (( E12 ^ E1P ) % n) * areaTriangle;
   b[2] = 1.0f - ( b[0] + b[1] );
   
   if ( (b[0] < 0.0f) || (b[1] < 0.0f) || (b[2] < 0.0f) )
      return miFALSE;
   return miTRUE;
}

inline miBoolean
calculate_bary(
	       miScalar bary[3], // Returned barycentric coords
	       const miState* const state,
	       const point& p   // Point to check 
	       )
{
   miVector v[3];
   get_vertices( state, v );
   return calculate_bary( bary, p, v );
}

//! Auxiliary routine to take two points and return their barycentric coords
inline void
calculate_bary2(
		miScalar bx[3],  // Returned barycentric coords for pt1
		miScalar by[3],  // Returned barycentric coords for pt2
		const miState* const state,
		const point& px,  // Point1
		const point& py   // Point2
	       )
{
   miVector v[3];
   get_vertices( state, v );
   if (! calculate_bary( bx, px, v ) )
      check_bary_bounds(bx);
   
   if (! calculate_bary( by, py, v ) )
      check_bary_bounds(by);
}

//
//! Raster area -ie. pixel footprint - ( more similar to
//! what shader writers expect from prman's area() ).
//!
//! Use areaTriangle(state) for area of actual (micro)triangle
//
inline miScalar area( const miState* const state )
{
   vector Rx( 1, 0, 0 ), Ry( 0, 1, 0 );
   mi_raster_unit( const_cast< miState* >( state ), &Rx, &Ry );
   Rx ^= Ry;
   return Rx.length();
}

//! Same as area, but squared
inline miScalar areaSquared( const miState* const state )
{
   vector Rx( 1, 0, 0 ), Ry( 0, 1, 0 );
   mi_raster_unit( const_cast< miState* >( state ), &Rx, &Ry );
   Rx ^= Ry;
   return Rx.lengthSquared();
}


//! @todo: test if this area(P) works at all
inline miScalar area( const miState* const state,
		      const miVector& P )
{
   vector Rx( 1, 0, 0 ), Ry( 0, 1, 0 );
   miState* const s = const_cast<miState*>(state);
   point tmp( state->point );  s->point = P;
   mi_raster_unit( const_cast< miState* >( state ), &Rx, &Ry );
   s->point = tmp;
   Rx ^= Ry;
   return Rx.length();
}



//! Calculate the squared area of the (micro)triangle.
//! Note that this can be different from the pixel footprint,
//! even if in Prman this is one and the same.
//! Use area(state) for pixel area
inline miScalar areatriangle( const miState* const state )
{
   miVector p[3];
   get_vertices( state, p );
   vector E12( p[0] - p[1] ), E13( p[0] - p[2] );
   vector n( E12 ^ E13 );
   return n.length();
}

//! Same as areatriangle, but squared
inline miScalar areatriangleSquared( const miState* const state )
{
   miVector p[3];
   get_vertices( state, p );
   vector E12( p[0] - p[1] ), E13( p[0] - p[2] );
   vector n( E12 ^ E13 );
   return n.lengthSquared();
}

//! Auxiliary routine to find the two indices to the sides of the triangle
//! (ie. disregard hypotenuse)
inline void get_sides( const miState* const state, int& w, int& h )
{
   miVector* p[3];
   mi_tri_vectors( const_cast<miState*>( state ), 'p', 0,
		   &p[0], &p[1], &p[2] );

   miScalar e0,e1,e2;
   
   vector E( *p[1] - *p[0] );
   e0 = E.length();

   E = *p[2] - *p[1];
   e1 = E.length();
   
   E = *p[0] - *p[2];
   e2 = E.length();
   
   w = 1; h = 2;
   if ( e1 > e0 && e1 > e2 )       w = 0;
   else if ( e2 > e0 && e2 > e1 )  h = 0;
}


//! Calculate Du(data), du and Dv(data), dv.
//!
//! Internal function not for user consumption.
template< typename T >
inline miBoolean DuDv_Impl( T& Du, T& Dv,
			    const miState* state,
			    const miScalar u1,
			    const miScalar u2,
			    const miScalar v1,
			    const miScalar v2,
			    const T data[3],
			    const int idx = 0   // tex_list[] considered as UV
			    )
{

   miScalar x1 = data[1].x - data[0].x;
   miScalar y1 = data[1].y - data[0].y;
   miScalar z1 = data[1].z - data[0].z;
   
   miScalar x2 = data[2].x - data[0].x;
   miScalar y2 = data[2].y - data[0].y;
   miScalar z2 = data[2].z - data[0].z;

   
   miScalar det = u1 * v2 - v1 * u2;   
   
   if (det == 0.0f)
   {
      // degenerate
      Du.x = Dv.y = 1;
      Du.y = Du.z = Dv.x = Dv.z = 0;
      return miFALSE;
   }
   
   miScalar invdet = 1.0f / det;
   Du.x = (x1 * v2 - v1 * x2) * invdet;
   Du.y = (y1 * v2 - v1 * y2) * invdet;
   Du.z = (z1 * v2 - v1 * z2) * invdet;
   Dv.x = (u1 * x2 - x1 * u2) * invdet;
   Dv.y = (u1 * y2 - y1 * u2) * invdet;
   Dv.z = (u1 * z2 - z1 * u2) * invdet;
   
   return miTRUE;
}


//! Calculate Du(data), du and Dv(data), dv.
//!
//! Internal function not for user consumption.
inline miBoolean DuDv_Impl( color& Du, color& Dv,
			    const miState* state,
			    const miScalar u1,
			    const miScalar u2,
			    const miScalar v1,
			    const miScalar v2,
			    const color data[3],
			    const int idx   // tex_list[] considered as UV
			    )
{
   
   miScalar x1 = data[1].r - data[0].r;
   miScalar y1 = data[1].g - data[0].g;
   miScalar z1 = data[1].b - data[0].b;
   miScalar a1 = data[1].a - data[0].a;
   
   miScalar x2 = data[2].r - data[0].r;
   miScalar y2 = data[2].g - data[0].g;
   miScalar z2 = data[2].b - data[0].b;
   miScalar a2 = data[2].a - data[0].a;
  
   miScalar det = u1 * v2 - v1 * u2;
   if (det == 0.0f)
   {
      // degenerate
      Du.r = Dv.g = 1;
      Du.g = Du.b = Dv.r = Dv.b = Dv.a = Du.a = 0;
      return miFALSE;
   }
   
   miScalar invdet = 1.0f / det;
   Du.r = (x1 * v2 - v1 * x2) * invdet;
   Du.g = (y1 * v2 - v1 * y2) * invdet;
   Du.b = (z1 * v2 - v1 * z2) * invdet;
   Du.a = (a1 * v2 - v1 * a2) * invdet;
   Dv.r = (u1 * x2 - x1 * u2) * invdet;
   Dv.g = (u1 * y2 - y1 * u2) * invdet;
   Dv.b = (u1 * z2 - z1 * u2) * invdet;
   Dv.a = (u1 * a2 - a1 * u2) * invdet;
   
   return miTRUE;
}



//! Calculate Du(data), du and Dv(data), dv.
//!
//! Internal function not for user consumption.
inline miBoolean DuDv_Impl( vector2d& Du, vector2d& Dv,
			    const miState* state,
			    const miScalar u1,
			    const miScalar u2,
			    const miScalar v1,
			    const miScalar v2,
			    const vector2d data[3],
			    const int idx   // tex_list[] considered as UV
			    )
{  
   miScalar x1 = data[1].u - data[0].u;
   miScalar y1 = data[1].v - data[0].v;
   
   miScalar x2 = data[2].u - data[0].u;
   miScalar y2 = data[2].v - data[0].v;
  
   miScalar det = u1 * v2 - v1 * u2;
   if (det == 0.0f)
   {
      // degenerate
      Du.u = Dv.v = 1;
      Du.v = Dv.u = 0;
      return miFALSE;
   }

   
   miScalar invdet = 1.0f / det;
   Du.u = (x1 * v2 - v1 * x2) * invdet;
   Du.v = (y1 * v2 - v1 * y2) * invdet;
   Dv.u = (u1 * x2 - x1 * u2) * invdet;
   Dv.v = (u1 * y2 - y1 * u2) * invdet;
   return miTRUE;
}





//! Calculate Du(data), du and Dv(data), dv.
//!
//! Internal function not for user consumption.
inline miBoolean DuDv_Impl( miScalar& Du, miScalar& Dv,
			    const miState* state,
			    const miScalar u1,
			    const miScalar u2,
			    const miScalar v1,
			    const miScalar v2,
			    const miScalar data[3],
			    const int idx    // tex_list[] considered as UV
			    )
{
   
  miScalar x1 = data[1] - data[0];
  miScalar x2 = data[2] - data[0];
  
  miScalar det = u1 * v2 - v1 * u2;
  if (det == 0.0f)
  {
     // degenerate
     Du = Dv = 0;
     return miFALSE;
  }
   
  miScalar invdet = 1.0f / det;
  Du = (x1 * v2 - v1 * x2) * invdet;
  Dv = (u1 * x2 - x1 * u2) * invdet;

  return miTRUE;
}


//! Calculates auxiliary determinants then goes to calculate Du,Dv
template< typename T >
inline miBoolean DuDv_Impl( T& Du, T& Dv,
			    const miState* state,
			    const miScalar bx[3],
			    const miScalar by[3],
			    const T data[3],
			    const int idx = 0   // tex_list[] considered as UV
			    )
{
   int num;
   mi_query( miQ_NUM_TEXTURES, const_cast<miState*>(state), miNULLTAG, &num );
   if ( idx >= num )
   {
      mi_error("DuDv:  idx( %d ) >= num textures in object (%d)",
	       idx, num);
      return miFALSE;
   }
   
   miVector* t[3];
   mi_tri_vectors(const_cast< miState* >(state), 't', idx,
		  &t[0], &t[1], &t[2]);
   
   miVector st0 = ( *t[0] * state->bary[0] +
		    *t[1] * state->bary[1] +
		    *t[2] * state->bary[2] );
   miVector st1 = bx[0] * *t[0] + bx[1] * *t[1] + bx[2] * *t[2];
   miVector st2 = by[0] * *t[0] + by[1] * *t[1] + by[2] * *t[2];
   
   miScalar u1 = st1.x - st0.x;
   miScalar u2 = st2.x - st0.x;
   miScalar v1 = st1.y - st0.y;
   miScalar v2 = st2.y - st0.y;
   
   return DuDv_Impl( Du, Dv, state, u1, u2, v1, v2, data, idx );  
}


//! Calculates du, dv then goes to calculate Du,Dv
template< typename T >
inline miBoolean DuDv_Impl( T& Du, T& Dv,
			    miScalar& du, miScalar& dv,
			    const miState* state,
			    const miScalar bx[3],
			    const miScalar by[3],
			    const T data[3],
			    const int idx = 0   // tex_list[] considered as UV
			    )
{
   int num;
   mi_query( miQ_NUM_TEXTURES, const_cast<miState*>(state), miNULLTAG, &num );
   if ( idx >= num )
   {
      mi_error("DuDv:  idx( %d ) >= num textures in object (%d)",
	       idx, num);
      return miFALSE;
   }
   
   miVector* t[3];
   mi_tri_vectors(const_cast< miState* >(state), 't', idx,
		  &t[0], &t[1], &t[2]);
   
   miVector st0 = ( *t[0] * state->bary[0] +
		    *t[1] * state->bary[1] +
		    *t[2] * state->bary[2] );
   miVector st1 = bx[0] * *t[0] + bx[1] * *t[1] + bx[2] * *t[2];
   miVector st2 = by[0] * *t[0] + by[1] * *t[1] + by[2] * *t[2];
   
   miScalar u1 = st1.x - st0.x;
   miScalar u2 = st2.x - st0.x;
   miScalar v1 = st1.y - st0.y;
   miScalar v2 = st2.y - st0.y;

   miScalar mx,mn;
   minmax( mn, mx, t[0]->x, t[1]->x, t[2]->x);
   du = mx - mn;
   minmax( mn, mx, t[0]->y, t[1]->y, t[2]->y);
   dv = mx - mn;
   
   return DuDv_Impl( Du, Dv, state, u1, u2, v1, v2, data, idx );
  
}


//! Calculate Du(data), du and Dv(data), dv.
//!
//! Du, Dv  are the derivatives of data in u and v
//!         (ie. Du(data), Dv(data)).
//! du, dv  are the derivatives of u and v.
//!
//! This is similar to prman's Du() and Dv() functions.
//! Accuracy of partial derivatives will be dependant on surface tesselation.
//!
//! data[3] is the data to interpolate.  Each index corresponds to a
//!         vertex.  data[0] corresponds to state->point,
//!                  data[1] corresponds to Px,
//!                  data[2] corresponds to Py.
//! idx     is the texture index (state->tex_list[]) to be used as UV.
//!
template< typename T >
inline miBoolean DuDv( T& Du, T& Dv,
		       miScalar& du, miScalar& dv,
		       const miState* state,
		       const point& Px,
		       const point& Py,
		       const T data[3],
		       const int idx = 0   // tex_list[] considered as UV
		       )
{
   miScalar bx[3], by[3];
   calculate_bary2( bx, by, state, Px, Py );
   return DuDv_Impl( Du, Dv, du, dv, state, bx, by, data, idx );  
}

//! Calculate Du(data), du and Dv(data), dv.
//!
//! Du, Dv  are the derivatives of data in u and v (ie. Du(data), Dv(data)).
//! du, dv  are the derivatives of u and v.
//!
//! This is similar to prman's Du() and Dv() functions.
//! Accuracy of partial derivatives will be dependant on surface tesselation.
//!
//! tri_data[3] is the data to interpolate.  Each index corresponds to a
//!             vertex of the triangle, in the order of mi_tri_vectors.
//! idx     is the texture index (state->tex_list[]) to be used as UV.
//!
//! To create tri_data[] when problem cannot be solved analitically,
//! you can use a dummy displacement shader to evaluate and store the result
//! for each vertex.  See mrPointCache.h
template< typename T >
inline miBoolean DuDv( T& Du, T& Dv,
		       miScalar& du, miScalar& dv,
		       const miState* state,
		       const T tri_data[3],
		       const int idx = 0   // tex_list[] considered as UV
		       )
{  
   vector Rx( 0.1f, 0.0f, 0.0f );
   vector Ry( 0.0f, 0.1f, 0.0f );
   mi_raster_unit( const_cast< miState* >( state ), &Rx, &Ry );
   point Px( state->point ); Px += Rx;
   point Py( state->point ); Py += Ry;

   miScalar bx[3], by[3];
   calculate_bary2( bx, by, state, Px, Py );

   T D[3];
   D[0] = ( tri_data[0] * state->bary[0] +
	    tri_data[1] * state->bary[1] +
	    tri_data[2] * state->bary[2] );
   D[1] = ( tri_data[0] * bx[0] +
	    tri_data[1] * bx[1] +
	    tri_data[2] * bx[2] );
   D[2] = ( tri_data[0] * by[0] +
	    tri_data[1] * by[1] +
	    tri_data[2] * by[2] );

   return DuDv_Impl( Du, Dv, du, dv, state, bx, by, D, idx ); 
}



//! Calculate Du(data) and Dv(data) from 3 positions.
//!
//! Du, Dv  are the derivatives of data in u and v (ie. Du(data), Dv(data)).
//!
//! Px      is the       point  offset.
//!         Usually, this is state->point + mi_raster_unit(..,X,..)
//! Py      is the other point  offset.
//!         Usually, this is state->point + mi_raster_unit(..,..,Y)
//! data[3] is the data to interpolate.
//!         data[0] is the data for current point (state->point)
//!         data[1] is the data for point Px
//!         data[2] is the data for point Py
//! idx     is the texture index (state->tex_list[]) to be used as UV.
//!
template< typename T >
inline miBoolean DuDv(
		      T& Du, T& Dv,
		      const miState* state,
		      const vector& Px, const vector& Py,
		      const T data[3],
		      const int idx = 0
		      )
{
   return DuDv( Du, Dv, state, Px, Py, data, idx );
}



//! Calculate Du(data) and Dv(data).
//!
//! Du, Dv  are the derivatives of data in u and v (ie. Du(data), Dv(data)).
//!
//! This is similar to prman's Du() and Dv() functions.
//! Accuracy of partial derivatives will be dependant on surface tesselation.
//!
//! tri_data[3] is the data to interpolate.  Each index corresponds to a
//!             vertex of the triangle, in the order of mi_tri_vectors.
//! idx     is the texture index (state->tex_list[]) to be used as UV.
//!
//! To create tri_data[] when problem cannot be solved analitically,
//! you can use a dummy displacement shader to evaluate and store the result
//! for each vertex.  See mrPointCache.h
template< typename T >
inline miBoolean  DuDv( T& Du, T& Dv,
			const miState* state,
			const T tri_data[3],
			const int idx = 0   // tex_list[] considered as UV
			)
{
   vector Rx( 0.1f, 0.0f, 0.0f );
   vector Ry( 0.0f, 0.1f, 0.0f );
   mi_raster_unit( const_cast< miState* >( state ), &Rx, &Ry );
   point Px( state->point ); Px += Rx;
   point Py( state->point ); Py += Ry;

   miScalar bx[3], by[3];
   calculate_bary2( bx, by, state, Px, Py );

   T D[3];
   D[0] = ( tri_data[0] * state->bary[0] +
	    tri_data[1] * state->bary[1] +
	    tri_data[2] * state->bary[2] );
   D[1] = ( tri_data[0] * bx[0] +
	    tri_data[1] * bx[1] +
	    tri_data[2] * bx[2] );
   D[2] = ( tri_data[0] * by[0] +
	    tri_data[1] * by[1] +
	    tri_data[2] * by[2] );

   return DuDv_Impl( Du, Dv, state, bx, by, D, idx );
}


//! Like DuDv, but returning Du(s)*du, Dv(s)*dv  and Du(t)*du, Dv(t)*dv 
inline void DsuDtv(
		   const miState* const state,
		   miScalar& DuS, miScalar& DvS,
		   miScalar& DuT, miScalar& DvT,
		   const int STidx = 0, // tex_list[] considered as ST
		   const int UVidx = 0  // tex_list[] considered as ST
		   )
{
   
   int num;
   mi_query( miQ_NUM_TEXTURES, const_cast<miState*>( state ),
	     miNULLTAG, &num );
   if ( STidx >= num )
   {
      mi_error("DsuDtv:  STidx( %d ) >= num textures in object (%d)",
	       STidx, num);
      DuS = DvS = DuT = DvT = 0.0f;
      return;
   }

   miVector*  t[3];
   mi_tri_vectors(const_cast< miState* >(state), 't', STidx,
		  &t[0], &t[1], &t[2]);

   miScalar du, dv;
   vector2d Du;
   vector2d Dv;
   vector2d st[3];
   st[0] = *( reinterpret_cast<vector2d*>(t[0]) );
   st[1] = *( reinterpret_cast<vector2d*>(t[1]) );
   st[2] = *( reinterpret_cast<vector2d*>(t[2]) );
   DuDv( Du, Dv, du, dv, state, st, UVidx );

   DuS = Du.u * du; DvS = Dv.u * dv;
   DuT = Du.v * du; DvT = Dv.v * dv;
}





//! @todo: The following routines here could be sped up a tad if we record the
//!        triangle id (instance, state->pri + state->pri_idx) and cache
//!        its dPds, dNds, etc. This has to be done for each miState.
//!        With that we can avoid recalculating them each time, as likely
//!        several contiguous samples will hit the same triangle.


//! Calculate dPds and dPdt for the state->point intersection.
//! idx controls the texture vectors (for when surface has multiple STs)
static
miBoolean dPdst(
		miVector& dPds, miVector& dPdt,
		const miState* const state, const int idx = 0
		)
{

  //
  // First, obtain the texture coordinates of the the three
  // vertices of the triangle, according to user parameters 
  // (projection type, etc).
  //
  miVector* t[3];
  mi_tri_vectors(const_cast< miState* >(state), 't', idx,
		 &t[0], &t[1], &t[2]);

  miScalar u1 = t[1]->x - t[0]->x;
  miScalar u2 = t[2]->x - t[0]->x;
  miScalar v1 = t[1]->y - t[0]->y;
  miScalar v2 = t[2]->y - t[0]->y;
  
  miScalar det = u1 * v2 - v1 * u2;
  if (det == 0.0f)
  {
     // degenerate
     dPds.x = dPdt.y = 1;
     dPds.y = dPds.z = dPdt.x = dPdt.z = 0;
     return miFALSE;
  }
  
  //
  // First, obtain the points of the triangle
  // 
  miVector v[3];
  get_vertices( state, v );
  
  miScalar x1 = v[1].x - v[0].x;
  miScalar x2 = v[2].x - v[0].x;
  miScalar y1 = v[1].y - v[0].y;
  miScalar y2 = v[2].y - v[0].y;
  miScalar z1 = v[1].z - v[0].z;
  miScalar z2 = v[2].z - v[0].z;
  
  miScalar invdet = 1.0f / det;
  dPds.x = (x1 * v2 - v1 * x2) * invdet;
  dPds.y = (y1 * v2 - v1 * y2) * invdet;
  dPds.z = (z1 * v2 - v1 * z2) * invdet;
  dPdt.x = (u1 * x2 - x1 * u2) * invdet;
  dPdt.y = (u1 * y2 - y1 * u2) * invdet;
  dPdt.z = (u1 * z2 - z1 * u2) * invdet;

  
  return miTRUE;
}



//! Calculate dNds and dNdt for the state->point intersection.
//! idx controls the texture vectors.
static
miBoolean dNdst( miVector& dNds, miVector& dNdt,
		 const miState* const state, const int idx = 0 )
{
  //
  // First, obtain the texture coordinates of the the three
  // vertices of the triangle, according to user parameters 
  // (projection type, etc).
  //
  miVector* t[3];
  mi_tri_vectors(const_cast< miState* >(state), 't', idx, &t[0], &t[1], &t[2]);

  
  miScalar u1 = t[1]->x - t[0]->x;
  miScalar u2 = t[2]->x - t[0]->x;
  miScalar v1 = t[1]->y - t[0]->y;
  miScalar v2 = t[2]->y - t[0]->y;
  
  miScalar det = u1 * v2 - v1 * u2;
  if (det == 0.0f)
  {
     // degenerate
     dNds.x = dNdt.y = 1;
     dNds.y = dNds.z = dNdt.x = dNdt.z = 0;
     return miFALSE;
  }

  //
  // Do dNds, dNdt
  //
  miVector N[3];
  get_normals( state, N );
  
  miScalar x1 = N[1].x - N[0].x;
  miScalar x2 = N[2].x - N[0].x;
  miScalar y1 = N[1].y - N[0].y;
  miScalar y2 = N[2].y - N[0].y;
  miScalar z1 = N[1].z - N[0].z;
  miScalar z2 = N[2].z - N[0].z;
  
  miScalar invdet = 1.0f / det;
  dNds.x = (x1 * v2 - v1 * x2) * invdet;
  dNds.y = (y1 * v2 - v1 * y2) * invdet;
  dNds.z = (z1 * v2 - v1 * z2) * invdet;
  dNdt.x = (u1 * x2 - x1 * u2) * invdet;
  dNdt.y = (u1 * y2 - y1 * u2) * invdet;
  dNdt.z = (u1 * z2 - z1 * u2) * invdet;

  return miTRUE;
}


//! Calculate dPds, dPdt, dNds and dNdt for the state->point intersection.
//! idx controls the texture vectors (for when surface has multiple STs)
static
miBoolean dPNdst( 
		  miVector& dPds, miVector& dPdt,
		  miVector& dNds, miVector& dNdt,
		  const miState* const state, const int idx = 0
		  )
{
  //
  // First, obtain the texture coordinates of the the three
  // vertices of the triangle, according to user parameters 
  // (projection type, etc).
  //
  miVector* t[3];
  mi_tri_vectors(const_cast< miState* >(state), 't', idx, &t[0], &t[1], &t[2]);

  
  miScalar u1 = t[1]->x - t[0]->x;
  miScalar u2 = t[2]->x - t[0]->x;
  miScalar v1 = t[1]->y - t[0]->y;
  miScalar v2 = t[2]->y - t[0]->y;
  
  miScalar det = u1 * v2 - v1 * u2;
  if (det == 0.0f)
  {
     // degenerate
     dNds.x = dNdt.y = 1;
     dNds.y = dNds.z = dNdt.x = dNdt.z = 0;
     dPds.x = dPdt.y = 1;
     dPds.y = dPds.z = dPdt.x = dPdt.z = 0;
     return miFALSE;
  }

  //
  // Do dNds, dNdt
  //
  miVector N[3];
  get_normals( state, N );
  
  miScalar x1 = N[1].x - N[0].x;
  miScalar x2 = N[2].x - N[0].x;
  miScalar y1 = N[1].y - N[0].y;
  miScalar y2 = N[2].y - N[0].y;
  miScalar z1 = N[1].z - N[0].z;
  miScalar z2 = N[2].z - N[0].z;
  
  miScalar invdet = 1.0f / det;
  dNds.x = (x1 * v2 - v1 * x2) * invdet;
  dNds.y = (y1 * v2 - v1 * y2) * invdet;
  dNds.z = (z1 * v2 - v1 * z2) * invdet;
  dNdt.x = (u1 * x2 - x1 * u2) * invdet;
  dNdt.y = (u1 * y2 - y1 * u2) * invdet;
  dNdt.z = (u1 * z2 - z1 * u2) * invdet;
     
  //
  // And dPds, dPdt
  // 
  miVector v[3];
  get_vertices( state, v );
  
  x1 = v[1].x - v[0].x;
  x2 = v[2].x - v[0].x;
  y1 = v[1].y - v[0].y;
  y2 = v[2].y - v[0].y;
  z1 = v[1].z - v[0].z;
  z2 = v[2].z - v[0].z;
  
  dPds.x = (x1 * v2 - v1 * x2) * invdet;
  dPds.y = (y1 * v2 - v1 * y2) * invdet;
  dPds.z = (z1 * v2 - v1 * z2) * invdet;
  dPdt.x = (u1 * x2 - x1 * u2) * invdet;
  dPdt.y = (u1 * y2 - y1 * u2) * invdet;
  dPdt.z = (u1 * z2 - z1 * u2) * invdet;

  return miTRUE;
}


//! Calculate dPdu and dPdv for the state->point intersection.
//! idx controls the texture vectors (for when surface has multiple STs)
static
miBoolean dPduv(
		miVector& dPdu, miVector& dPdv,
		const miState* const state
		)
{
   dPdu = state->derivs[0];
   dPdv = state->derivs[1];
   
   miBoolean hasDerivs;
   mi_query( miQ_GEO_HAS_DERIVS, const_cast<miState*>(state),
	     miNULLTAG, &hasDerivs);
   
   if ( ! hasDerivs )
   {
      static std::set<miTag> inst;
      if ( inst.find(state->instance) == inst.end() )
      {
	 mr_warn("Object " << tag2name(state->instance) <<
		 " does not have dPdu, dPdv derivatives.");
	 inst.insert( state->instance );
      }
      return miFALSE;
   }
   
   return miTRUE;
}


//! Get bump basis for a texture index
static
miBoolean BumpUV( 
		 miVector& dPdu, miVector& dPdv,
		 const miState* const state,
		 const int idx = 0
		)
{
   // Check to see if bump derivatives available.  If so, use that instead, as
   // they are more accurate.
   int num;
   mi_query( miQ_NUM_BUMPS, const_cast<miState*>(state), miNULLTAG, &num);
   
   if ( num > idx )
   {
      dPdu = state->bump_x_list[idx];
      dPdv = state->bump_y_list[idx];
      return miTRUE;
   }

   if ( ! dPdst( dPdu, dPdv, state, idx ) ) return miFALSE;
   
   // Make sure tangents form an orthogonal basis with normal.
   // See Ken Turkowski '93
   // http://www.worldserver.com/turk/computergraphics/DifferentialMappings.pdf
   miVector C;
   mi_vector_prod(&C, &state->normal, &dPdu );
   miScalar t2 = dPdu.x * dPdu.x + dPdu.y * dPdu.y + dPdu.z * dPdu.z;
   miScalar NxT2 = C.x * C.x + C.y * C.y + C.z * C.z;
   mi_vector_prod(&dPdu, &C, &state->normal);
   dPdu *= ( t2 / NxT2 );
   
   mi_vector_prod(&C, &state->normal, &dPdv );
   t2 = dPdv.x * dPdv.x + dPdv.y * dPdv.y + dPdv.z * dPdv.z;
   NxT2 = C.x * C.x + C.y * C.y + C.z * C.z;
   mi_vector_prod(&dPdv, &C, &state->normal);
   dPdv *= ( t2 / NxT2 );

   return miTRUE;
}


//! Get second order derivatives d2x/du2, d2x/dv2, d2x/dudv
static
miBoolean dPduv2(
		 miVector& dP2du2, miVector& dP2dv2, miVector& dP2dudv,
		 const miState* const state
		 )
{
   dP2du2 = state->derivs[2];
   dP2dv2 = state->derivs[3];
   dP2dudv = state->derivs[4];
      
   miBoolean hasDerivs;
   mi_query( miQ_GEO_HAS_DERIVS2, const_cast<miState*>(state),
	     miNULLTAG, &hasDerivs);
   if ( !hasDerivs )
   {
      static std::set<miTag> inst;
      if ( inst.find(state->instance) == inst.end() )
      {
	 mr_warn("Object " << tag2name(state->instance) <<
		 " does not have dP2du2, dP2dv2, dP2dudv derivatives.");
	 inst.insert( state->instance );
      }
      return miFALSE;
   }
   
   return miTRUE;
}


//////////////// Add the Vector/Notmal to/fromTangent space

template< class C, typename T >
inline void vec_base< C, T >::fromTangent( const miState* const state,
					   const int idx )
{
   vector dPdu( kNoInit ), dPdv( kNoInit );
   if ( ! dPdut( state, idx, dPdu, dPdv ) ) return;
   dPdu.normalize();
   dPdv.normalize();
   matrix m( dPdu.x, dPdu.y, dPdu.z,
	     dPdv.x, dPdv.y, dPdv.z,
	     state->normal.x, state->normal.y, state->normal.z
	      );
   *this *= m;
}

template< class C, typename T >
inline void vec_base< C, T >::toTangent( const miState* const state,
					 const int idx  )
{
   vector dPdu( kNoInit ), dPdv( kNoInit );
   if ( ! dPduv( dPdu, dPdv, state ) ) return;
   dPdu.normalize();
   dPdv.normalize();
   matrix m( dPdu.x, dPdu.y, dPdu.z,
	     dPdv.x, dPdv.y, dPdv.z,
	     state->normal.x, state->normal.y, state->normal.z
	     );
   m.invert3x3();
   *this *= m;
}

template< class C, typename T >
inline void normal_base< C, T >::fromTangent( const miState* const state,
					      const int idx )
{
   vector dPdu( kNoInit ), dPdv( kNoInit );
   if ( ! dPduv( dPdu, dPdv, state ) ) return;
   dPdu.normalize();
   dPdv.normalize();
   matrix m( dPdu.x, dPdu.y, dPdu.z,
	     dPdv.x, dPdv.y, dPdv.z,
	     state->normal.x, state->normal.y, state->normal.z );
   *this *= m;
}

template< class C, typename T >
inline void normal_base< C, T >::toTangent( const miState* const state,
					    const int idx  )
{
   vector dPdu( kNoInit ), dPdv( kNoInit );
   if ( ! dPduv( dPdu, dPdv, state ) ) return;
   dPdu.normalize();
   dPdv.normalize();
   matrix m( dPdu.x, dPdu.y, dPdu.z,
	     dPdv.x, dPdv.y, dPdv.z,
	     state->normal.x, state->normal.y, state->normal.z );
   m.invert3x3();
   *this *= m;
}

END_NAMESPACE( mr )



#endif // mrDerivs_h

