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
// Split out three new classes from miVector to distinguish
// vectors, points and normals.
// Use of templates is to avoid rewriting code for all classes
// and to also take advantage of templates for binary operators
//
#ifndef mrVector_inl
#define mrVector_inl

#ifdef MR_DEBUG
#define CHECK_NANS \
   mrASSERT( !ISNAN(x) ); \
   mrASSERT( !ISNAN(y) ); \
   mrASSERT( !ISNAN(z) ); 
#else
#define CHECK_NANS
#endif

BEGIN_NAMESPACE( mr )

BEGIN_NAMESPACE( base )


// Conversion to color
template< class C, typename T >
inline vec3< C, T >::operator miColor()
{
   miColor c = { x, y, z, 1.0f };
   return c;
}


template< class C, typename T >
inline vec3< C, T >::vec3() {};

template< class C, typename T >
inline vec3< C, T >::vec3( const T xx, const T yy, const T zz ) 
{ x = xx; y = yy;  z = zz; CHECK_NANS; };

template< class C, typename T >
inline const T vec3< C, T >::Evaluate( const unsigned short i ) const 
{ mrASSERT( i < 3 ); return ((T*)this)[i]; }

template< class C, typename T >
inline T&     vec3< C, T >::operator[] ( const unsigned short i )
{ mrASSERT( i < 3 ); return ((T*)this)[i]; }

template< class C, typename T >
inline const T  vec3< C, T >::operator[] ( const unsigned short i )   const
{ mrASSERT( i < 3 ); return ((T*)this)[i]; }

template< class C, typename T >
inline T  vec3< C, T >::get( const unsigned short i )   const
{ mrASSERT( i < 3 ); return ((T*)this)[i]; }

template< class C, typename T >
inline void  vec3< C, T >::set( const unsigned short i, const T t )
{ mrASSERT( i < 3 ); ((T*)this)[i] = t; CHECK_NANS(); }

template< class C, typename T >
inline void  vec3< C, T >::set( const T xx, const T yy, const T zz )
{ mrASSERT( i < 3 ); x = xx; y = yy; z = zz; CHECK_NANS(); }


template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec3< C, T >&
vec3< C, T >::operator=( const base::exp< X, Y, Oper >& e )
{ 
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) ); 
   CHECK_NANS; return *this; 
}

template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator=( const vec3< C, T >& b )
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS; return *this;
}
    
      
template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator=( const C& b )
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS; return *this;
}

      
template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator=( const T b )
{
   x = y = z = b; CHECK_NANS; return *this;
}

// Equivalence functions...

template< class C, typename T >
template< class X, class Y, class Oper >
inline const bool
vec3< C, T >::operator==( const base::exp< X, Y, Oper >& e ) const
{
   return ( ( x == e.Evaluate(0) ) && 
	    ( y == e.Evaluate(1) ) && 
	    ( z == e.Evaluate(2) ) );
}

template< class C, typename T >
inline const bool
vec3< C, T >::operator==( const vec3< C, T >& b ) const
{
   return ( ( x == b.x ) && ( y == b.y ) && ( z == b.z ) );
}

      
template< class C, typename T >
inline const bool
vec3< C, T >::operator==( const miVector& b ) const
{
   return ( ( x == b.x ) && ( y == b.y ) && ( z == b.z ) );
}
	  

      
template< class C, typename T >
inline const bool
vec3< C, T >::operator==( const T b ) const
{
   return ( ( x == b ) && ( y == b ) && ( z == b ) );
}



template< class C, typename T >
template< class X, class Y, class Oper >
inline const bool
vec3< C, T >::operator!=( const base::exp< X, Y, Oper >& b ) const
{
   return ( ( x != b.Evaluate(0) ) || 
	    ( y != b.Evaluate(1) ) || 
	    ( z != b.Evaluate(2) ) );
}


template< class C, typename T >
inline const bool
vec3< C, T >::operator!=( const vec3< C, T >& b ) const
{
   return ( (x != b.x) || ( y != b.y ) || ( z != b.z ) );
}

      
template< class C, typename T >
inline const bool
vec3< C, T >::operator!=( const miVector& b ) const
{
   return ( (x != b.x) || ( y != b.y ) || ( z != b.z ) );
}

template< class C, typename T >
inline const bool
vec3< C, T >::operator!=( const T b ) const
{
   return ( (x != b) || ( y != b ) || ( z != b ) );
}

template< class C, typename T >
inline vec3< C, T >  vec3< C, T >::lessThan( const C& b )
{
   return vec3< C, T >( x < b.x, y < b.y, z < b.z );
}

template< class C, typename T >
inline vec3< C, T >  vec3< C, T >::lessThanEqual( const C& b )
{
   return vec3< C, T >( x <= b.x, y <= b.y, z <= b.z );
}

template< class C, typename T >
inline vec3< C, T >  vec3< C, T >::greaterThan( const C& b )
{
   return vec3< C, T >( x > b.x, y > b.y, z > b.z );
}

template< class C, typename T >
inline vec3< C, T >  vec3< C, T >::greaterThanEqual( const C& b )
{
   return vec3< C, T >( x >= b.x, y >= b.y, z >= b.z );
}

// to be used mainly with the return vec of
// functions above
template< class C, typename T >
inline const bool vec3< C, T >::any()   
{
   return x || y || z;
}



/////////////////////////// Operators
template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec3< C, T >&
vec3< C, T >::operator+=( const base::exp< X, Y, Oper >& b )
{
   x += static_cast< miScalar >( b.Evaluate(0) ); 
   y += static_cast< miScalar >( b.Evaluate(1) ); 
   z += static_cast< miScalar >( b.Evaluate(2) ); 
   CHECK_NANS; return *this;
}

      
template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator+=( const T b )
{
   x += b; y += b; z += b; CHECK_NANS; return *this;
}

      
template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator+=( const C& b )
{
   x += b.x; y += b.y; z += b.z; CHECK_NANS; return *this;
}




template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec3< C, T >&
vec3< C, T >::operator-=( const base::exp< X, Y, Oper >& b )
{
   x -= static_cast< miScalar >( b.Evaluate(0) ); 
   y -= static_cast< miScalar >( b.Evaluate(1) ); 
   z -= static_cast< miScalar >( b.Evaluate(2) ); 
   CHECK_NANS; return *this;
}


template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator-=( const T b )
{
   x -= b; y -= b; z -= b; CHECK_NANS; return *this;
}

      
template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator-=( const C& b )
{
   x -= b.x; y -= b.y; z -= b.z; CHECK_NANS; return *this;
}




template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec3< C, T >&
vec3< C, T >::operator*=( const base::exp< X, Y, Oper >& b )
{
   x *= static_cast< miScalar >( b.Evaluate(0) ); 
   y *= static_cast< miScalar >( b.Evaluate(1) ); 
   z *= static_cast< miScalar >( b.Evaluate(2) ); 
   CHECK_NANS; return *this;
}


template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator*=( const T b )
{
   x *= b; y *= b; z *= b; CHECK_NANS; return *this;
}

      
template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator*=( const C& b )
{
   x *= b.x; y *= b.y; z *= b.z; CHECK_NANS; return *this;
}





template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec3< C, T >&
vec3< C, T >::operator/=( const base::exp< X, Y, Oper >& b )
{
   x /= static_cast< miScalar >( b.Evaluate(0) ); 
   y /= static_cast< miScalar >( b.Evaluate(1) ); 
   z /= static_cast< miScalar >( b.Evaluate(2) ); 
   CHECK_NANS; return *this;
}


template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator/=( const T b )
{
   mrASSERT( b != 0 );
   T c = static_cast< T >( 1.0 ) / b;
   x *= c; y *= c; z *= c; CHECK_NANS; return *this;
}


template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::operator/=( const C& b )
{
   mrASSERT( (b.x != 0.0f) && ( b.y != 0.0f ) && ( b.z != 0.0f ) );
   x /= b.x; y /= b.y; z /= b.z; CHECK_NANS; return *this;
}




// Common functions
template< class C, typename T >
inline const bool
vec3< C, T >::isEquivalent( const C& b, const T tolerance ) const
{
   return ( (mr::isEquivalent(x, b.x, tolerance))&&
	    (mr::isEquivalent(y, b.y, tolerance))&&
	    (mr::isEquivalent(z, b.z, tolerance)) );
}

template< class C, typename T >
inline const vec3< C, T >&
vec3< C, T >::mix( const C& b, const miScalar p )
{
   if      ( p <= 0.0f ) return *this;
   else if ( p >= 1.0f ) *this = b;
   else 
   {
      x += (b.x - a.x) * p;
      y += (b.y - a.y) * p;
      z += (b.z - a.z) * p;
   }
   CHECK_NANS;
   return *this;
}






#ifdef MR_DEBUG
#undef CHECK_NANS
#define CHECK_NANS \
   mrASSERT( !ISNAN(u) ); \
   mrASSERT( !ISNAN(v) );
#endif




inline const miScalar vec2::Evaluate( const unsigned short i ) const 
{ mrASSERT( i < 2 ); return ((miScalar*)this)[i]; }

inline miScalar&     vec2::operator[] ( const unsigned short i )
{ mrASSERT( i < 2 ); return ((miScalar*)this)[i]; }

inline const miScalar  vec2::operator[] ( const unsigned short i ) const
{ mrASSERT( i < 2 ); return ((miScalar*)this)[i]; }

inline const miScalar  vec2::get( const unsigned short i ) const
{ mrASSERT( i < 2 ); return ((miScalar*)this)[i]; }

inline void  vec2::set( const unsigned short i, const miScalar s )
{ mrASSERT( i < 2 ); ((miScalar*)this)[i] = s; }

inline void  vec2::set( const miScalar uu, const miScalar vv )
{ u = uu; v = vv; }


inline vec2::vec2()
{
   u = v = 0.0f;
}

inline vec2::vec2( const miScalar uu, const miScalar vv )
{
   u = uu; v = vv; CHECK_NANS;
}


template< class X, class Y, class Oper >
inline vec2::vec2( const base::exp< X, Y, Oper >& e )
{
   u = static_cast< miScalar >( e.Evaluate(0) ); 
   v = static_cast< miScalar >( e.Evaluate(1) );  CHECK_NANS;
}

inline vec2::vec2( const miVector2d& b )
{
   u = b.u; v = b.v; CHECK_NANS;
}

inline vec2::vec2( const vec2& b )
{
   u = b.u; v = b.v; CHECK_NANS;
}


template< class X, class Y, class Oper >
inline const vec2& vec2::operator=( const base::exp< X, Y, Oper >& e )
{ 
   u = static_cast< miScalar >( e.Evaluate(0) ); 
   v = static_cast< miScalar >( e.Evaluate(1) ); 
   CHECK_NANS; return *this; 
}

      
inline const vec2& vec2::operator=( const vec2& b )
{
   u = b.u; v = b.v; CHECK_NANS; return *this;
}
    
      
inline const vec2& vec2::operator=( const miVector2d& b )
{
   u = b.u; v = b.v; CHECK_NANS; return *this;
}

      
inline const vec2& vec2::operator=( const miScalar b )
{
   u = v = b; CHECK_NANS; return *this;
}



template< class X, class Y, class Oper >
inline const bool
vec2::operator==( const base::exp< X, Y, Oper >& e ) const
{
   return ( ( u == e.Evaluate(0) ) && ( v == e.Evaluate(1) ) );
}

      
inline const bool vec2::operator==( const vec2& b ) const
{
   return ( ( u == b.u ) && ( v == b.v ) );
}

      
inline const bool vec2::operator==( const miVector2d& b ) const
{
   return ( ( u == b.u ) && ( v == b.v ) );
}
      
inline const bool vec2::operator==( const miScalar b ) const
{
   return ( ( u == b ) && ( v == b ) );
}


template< class X, class Y, class Oper >
inline const bool
vec2::operator!=( const base::exp< X, Y, Oper >& b ) const
{
   return ( ( u != b.Evaluate(0) ) || ( v != b.Evaluate(1) ) );
}

      
inline const bool vec2::operator!=( const vec2& b ) const
{
   return ( ( u != b.u ) || ( v != b.u ) );
}


inline const bool vec2::operator!=( const miVector2d& b ) const
{
   return ( ( u != b.u ) || ( v != b.v ) );
}



inline const bool vec2::operator!=( const miScalar b ) const
{
   return ( ( u != b ) || ( v != b ) );
}


template< class X, class Y, class Oper >
inline const vec2&
vec2::operator+=( const base::exp< X, Y, Oper >& b )
{
   u += static_cast< miScalar >( b.Evaluate(0) ); 
   v += static_cast< miScalar >( b.Evaluate(1) ); 
   CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator+=( const miScalar b )
{
   u += b; v += b; CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator+=( const miVector2d& b )
{
   u += b.u; v += b.v; CHECK_NANS; return *this;
}

///////////////////////////////////////////////////////

template< class X, class Y, class Oper >
inline const vec2&
vec2::operator-=( const base::exp< X, Y, Oper >& b )
{
   u -= static_cast< miScalar >( b.Evaluate(0) ); 
   v -= static_cast< miScalar >( b.Evaluate(1) ); 
   CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator-=( const miScalar b )
{
   u -= b; v -= b; CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator-=( const miVector2d& b )
{
   u -= b.u; v -= b.v; CHECK_NANS; return *this;
}

///////////////////////////////////////////////////////

template< class X, class Y, class Oper >
inline const vec2&
vec2::operator*=( const base::exp< X, Y, Oper >& b )
{
   u *= static_cast< miScalar >( b.Evaluate(0) ); 
   v *= static_cast< miScalar >( b.Evaluate(1) ); 
   CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator*=( const miScalar b )
{
   u *= b; v *= b; CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator*=( const miVector2d& b )
{
   u *= b.u; v *= b.v; CHECK_NANS; return *this;
}

///////////////////////////////////////////////////////

template< class X, class Y, class Oper >
inline const vec2&
vec2::operator/=( const base::exp< X, Y, Oper >& b )
{
   u /= static_cast< miScalar >( b.Evaluate(0) ); 
   v /= static_cast< miScalar >( b.Evaluate(1) ); 
   CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator/=( const miScalar b )
{
   mrASSERT( b != 0 );
   miScalar c = static_cast< miScalar >( 1.0 ) / b;
   u *= c; v *= c; CHECK_NANS; return *this;
}

      
inline const vec2&  vec2::operator/=( const miVector2d& b )
{
   mrASSERT( (b.u != 0.0f) && ( b.v != 0.0f ) );
   u /= b.u; v /= b.v; CHECK_NANS; return *this;
}



// Common functions
inline vec2  vec2::lessThan( const miVector2d& b )
{
   return vec2( u < b.u, v < b.u );
}

inline vec2  vec2::lessThanEqual( const miVector2d& b )
{
   return vec2( u <= b.u, v <= b.v );
}

inline vec2  vec2::greaterThan( const miVector2d& b )
{
   return vec2( u > b.u, v > b.v );
}

inline vec2  vec2::greaterThanEqual( const miVector2d& b )
{
   return vec2( u >= b.u, v >= b.v );
}

inline bool vec2::any()
{
   return u || v;
}

// Common functions
inline const bool
vec2::isEquivalent( const miVector2d& b, const miScalar tolerance ) const
{
   return ( (mr::isEquivalent(u, b.u, tolerance))&&
	    (mr::isEquivalent(v, b.v, tolerance)) );
}

inline const vec2&
vec2::mix( const miVector2d& b, const miScalar p )
{
   if      ( p <= 0.0f ) return *this;
   else if ( p >= 1.0f ) *this = b;
   else 
   {
      u += (b.u - u) * p;
      v += (b.v - v) * p;
   }
   CHECK_NANS;
   return *this;
}


END_NAMESPACE( base )




#ifdef MR_DEBUG
#undef  CHECK_NANS
#define CHECK_NANS \
   mrASSERT( !ISNAN(x) ); \
   mrASSERT( !ISNAN(y) ); \
   mrASSERT( !ISNAN(z) ); 
#endif


//
// Functions common to vectors and normals
//
template< class C, typename T >
inline vecnorm< C, T >::vecnorm() {};

template< class C, typename T >
inline vecnorm< C, T >::vecnorm( const T xx, const T yy, const T zz )
{
   x = xx; y = yy; z = zz;
}


template< class C, typename T >
inline void vecnorm< C, T >::invert()
{
   x = -x; y = -y; z = -z;
}

template< class C, typename T >
inline T   vecnorm< C, T >::lengthSquared() const
{
   return x*x + y*y + z*z;
}

template< class C, typename T >
inline T   vecnorm< C, T >::length() const
{
   return math<T>::sqrt( lengthSquared() );
}


template< class C, typename T >
inline T       vecnorm< C, T >::inverseLength()  const
{
   return math<T>::invsqrt( lengthSquared() );
}

template< class C, typename T >
inline T       vecnorm< C, T >::inverseLengthFast()  const
{
   return fastmath<T>::invsqrt( lengthSquared() );
}

template< class C, typename T >
inline void    vecnorm< C, T >::normalizeFast()
{
   T len = inverseLengthFast();
   x *= len; y *= len; z *= len; CHECK_NANS; 
}

template< class C, typename T >
inline void    vecnorm< C, T >::normalize()
{
   T len = length();
   if ( len > 0.0f )  len = (static_cast< T >( 1.0 )) / len;
   x *= len; y *= len; z *= len; CHECK_NANS; 
}

template< class C, typename T >
inline bool    vecnorm< C, T >::isNormalized() const
{
   T len = lengthSquared();
   return mr::isEquivalent(len, (T)1.0);
}



template< class C, typename T >
template< class X, class Y, class Oper >
inline T vecnorm< C, T >::operator%( const base::exp< X, Y, Oper >& e ) const
{
   return x * e.Evaluate(0) + y * e.Evaluate(1) + z * e.Evaluate(2);
}

    
template< class C, typename T >
inline T vecnorm< C, T >::operator%( const miVector& b ) const
{
   return x * b.x + y * b.y + z * b.z;
}

/////////////////////////////////////////////////////////////////////////


//---------------------------------------------------
// Length comparisons
//---------------------------------------------------
template< class C, typename T >
inline bool     vecnorm< C, T >::operator<( const T a ) const
{
   return ( length() < a ); 
}

template< class C, typename T >
inline bool     vecnorm< C, T >::operator>( const T a ) const
{
   return ( length() > a ); 
}

template< class C, typename T >
inline bool     vecnorm< C, T >::operator<=( const T a ) const
{
   return ( length() <= a ); 
}

template< class C, typename T >
inline bool     vecnorm< C, T >::operator>=( const T a ) const
{
   return ( length() >= a ); 
}



template< class C, typename T >
inline bool     vecnorm< C, T >::operator<( const vecnorm& a ) const
{
   return ( lengthSquared() < a.lengthSquared() ); 
}

template< class C, typename T >
inline bool     vecnorm< C, T >::operator>( const vecnorm& a ) const
{
   return ( lengthSquared() > a.lengthSquared() ); 
}

template< class C, typename T >
inline bool     vecnorm< C, T >::operator<=( const vecnorm& a ) const
{
   return ( lengthSquared() <= a.lengthSquared() ); 
}

template< class C, typename T >
inline bool     vecnorm< C, T >::operator>=( const vecnorm& a ) const
{
   return ( lengthSquared() >= a.lengthSquared() ); 
}


//
// Functions common to vectors only
//
template< class C, typename T >
inline void vec_base< C, T >::fromLight( const miState* const state )
{
   mi_vector_from_light( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::fromWorld( const miState* const state )
{
   mi_vector_from_world( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::fromObject( const miState* const state )
{
   mi_vector_from_object( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::fromCamera( const miState* const state )
{
   mi_vector_from_camera( const_cast< miState* >( state ), this, this );
}


template< class C, typename T >
inline void vec_base< C, T >::toLight( const miState* const state )
{
   mi_vector_to_light( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::toWorld( const miState* const state )
{
   mi_vector_to_world( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::toObject( const miState* const state )
{
   mi_vector_to_object( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::toCamera( const miState* const state )
{
   mi_vector_to_camera( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void vec_base< C, T >::toRaster( const miState* const state )
{
   // this transforms a vector to raster space
   vec_base< C, T> p1( kNoInit );  vec_base< C, T> p2( state->point + *this );
   mi_point_to_raster( const_cast< miState* >( state ), &p1, 
		       const_cast< miVector* >(&state->point) );
   mi_point_to_raster( const_cast< miState* >( state ), &p2, &p2 );
   *this = p2 - p1;
}

template< class C, typename T >
inline void vec_base< C, T >::toNDC( const miState* const state )
{
   //@todo:  toNDC takes precendence over raster, this could be faster
   //        if I do my own matrix calculation.
   toRaster( state );
   x /= state->camera->x_resolution;
   y /= state->camera->y_resolution;
}

template< class C, typename T >
inline void vec_base< C, T >::to( const miState* const state, 
				  const space::type toSpace )
{
   switch ( toSpace ) 
   {
      case space::kObject:
	 toObject( state ); break;
      case space::kWorld:
	 toWorld( state ); break;
      case space::kCamera:
	 toCamera( state ); break;
      case space::kRaster:
	 toRaster( state ); break;
      case space::kNDC:
	 toNDC( state ); break;
      case space::kLight:
	 toLight( state ); break;
      case space::kTangent:
	 toTangent( state ); break;
      case space::kInternal:
	 break;
      case space::kScreen:
	 mi_warning("Cannot transform a vector to that space.");
	 break;
      case space::kUnknown:
      default:
	 break;
   }
}

template< class C, typename T >
inline void vec_base< C, T >::from( const miState* const state, 
				    const space::type fromSpace )
{
   switch ( fromSpace ) 
   {
      case space::kObject:
	 fromObject( state ); break;
      case space::kWorld:
	 fromWorld( state ); break;
      case space::kCamera:
	 fromCamera( state ); break;
      case space::kLight:
	 fromLight( state ); break;
      case space::kTangent:
	 fromTangent( state ); break;
      case space::kInternal:
	 break;
      case space::kScreen:
      case space::kNDC:
      case space::kRaster:
	 mi_warning("Cannot transform a vector from that space.");
	 break;
      case space::kUnknown:
      default:
	 break;
   }
}

template< class C, typename T >
inline void vec_base< C, T >::transform( const miState* const state, 
					 const space::type fromSpace,
					 const space::type toSpace )
{
   from( state, fromSpace );
   to( state, toSpace );
}



template< class C, typename T >
inline vec_base< C, T >::vec_base( kNoConstruct x )
{}

template< class C, typename T >
inline vec_base< C, T >::vec_base() 
{ x = y = z = static_cast< T >( 0 ); };


template< class C, typename T >
template< class X, class Y, class Oper >
inline vec_base< C, T >::vec_base( const base::exp< X, Y, Oper >& e )
{
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) );
   CHECK_NANS;
}

template< class C, typename T >
inline vec_base< C, T >::vec_base( const vec_base< C, T >& b ) 
{
   x = b.x; y = b.y; z = b.z;  CHECK_NANS;
}


template< class C, typename T >
inline vec_base< C, T >::vec_base( const C& b ) 
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
}

template< class C, typename T >
inline vec_base< C, T >::vec_base( const T b ) 
{
   x = y = z = b;  CHECK_NANS;
}

template< class C, typename T >
inline vec_base< C, T >::vec_base( const T xx, 
				   const T yy, 
				   const T zz ) 
{
   x = xx; y = yy; z = zz; CHECK_NANS;
}

template< class C, typename T >
inline vec_base< C, T >::vec_base( const miState* const state, 
				   const space::type fromSpace, 
				   const C& v )
{
   x = v.x; y = v.y; z = v.z; CHECK_NANS;
   from( state, fromSpace ); CHECK_NANS;
};


template< class C, typename T >
inline vec_base< C, T >::vec_base( const miState* const state, 
				   const space::type fromSpace, 
				   const T xx, const T yy, const T zz )
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
   from( state, fromSpace ); CHECK_NANS;
};

template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec_base< C, T >&
vec_base< C, T >::operator=( const base::exp< X, Y, Oper >& e )
{ 
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) ); 
   CHECK_NANS; return *this;
}

template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator=( const vec_base< C, T >& b )
{
   x = b.x; y = b.y; z = b.z;
   CHECK_NANS; return *this;
}

template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator=( const C& b )
{
   x = b.x; y = b.y; z = b.z;
   CHECK_NANS; return *this;
}


template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator=( const T b )
{
   x = y = z = b; CHECK_NANS; return *this;
}

template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator*= ( const T b )
{
   x *= b; y *= b; z *= b; CHECK_NANS; return *this;
}

template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator*=( const C& b )
{
   x *= b.x; y *= b.y; z *= b.z; CHECK_NANS; return *this;
}

template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator*=( const miMatrix m )
{
   miScalar ox = x, oy = y;
   x  = ox * m[0] + oy * m[4] + z * m[8];
   y  = ox * m[1] + oy * m[5] + z * m[9];
   z *= m[10];
   z += ox * m[2] + oy * m[6]; CHECK_NANS;
   return *this;
}

template< class C, typename T >
inline const vec_base< C, T >&  
vec_base< C, T >::operator*=( const matrix& m )
{
   return this->operator*=( &m );
}

template< class C, typename T >
inline vec_base< C, T >  
vec_base< C, T >::operator*( const miMatrix m ) const
{
   return vec_base< C, T >( x * m[0] + y * m[4] + z * m[8],
			    x * m[1] + y * m[5] + z * m[9], 
			    x * m[2] + y * m[6] + z * m[10] );
}

template< class C, typename T >
inline vec_base< C, T > 
vec_base< C, T >::operator*( const matrix& m ) const
{
   return this->operator*( &m );
}


template< class C, typename T >
inline vec_base< C, T > vec_base< C, T >::inverse() const
{
   return vec_base< C, T >( -x, -y, -z );
}

template< class C, typename T >
inline vec_base< C, T > vec_base< C, T >::operator-() const
{
   return vec_base< C, T >( -x, -y, -z );
}

template< class C, typename T >
inline vec_base< C, T >  vec_base< C, T >::normalized()  const
{
   mrASSERT( length() != 0.0f );
   T len = (static_cast< T >(1.0 )) / length();
   return vec_base< C, T >( x * len, y * len, z * len );
}

template< class C, typename T >
inline vec_base< C, T >  vec_base< C, T >::normalizedFast()  const
{
   T len = inverseLengthFast();
   return vec_base< C, T >( x * len, y * len, z * len );
}



template< class C, typename T >
template< class X, class Y, class Oper >
inline vec_base< C, T >
vec_base< C, T >::operator^( const base::exp< X, Y, Oper >& b ) const
{
   vec_base c( b.Evaluate(0), b.Evaluate(1), b.Evaluate(2) );
   return vec_base< C, T >( y * c.z - z * c.y,
			    z * c.x - x * c.z,
			    x * c.y - y * c.x );
}
   
template< class C, typename T >
inline vec_base< C, T > vec_base< C, T >::operator^( const miVector& b ) const
{
   return vec_base< C, T >( y * b.z - z * b.y,
			    z * b.x - x * b.z,
			    x * b.y - y * b.x );
}


template< class C, typename T >
template< class X, class Y, class Oper >
inline const vec_base< C, T >&
vec_base< C, T >::operator^=( const base::exp< X, Y, Oper >& b )
{
   *this = this->operator^( b );
   CHECK_NANS; return *this;
}

    
template< class C, typename T >
inline const vec_base< C, T >&
vec_base< C, T >::operator^=( const miVector& b )
{
   *this = this->operator^( b );
   CHECK_NANS; return *this;
}

template< class C, typename T >
template< typename X >
inline const vec_base< C, T >& vec_base< C, T >::cross( const X& b )
{
   return this->operator^=(b);
}

//
// Functions common to normals only
//


template< class C, typename T >
inline void normal_base< C, T >::fromLight( const miState* const state )
{
   mi_normal_from_light( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::fromWorld( const miState* const state )
{
   mi_normal_from_world( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::fromObject( const miState* const state )
{
   mi_normal_from_object( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::fromCamera( const miState* const state )
{
   mi_normal_from_camera( const_cast< miState* >( state ), this, this );
}


template< class C, typename T >
inline void normal_base< C, T >::toLight( const miState* const state )
{
   mi_normal_to_light( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::toWorld( const miState* const state )
{
   mi_normal_to_world( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::toObject( const miState* const state )
{
   mi_normal_to_object( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::toCamera( const miState* const state )
{
   mi_normal_to_camera( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void normal_base< C, T >::toRaster( const miState* const state )
{
   normal_base< C, T > p1( kNoInit );
   normal_base< C, T > p2( state->point + *this );
   mi_point_to_raster( const_cast< miState* >( state ), &p1, 
		       const_cast< miVector* >(&state->point) );
   mi_point_to_raster( const_cast< miState* >( state ), &p2, &p2 );
   *this = p2 - p1;
}

template< class C, typename T >
inline void normal_base< C, T >::toNDC( const miState* const state )
{
   toRaster( state );
   x /= state->camera->x_resolution;
   y /= state->camera->y_resolution; CHECK_NANS;
}

template< class C, typename T >
inline void normal_base< C, T >::to( const miState* const state, 
				     const space::type toSpace )
{
   switch ( toSpace ) 
   {
      case space::kObject:
	 toObject( state ); break;
      case space::kWorld:
	 toWorld( state ); break;
      case space::kCamera:
	 toCamera( state ); break;
      case space::kRaster:
	 toRaster( state ); break;
      case space::kNDC:
	 toNDC( state ); break;
      case space::kLight:
	 toLight( state ); break;
      case space::kInternal:
	 break;
      case space::kScreen:
	 mi_warning("Cannot transform a normal to that space.");
	 break;
      case space::kUnknown:
      default:
	 break;
   }
}

template< class C, typename T >
inline void normal_base< C, T >::from( const miState* const state, 
				       const space::type fromSpace )
{
   switch ( fromSpace ) 
   {
      case space::kObject:
	 fromObject( state ); break;
      case space::kWorld:
	 fromWorld( state ); break;
      case space::kCamera:
	 fromCamera( state ); break;
      case space::kLight:
	 fromLight( state ); break;
      case space::kInternal:
	 break;
      case space::kScreen:
      case space::kNDC:
      case space::kRaster:
	 mi_warning("Cannot transform a normal from that space.");
	 break;
      case space::kUnknown:
      default:
	 break;
   }
}

template< class C, typename T >
inline void normal_base< C, T >::transform( const miState* const state, 
					    const space::type fromSpace,
					    const space::type toSpace )
{
   from( state, fromSpace );
   to( state, toSpace ); CHECK_NANS;
}



template< class C, typename T >
inline normal_base< C, T >::normal_base( kNoConstruct x )
{}

template< class C, typename T >
inline normal_base< C, T >::normal_base() 
{ x = y = z = static_cast< T >( 0 ); };


template< class C, typename T >
template< class X, class Y, class Oper >
inline normal_base< C, T >::normal_base( const base::exp< X, Y, Oper >& e )
{
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) );  CHECK_NANS;
}

template< class C, typename T >
inline normal_base< C, T >::normal_base( const normal_base< C, T >& b ) 
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
}


template< class C, typename T >
inline normal_base< C, T >::normal_base( const C& b ) 
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
}

template< class C, typename T >
inline normal_base< C, T >::normal_base( const T b ) 
{
   x = y = z = b; CHECK_NANS;
}

template< class C, typename T >
inline normal_base< C, T >::normal_base( const T xx, 
					 const T yy, 
					 const T zz ) 
{
   x = xx; y = yy; z = zz; CHECK_NANS;
}


template< class C, typename T >
inline normal_base< C, T >::normal_base( const miState* const state, 
					 const space::type fromSpace, 
					 const T xx, const T yy, 
					 const T zz )
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
   from( state, fromSpace ); CHECK_NANS;
}


template< class C, typename T >
inline normal_base< C, T >::normal_base( const miState* const state, 
					 const space::type fromSpace, 
					 const C& v )
{
   x = v.x; y = v.y; z = v.z; CHECK_NANS;
   from( state, fromSpace ); CHECK_NANS;
}



template< class C, typename T >
template< class X, class Y, class Oper >
inline const normal_base< C, T >&
normal_base< C, T >::operator=( const base::exp< X, Y, Oper >& e )
{ 
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) ); 
   CHECK_NANS; return *this;
}

template< class C, typename T >
inline const normal_base< C, T >&
normal_base< C, T >::operator=( const normal_base< C, T >& b )
{
   x = b.x; y = b.y; z = b.z;
   CHECK_NANS; return *this;
}

template< class C, typename T >
inline const normal_base< C, T >&
normal_base< C, T >::operator=( const C& b )
{
   x = b.x; y = b.y; z = b.z;
   CHECK_NANS; return *this;
}


template< class C, typename T >
inline const normal_base< C, T >&
normal_base< C, T >::operator=( const T b )
{
   x = y = z = b; CHECK_NANS; return *this;
}



template< class C, typename T >
inline const normal_base< C, T >&
normal_base< C, T >::operator*=( const T b )
{
   x *= b; y *= b; z *= b; CHECK_NANS; return *this;
}


template< class C, typename T >
inline const normal_base< C, T >&
normal_base< C, T >::operator*=( const C& b )
{
   x *= b.x; y *= b.y; z *= b.z; CHECK_NANS; return *this;
}



template< class C, typename T >
inline const normal_base< C, T >&  
normal_base< C, T >::operator*=( const miMatrix a )
{
   miScalar ox = x, oy = y;
   x  = ox * m[0] + oy * m[1] + z * m[2];
   y  = ox * m[4] + oy * m[5] + z * m[6];
   z *= m[10];
   z += ox * m[8] + oy * m[9];
}


template< class C, typename T >
inline normal_base< C, T >   
normal_base< C, T >::operator*( const miMatrix m ) const
{
   return normal_base< C, T >( x * m[0] + y * m[1] + z * m[2],
			       x * m[4] + y * m[5] + z * m[6], 
			       x * m[8] + y * m[9] + z * m[10] );
}

template< class C, typename T >
inline const normal_base< C, T >& 
normal_base< C, T >::operator*=( const matrix& m )
{
   return this->operator*=( &m );
}

template< class C, typename T >
inline normal_base< C, T > 
normal_base< C, T >::operator*( const matrix& m ) const
{
   return this->operator*( &m );
}


template< class C, typename T >
inline normal_base< C, T > normal_base< C, T >::inverse() const
{
   return normal_base< C, T >( -x, -y, -z );
}

template< class C, typename T >
inline normal_base< C, T > normal_base< C, T >::operator-() const
{
   return normal_base< C, T >( -x, -y, -z );
}

template< class C, typename T >
inline normal_base< C, T >  normal_base< C, T >::normalized()  const
{
   mrASSERT( length() != 0.0f );
   T len = (static_cast< T >(1.0 )) / length();
   return normal_base< C, T >( x * len, y * len, z * len );
}

template< class C, typename T >
inline normal_base< C, T >  normal_base< C, T >::normalizedFast()  const
{
   T len = inverseLengthFast();
   return normal_base< C, T >( x * len, y * len, z * len );
}

template< class C, typename T >
template< class X, class Y, class Oper >
inline normal_base< C, T >
normal_base< C, T >::operator^( const base::exp< X, Y, Oper >& b ) const
{
   normal_base c( b.Evaluate(0), b.Evaluate(1), b.Evaluate(2) );
   return normal_base< C, T >( y * c.z - z * c.y,
			   z * c.x - x * c.z,
			   x * c.y - y * c.x );
}
   
template< class C, typename T >
inline normal_base< C, T >
normal_base< C, T >::operator^( const miVector& b ) const
{
   return normal_base< C, T >( y * b.z - z * b.y,
			       z * b.x - x * b.z,
			       x * b.y - y * b.x );
}



template< class C, typename T >
template< class X, class Y, class Oper >
inline const normal_base< C, T >&
normal_base< C, T >::operator^=( const base::exp< X, Y, Oper >& b )
{
   *this = this->operator^( b );
   CHECK_NANS; return *this;
}

    
template< class C, typename T >
inline const normal_base< C, T >&
normal_base< C, T >::operator^=( const miVector& b )
{
   *this = this->operator^( b );
   CHECK_NANS; return *this;
}

template< class C, typename T >
template< typename X >
inline const normal_base< C, T >& normal_base< C, T >::cross( const X& b )
{
   return this->operator^=(b);
}

//
// Functions common to points only
//

template< class C, typename T >
inline void point_base< C, T >::fromLight( const miState* const state )
{
   mi_point_from_light( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::fromWorld( const miState* const state )
{
   mi_point_from_world( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::fromObject( const miState* const state )
{
   mi_point_from_object( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::fromCamera( const miState* const state )
{
   mi_point_from_camera( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::fromScreen( const miState* const state )
{
   mi_warning("fromScreen not working properly yet.");
   
   matrix m( state );
   m.inverse();
   *this *= m;
   
   //@todo:  need to do my own math here to calculate z properly
   CHECK_NANS;
   mi_point_from_camera( const_cast< miState* >( state ), this, this );
   CHECK_NANS;
}

template< class C, typename T >
inline void point_base< C, T >::fromRaster( const miState* const state )
{
   //@todo:  need to do my own math here...
   x /= state->camera->x_resolution;
   y /= state->camera->y_resolution;
   x = x * 2.0f - 1.0f;
   y = y * 2.0f - 1.0f;
   // We are in screen space now...

   fromScreen( state );
   CHECK_NANS;
}

template< class C, typename T >
inline void point_base< C, T >::fromNDC( const miState* const state )
{
   x = x * 2.0f - 1.0f;
   y = y * 2.0f - 1.0f; CHECK_NANS;
   fromScreen( state ); CHECK_NANS;
}

template< class C, typename T >
inline void point_base< C, T >::toLight( const miState* const state )
{
  mi_point_to_light( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::toWorld( const miState* const state )
{
  mi_point_to_world( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::toObject( const miState* const state )
{
  mi_point_to_object( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::toCamera( const miState* const state )
{
  mi_point_to_camera( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::toScreen( const miState* const state )
{
   mi_point_to_raster( const_cast< miState* >( state ), this, this );
   x /= state->camera->x_resolution;
   y /= state->camera->y_resolution;
   x = x * 2.0f - 1.0f;
   y = y * 2.0f - 1.0f;
   CHECK_NANS;
}

template< class C, typename T >
inline void point_base< C, T >::toRaster( const miState* const state )
{
  mi_point_to_raster( const_cast< miState* >( state ), this, this );
}

template< class C, typename T >
inline void point_base< C, T >::toNDC( const miState* const state )
{
  mi_point_to_raster( const_cast< miState* >( state ), this, this );
  x /= state->camera->x_resolution;
  y /= state->camera->y_resolution; CHECK_NANS;
}


template< class C, typename T >
inline void point_base< C, T >::to( const miState* const state, 
				    const space::type toSpace )
{
  switch ( toSpace ) 
    {
    case space::kObject:
      toObject( state ); break;
    case space::kWorld:
      toWorld( state ); break;
    case space::kCamera:
      toCamera( state ); break;
    case space::kRaster:
      fromRaster( state ); break;
    case space::kNDC:
      fromNDC( state ); break;
    case space::kScreen:
      fromScreen( state ); break;
    case space::kLight:
      toLight( state ); break;
    case space::kInternal:
       break;
    case space::kUnknown:
    default:
      break;
    }
}

template< class C, typename T >
inline void point_base< C, T >::from( const miState* const state, 
				      const space::type fromSpace )
{
  switch ( fromSpace ) 
    {
    case space::kObject:
      fromObject( state ); break;
    case space::kWorld:
      fromWorld( state ); break;
    case space::kCamera:
      fromCamera( state ); break;
    case space::kRaster:
      fromRaster( state ); break;
    case space::kNDC:
      fromNDC( state ); break;
    case space::kScreen:
      fromScreen( state ); break;
    case space::kLight:
      fromLight( state ); break;
      break;
    case space::kInternal:
       break;
    case space::kUnknown:
    default:
      break;
    }
}

template< class C, typename T >
inline void point_base< C, T >::transform( const miState* const state, 
					   const space::type fromSpace,
					   const space::type toSpace )
{
  from( state, fromSpace ); CHECK_NANS;
  to( state, toSpace ); CHECK_NANS;
}


template< class C, typename T >
inline point_base< C, T >::point_base( kNoConstruct x )
{}

template< class C, typename T >
inline point_base< C, T >::point_base() 
{ x = y = z = static_cast< T >( 0 ); };


template< class C, typename T >
template< class X, class Y, class Oper >
inline point_base< C, T >::point_base( const base::exp< X, Y, Oper >& e )
{
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) ); 
}

template< class C, typename T >
inline point_base< C, T >::point_base( const point_base< C, T >& b ) 
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
}


template< class C, typename T >
inline point_base< C, T >::point_base( const C& b ) 
{
   x = b.x; y = b.y; z = b.z; CHECK_NANS;
}

template< class C, typename T >
inline point_base< C, T >::point_base( const T b ) 
{
   x = y = z = b; CHECK_NANS;
}

template< class C, typename T >
inline point_base< C, T >::point_base( const T xx, const T yy, const T zz ) 
{
  x = xx; y = yy; z = zz; CHECK_NANS;
}

template< class C, typename T >
inline point_base< C, T >::point_base( const miState* const state, 
				       const space::type toSpace, 
				       const T xx, const T yy, const T zz ) 
{
  x = b.x; y = b.y; z = b.z; CHECK_NANS;
  to( state, toSpace ); CHECK_NANS;
};



template< class C, typename T >
inline point_base< C, T >::point_base( const miState* const state, 
				       const space::type fromSpace, 
				       const C& v )
{
  x = v.x; y = v.y; z = v.z; CHECK_NANS;
  from( state, fromSpace ); CHECK_NANS;
};




template< class C, typename T >
template< class X, class Y, class Oper >
inline const point_base< C, T >&
point_base< C, T >::operator=( const base::exp< X, Y, Oper >& e )
{ 
   x = static_cast< T >( e.Evaluate(0) ); 
   y = static_cast< T >( e.Evaluate(1) ); 
   z = static_cast< T >( e.Evaluate(2) ); 
   CHECK_NANS; return *this;
}

template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator=( const point_base< C, T >& b )
{
   x = b.x; y = b.y; z = b.z;
   CHECK_NANS; return *this;
}

template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator=( const C& b )
{
   x = b.x; y = b.y; z = b.z;
   CHECK_NANS; return *this;
}


template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator=( const T b )
{
  x = y = z = b; CHECK_NANS; return *this;
}




template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator*=( const T b )
{
  x *= b; y *= b; z *= b; CHECK_NANS; return *this;
}

template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator*=( const C& b )
{
  x *= b.x; y *= b.y; z *= b.z; CHECK_NANS; return *this;
}

template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator*=( const miMatrix m )
{
  miScalar w = x * m[3] + y * m[7] + z * m[11] + m[15];

  mrASSERT( w != 0.0f );

  float ox = x, oy = y;
  x  = ox * m[0] + oy * m[4] + z * m[8] + m[12];
  y  = ox * m[1] + oy * m[5] + z * m[9] + m[13];
  z *= m[10];
  z += ox * m[2] + oy * m[6] + m[14];
  if ( w != 1.0f ) {
    w = 1.0f/w;
    x *= w; y *= w; z *= w;
  }

  CHECK_NANS; return *this;
}

template< class C, typename T >
inline point_base< C, T > 
point_base< C, T >::operator*( const miMatrix m ) const
{
  miScalar w = x * m[3] + y * m[7] + z * m[11] + m[15];

  mrASSERT( w != 0.0f );

  if ( w == 1.0f )
    {
      return point_base< C, T >( x * m[0] + y * m[4] + z * m[8]  + m[12],
				 x * m[1] + y * m[5] + z * m[9]  + m[13], 
				 x * m[2] + y * m[6] + z * m[10] + m[14] );
    }
  else
    {
      w = 1.0f/w;
      return point_base< C, T >( w * (x * m[0] + y * m[4] + z * m[8]  + m[12]),
				 w * (x * m[1] + y * m[5] + z * m[9]  + m[13]), 
				 w * (x * m[2] + y * m[6] + z * m[10] + m[14]) );
    }
}

template< class C, typename T >
inline const point_base< C, T >&
point_base< C, T >::operator*=( const matrix& m )
{
  return this->operator*=( &m );
}

template< class C, typename T >
inline point_base< C, T >
point_base< C, T >::operator*( const matrix& m ) const
{
  //return this->operator*( static_cast< miMatrix >( &m ) );
  return this->operator*( &m );
}




//
// Static Constants
//

template< class C, typename T >
vec_base< C, T >
vec_base< C, T >::kNull = vec_base< C, T >( (T)0.0, (T)0.0, (T)0.0 );

template< class C, typename T >
vec_base< C, T >
vec_base< C, T >::kAxisX = vec_base< C, T >( (T)1.0, (T)0.0, (T)0.0 );

template< class C, typename T >
vec_base< C, T >
vec_base< C, T >::kAxisY = vec_base< C, T >( (T)0.0, (T)1.0, (T)0.0 );

template< class C, typename T >
vec_base< C, T >
vec_base< C, T >::kAxisZ = vec_base< C, T >( (T)0.0, (T)0.0, (T)1.0 );

template< class C, typename T >
vec_base< C, T >
vec_base< C, T >::kUnitScale = vec_base< C, T >( (T)1.0f, (T)1.0f, (T)1.0f );



template< class C, typename T >
point_base< C, T >
point_base< C, T >::kOrigin = point_base< C, T >( (T)0.0f, (T)0.0f, (T)0.0f );


END_NAMESPACE( mr )



#undef CHECK_NANS


#endif  // mrVector_inl
