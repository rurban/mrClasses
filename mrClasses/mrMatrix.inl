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
// mrMatrix.inl
//
// A class to work with matrices which uses miMatrix 
// at its core.
//
 
#ifndef mrMatrix_inl
#define mrMatrix_inl


BEGIN_NAMESPACE( mr )

//
// miScalar = _det2x2( miScalar a, miScalar b, miScalar c, miScalar d )
// 
// calculate the determinant of a 2x2 matrix.
//     | a, c |
//     | b, d |
//

inline
miScalar matrix::_det2x2( miScalar a, miScalar b, miScalar c, miScalar d) const
{
  return (a * d - b * c);
}

//
// miScalar = _det3x3(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
// 
// calculate the determinant of a 3x3 matrix
// in the form
//
//     | a1,  b1,  c1 |
//     | a2,  b2,  c2 |
//     | a3,  b3,  c3 |
//

inline
miScalar matrix::_det3x3( miScalar a1,  miScalar a2,  miScalar a3, 
			  miScalar b1,  miScalar b2,  miScalar b3, 
			  miScalar c1,  miScalar c2,  miScalar c3 ) const
{
  return (   a1 * _det2x2( b2, b3, c2, c3 )
	     - b1 * _det2x2( a2, a3, c2, c3 )
	     + c1 * _det2x2( a2, a3, b2, b3 ) );
}


inline void matrix::setToIdentity() 
{
  _m[1] = _m[2] = _m[3] =
    _m[4] = _m[6] = _m[7] = 
    _m[8] = _m[9] = _m[11] = 
    _m[12] = _m[13] = _m[14] = 0.0f;
  _m[0] = _m[5] = _m[10] = _m[15] = 1.0f;
}

inline void matrix::setToNull() 
{
  mi_matrix_null( &_m );
}

inline void matrix::setInternalToCamera( miState* const state ) 
{
  miMatrix *m1;
  mi_query( miQ_TRANS_INTERNAL_TO_CAMERA, state, miNULLTAG, &m1 );
  *this = *m1;
}

inline void matrix::setInternalToWorld( miState* const state ) 
{
  miMatrix *m1;
  mi_query( miQ_TRANS_INTERNAL_TO_WORLD, state, miNULLTAG, &m1 );
  *this = *m1;
}

inline void matrix::setInternalToObject( miState* const state ) 
{
  miMatrix *m1;
  mi_query( miQ_TRANS_INTERNAL_TO_OBJECT, state, miNULLTAG, &m1 );
  *this = *m1;
}

inline void matrix::setCameraToInternal( miState* const state ) 
{
  miMatrix *m1;
  mi_query( miQ_TRANS_CAMERA_TO_INTERNAL, state, miNULLTAG, &m1 );
  *this = *m1;
}

inline void matrix::setCameraToWorld( miState* const state ) 
{
  miMatrix *m1;
  mi_query( miQ_TRANS_CAMERA_TO_WORLD, state, miNULLTAG, &m1 );
  *this = *m1;
}

inline void matrix::setCameraToObject( miState* const state ) 
{
  miMatrix *m1, *m2;

  mi_query( miQ_TRANS_CAMERA_TO_INTERNAL, state, miNULLTAG, (void*) &m1 );
  mi_query( miQ_TRANS_INTERNAL_TO_OBJECT, state, miNULLTAG, (void*) &m2 );

  mi_matrix_prod( _m, *m1, *m2 );
}

inline void matrix::setObjectToInternal( miState* const state ) 
{
  miMatrix* m1;
  mi_query( miQ_TRANS_OBJECT_TO_INTERNAL, state, miNULLTAG, &m1 );
  *this = *m1;
}

inline void matrix::setObjectToWorld( miState* const state ) 
{
  miMatrix* m1;
  mi_query( miQ_TRANS_OBJECT_TO_WORLD, state, miNULLTAG, &m1 );
  *this = *m1;
}


inline void matrix::setObjectToCamera( miState* const state ) 
{
  miMatrix *m1, *m2;

  mi_query( miQ_TRANS_OBJECT_TO_INTERNAL, state, miNULLTAG, &m1 );
  mi_query( miQ_TRANS_INTERNAL_TO_CAMERA, state, miNULLTAG, &m2 );

  mi_matrix_prod( _m, *m1, *m2 );
}


inline void matrix::orthographic(
				 const miScalar near_plane, 
				 const miScalar far_plane,
				 const miScalar left_plane, 
				 const miScalar right_plane, 
				 const miScalar top_plane, 
				 const miScalar bottom_plane
			       )
{
   setToNull();
   _m[0]  = 2.0f / (right_plane - left_plane);
   _m[5]  = 2.0f / (top_plane - bottom_plane);
   _m[10] = -2.0f / (far_plane - near_plane);
   _m[12] = (right_plane + left_plane) / (right_plane - left_plane);
   _m[13] = (top_plane + bottom_plane) / (top_plane - bottom_plane);
   _m[14] = (far_plane + near_plane) / (far_plane - near_plane);
}


inline void matrix::orthographic( const miState* const state )
{
   if (!state->camera->orthographic) return projection(state);
   miCamera* c = state->camera;
   miScalar aperture = c->aperture;
   miScalar height = aperture / c->aspect;
   miScalar w = aperture * 0.5f;
   miScalar h = height * 0.5f;
   projection( c->focal, c->clip.max, -w, w, h, -h );
}


inline void matrix::projection(
			       const miScalar near_plane, 
			       const miScalar far_plane,
			       const miScalar left_plane, 
			       const miScalar right_plane, 
			       const miScalar top_plane, 
			       const miScalar bottom_plane
			       )
{
   setToNull();
   _m[0]  = 2.0f * near_plane / (right_plane - left_plane);
   _m[5]  = 2.0f * near_plane / (top_plane - bottom_plane);
   _m[8]  = (right_plane + left_plane) / (right_plane - left_plane);
   _m[9]  = (top_plane + bottom_plane) / (top_plane - bottom_plane);
   _m[10] = -(far_plane + near_plane) / (far_plane - near_plane);
   _m[11] = -1.0f;
   _m[14] = 2.0f * far_plane * near_plane / (far_plane - near_plane);
}


inline void matrix::projection( const miState* const state )
{
   if (state->camera->orthographic) return orthographic(state);
   
   miCamera* c = state->camera;
   miScalar aperture = c->aperture;
   miScalar height = aperture / c->aspect;
   miScalar w = aperture * 0.5f;
   miScalar h = height * 0.5f;
   projection( c->focal, c->clip.max, -w, w, h, -h );
}



//
// CONSTRUCTORS
//
inline matrix::matrix( kNoConstruct x ) 
{
}

inline matrix::matrix()
{ 
  setToIdentity(); 
}


inline matrix::matrix( miState* const state, const MatrixType type )
{ 
   switch(type) 
   {
      case kIdentity:
	 setToIdentity(); break;
      case kEmpty:
	 setToNull();    break;
      case kInternalToWorld:
	 setInternalToWorld(state); break;
      case kInternalToCamera:
	 setInternalToCamera(state); break;
      case kInternalToObject:
	 setInternalToObject(state); break;
      case kObjectToWorld:
	 setObjectToWorld(state); break;
      case kObjectToInternal:
	 setObjectToInternal(state); break;
      case kObjectToCamera:
	 setObjectToCamera(state); break;
      case kCameraToWorld:
	 setCameraToWorld(state); break;
      case kCameraToInternal:
	 setCameraToInternal(state); break;
      case kCameraToObject:
	 setCameraToObject(state); break;
      case kCameraProjection:
	 projection(state); break;
      default:
	 mi_fatal("matrix::matrix(state,type) Unknown type of matrix.");
   }
}

inline matrix::matrix( const MatrixType type )
{ 
   switch(type) 
   {
      case kIdentity:
	 setToIdentity(); break;
      case kEmpty:
	 setToNull();    break;
      case kInternalToWorld:
      case kInternalToCamera:
      case kInternalToObject:
      case kCameraToWorld:
      case kCameraToInternal:
      case kCameraToObject:
      case kObjectToWorld:
      case kObjectToInternal:
      case kObjectToCamera:   
      case kCameraProjection:
	 mi_fatal("matrix::matrix() Need to use matrix(state,matrixType).");
	 break;
      default:
	 mi_fatal("matrix::matrix() Unknown type of matrix.");
   }
}

inline matrix::matrix( const miScalar m[16] ) 
{ 
  memcpy( &_m, m, sizeof( miMatrix ) );
}

inline matrix::matrix( const miInteger a )
{ 
	mrASSERT( a == 0 || a == 1 );
  _m[1] = _m[2] = _m[3] =
    _m[4] = _m[6] = _m[7] = 
    _m[8] = _m[9] = _m[11] = 
    _m[12] = _m[13] = _m[14] = 
    _m[0] = _m[5] = _m[10] = _m[15] = (miScalar) a;
}

inline matrix::matrix( const matrix& m  ) 
{ 
  memcpy( &_m, m._m, sizeof( miMatrix ) );
}

inline 
matrix::matrix( const miScalar m00, const miScalar m01, const miScalar m02,
		const miScalar m10, const miScalar m11, const miScalar m12,
		const miScalar m20, const miScalar m21, const miScalar m22 
		)
{
  _m[ 0] = m00;  _m[ 1] = m01;  _m[ 2] = m02;  _m[ 3] = 0.0f;
  _m[ 4] = m10;  _m[ 5] = m11;  _m[ 6] = m12;  _m[ 7] = 0.0f;
  _m[ 8] = m20;  _m[ 9] = m21;  _m[10] = m22;  _m[11] = 0.0f;
  _m[12] = 0.0f; _m[13] = 0.0f; _m[14] = 0.0f; _m[15] = 1.0f;
}

inline
matrix::matrix( const miScalar m00, const miScalar m01, 
		const miScalar m02, const miScalar m03, 
	  
		const miScalar m10, const miScalar m11, 
		const miScalar m12, const miScalar m13, 
		  
		const miScalar m20, const miScalar m21, 
		const miScalar m22, const miScalar m23,
	  
		const miScalar m30, const miScalar m31, 
		const miScalar m32, const miScalar m33
		)
{
  _m[ 0] = m00;  _m[ 1] = m01;  _m[ 2] = m02;  _m[ 3] = m03;
  _m[ 4] = m10;  _m[ 5] = m11;  _m[ 6] = m12;  _m[ 7] = m13;
  _m[ 8] = m20;  _m[ 9] = m21;  _m[10] = m22;  _m[11] = m23;
  _m[12] = m30;  _m[13] = m31;  _m[14] = m32;  _m[15] = m33;
}



inline matrix::matrix( const miState* const state )
{
   projection(state);
}





inline matrix& matrix::operator= ( const matrix& m )   
{ 
  memcpy( &_m, m._m, sizeof( miMatrix ) );
  return *this;
}

inline matrix& matrix::operator= ( const miMatrix m )   
{ 
  memcpy( &_m, m, sizeof( miMatrix ) );
  return *this;
}

inline matrix& matrix::operator= ( const MatrixType type ) 
{ 
  switch(type) 
    {
    case kIdentity:
      setToIdentity(); break;
    case kEmpty:
      setToNull();    break;
    default:
      mi_fatal("matrix::matrix() Unknown type of matrix.");
    }
  return *this;
}

inline matrix& matrix::operator= ( const miInteger a )
{ 
  return matrix::operator=((MatrixType) a); 
}

inline const miScalar* matrix::operator[]( const unsigned row ) const
{
   mrASSERT( row < 4 );
   return &(_m[row*4]);
}

inline miScalar* matrix::operator[]( const unsigned row )
{
  mrASSERT( row < 4 );
  return &(_m[row*4]);
}

inline miScalar matrix::get( const unsigned idx ) const
{
   mrASSERT( idx < 16 );
   return _m[idx];
}

inline void matrix::set( const unsigned idx,
			 const miScalar s )
{
   mrASSERT( idx < 16 );
   _m[idx] = s;
}


inline matrix matrix::transposed3x3 () const
{ 
  return matrix(
		_m[0], _m[4],  _m[8],
		_m[1], _m[5],  _m[9],
		_m[2], _m[6], _m[10]
		);
}

inline matrix matrix::transposed () const
{ 
  return matrix(
		_m[0], _m[4],  _m[8], _m[12],
		_m[1], _m[5],  _m[9], _m[13],
		_m[2], _m[6], _m[10], _m[14],
		_m[3], _m[7], _m[11], _m[15]
		);
}


inline const matrix& matrix::transpose3x3 ()
{ 
  *this = this->transposed3x3();
  return *this;
}

inline const matrix& matrix::transpose ()
{ 
  *this = this->transposed();
  return *this;
}

inline bool matrix::operator!=( const matrix& a ) const 
{ 
  return ((  _m[0] != a._m[0] )||(   _m[1] != a._m[1] )||
	  (  _m[2] != a._m[2] )||(   _m[3] != a._m[3] )||
	  (  _m[4] != a._m[4] )||(   _m[5] != a._m[5] )||
	  (  _m[6] != a._m[6] )||(   _m[7] != a._m[7] )||
	  (  _m[8] != a._m[8] )||(   _m[9] != a._m[9] )||
	  ( _m[10] != a._m[10])||( _m[11] != a._m[11] )||
	  ( _m[12] != a._m[12])||( _m[13] != a._m[13] )||
	  ( _m[14] != a._m[14])||( _m[15] != a._m[15] )); 
}

inline bool matrix::operator==( const matrix& a ) const 
{ 
  return ((  _m[0] == a._m[0] )&&(   _m[1] == a._m[1] )&&
	  (  _m[2] == a._m[2] )&&(   _m[3] == a._m[3] )&&
	  (  _m[4] == a._m[4] )&&(   _m[5] == a._m[5] )&&
	  (  _m[6] == a._m[6] )&&(   _m[7] == a._m[7] )&&
	  (  _m[8] == a._m[8] )&&(   _m[9] == a._m[9] )&&
	  ( _m[10] == a._m[10])&&( _m[11] == a._m[11] )&&
	  ( _m[12] == a._m[12])&&( _m[13] == a._m[13] )&&
	  ( _m[14] == a._m[14])&&( _m[15] == a._m[15] )); 
}



inline matrix& matrix::operator*= ( const miScalar m ) 
{ 
  _m[ 0] *= m;  _m[ 1] *= m;  _m[ 2] *= m;  _m[ 3] *= m;
  _m[ 4] *= m;  _m[ 5] *= m;  _m[ 6] *= m;  _m[ 7] *= m;
  _m[ 8] *= m;  _m[ 9] *= m;  _m[10] *= m;  _m[11] *= m;
  _m[12] *= m;  _m[13] *= m;  _m[14] *= m;  _m[15] *= m;
  return *this;
}


inline matrix& matrix::operator*=( const miMatrix a ) 
{ 
   mi_matrix_prod( _m, _m, a );  // this is a macro
   return *this; 
}


inline matrix& matrix::operator*=( const matrix& a ) 
{ 
   mi_matrix_prod( _m, _m, a._m );  // this is a macro
   return *this; 
}


inline matrix& matrix::operator/= ( const miScalar scalar ) 
{ 
  if ( scalar == 0.0f ) return *this;
  register miScalar d = 1.0f / scalar;
  *this *= d;
  return *this;
}


inline matrix& matrix::operator/=( const miMatrix a ) 
{ 
  matrix b ( a );
  mi_matrix_prod( _m, _m, (miScalar*) &(b.inverse()) ); // this is a macro
  return *this; 
}


inline matrix& matrix::operator/=( const matrix& a ) 
{ 
  mi_matrix_prod( _m, _m, (miScalar*) &(a.inverse()) ); // this is a macro
  return *this; 
}


inline matrix& matrix::operator+=( const miMatrix a ) 
{ 
  for (register int i = 0; i < 16; ++i)
    _m[i] += a[i];
  return *this; 
}


inline matrix& matrix::operator+=( const matrix& b ) 
{ 
  for (register int i = 0; i < 16; ++i)
    _m[i] += b._m[i];
  return *this; 
}


inline matrix& matrix::operator-=( const matrix& b ) 
{ 
  for (register int i = 0; i < 16; ++i)
    _m[i] -= b._m[i];
  return *this; 
}


inline matrix& matrix::operator-=( const miMatrix a ) 
{ 
  for (register int i = 0; i < 16; ++i)
    _m[i] -= a[i];
  return *this; 
}





inline matrix matrix::operator- () const
{ 
  matrix a; 
  miBoolean ok = mi_matrix_invert( (miScalar*)&a, _m ); 
  if ( !ok )
    mi_warning("matrix::operator- could not invert matrix.");
  return a;
}


inline matrix matrix::operator* ( const miScalar scalar ) const
{ 
  matrix a( *this );
  a *= scalar;
  return a; 
}


inline matrix matrix::operator* ( const miMatrix a ) const 
{ 
  matrix x;
  mi_matrix_prod( (miScalar*)&x, _m, a );
  return x; 
}


inline matrix matrix::operator* ( const matrix& a ) const 
{ 
  matrix x;
  mi_matrix_prod( (miScalar*)&x, _m, (miScalar*)&a );
  return x; 
}


inline matrix matrix::operator/ ( const miScalar scalar ) const
{ 
  if ( scalar == 0 ) return *this;
  miScalar d = 1.0f/scalar;
  matrix a( *this ); a *= d;
  return a; 
}


inline matrix matrix::operator/ ( const miMatrix b ) const
{ 
  matrix a( *this ); a /= b; return a; 
}


inline matrix matrix::operator/ ( const matrix& b ) const
{ 
  matrix a( *this ); a /= b; return a; 
}


inline matrix matrix::operator+ ( const miMatrix b ) const
{ 
  matrix a( *this ); a += b; return a; 
}


inline matrix matrix::operator+ ( const matrix& b ) const
{ 
  matrix a( *this ); a += b; return a; 
}


inline matrix matrix::operator- ( const miMatrix b ) const
{ 
  matrix a( *this ); a -= b; return a; 
}


inline matrix matrix::operator- ( const matrix& b ) const
{ 
  matrix a( *this ); a -= b; return a; 
}

inline std::ostream& operator<< ( std::ostream& o, const matrix& a )
{
  using namespace std;
  int col = o.tellp();  col = col >= 0 ? col+2 : 2;
  return o << "| " 
	   << setw(14) << setprecision(5) << a._m[0] << "|"
	   << setw(14) << setprecision(5) << a._m[1] << "|"
	   << setw(14) << setprecision(5) << a._m[2] << "|"
	   << setw(14) << setprecision(5) << a._m[3] << " |" 
	   << endl << setw(col)
	   << "| " 
	   << setw(14) << setprecision(5) << a._m[4] << "|"
	   << setw(14) << setprecision(5) << a._m[5] << "|"
	   << setw(14) << setprecision(5) << a._m[6] << "|"
	   << setw(14) << setprecision(5) << a._m[7] << " |"
	   << endl << setw(col)
	   << "| " 
	   << setw(14) << setprecision(5) << a._m[8] << "|"
	   << setw(14) << setprecision(5) << a._m[9] << "|"
	   << setw(14) << setprecision(5) << a._m[10] << "|"
	   << setw(14) << setprecision(5) << a._m[11] << " |"
	   << endl << setw(col)
	   << "| " 
	   << setw(14) << setprecision(5) << a._m[12] << "|"
	   << setw(14) << setprecision(5) << a._m[13] << "|"
	   << setw(14) << setprecision(5) << a._m[14] << "|"
	   << setw(14) << setprecision(5) << a._m[15] << " |";
}


inline matrix operator*( const miScalar scalar, const matrix& b )
{
  matrix r ( b );
  r *= scalar;
  return r;
}


//
// Matrix Inversion
// by Richard Carling
// from "Graphics Gems", Academic Press, 1990
//

inline
miScalar matrix::det4x4() const
{
  miScalar a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  a1 = _m[0]; b1 = _m[1]; 
  c1 = _m[2]; d1 = _m[3];

  a2 = _m[4]; b2 = _m[5]; 
  c2 = _m[6]; d2 = _m[7];

  a3 = _m[8]; b3 = _m[9]; 
  c3 = _m[10]; d3 = _m[11];

  a4 = _m[12]; b4 = _m[13]; 
  c4 = _m[14]; d4 = _m[15];

  return (   a1 * _det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
	     - b1 * _det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
	     + c1 * _det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
	     - d1 * _det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4) );
}


// 
//   adjoint( A=original_matrix, B=inverse_matrix )
// 
//     calculate the adjoint of a 4x4 matrix
//
//      Let  a   denote the minor determinant of matrix A obtained by
//           ij
//
//      deleting the ith row and jth column from A.
//
//                    i+j
//     Let  b   = (-1)    a
//          ij            ji
//
//    The matrix B = (b  ) is the adjoint of A
//                     ij
//

inline matrix matrix::adjoint() const
{
  register miScalar a1, a2, a3, a4, b1, b2, b3, b4;
  register miScalar c1, c2, c3, c4, d1, d2, d3, d4;

  matrix out( kNoInit );

  a1 = _m[0]; b1 = _m[1]; 
  c1 = _m[2]; d1 = _m[3];

  a2 = _m[4]; b2 = _m[5]; 
  c2 = _m[6]; d2 = _m[7];

  a3 = _m[8]; b3 = _m[9]; 
  c3 = _m[10]; d3 = _m[11];

  a4 = _m[12]; b4 = _m[13]; 
  c4 = _m[14]; d4 = _m[15];


  // row column labeling reversed since we transpose rows & columns
  miScalar* x = out[ (unsigned) 0];
  float t = out[0][0];

  out[0][0]  =   _det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
  out[1][0]  = - _det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
  out[2][0]  =   _det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
  out[3][0]  = - _det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
        
  out[0][1]  = - _det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
  out[1][1]  =   _det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
  out[2][1]  = - _det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
  out[3][1]  =   _det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
  out[0][2]  =   _det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
  out[1][2]  = - _det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
  out[2][2]  =   _det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
  out[3][2]  = - _det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
  out[0][3]  = - _det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
  out[1][3]  =   _det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
  out[2][3]  = - _det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
  out[3][3]  =   _det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);

  return out;
}

//
//   inverse()
//
//    calculate the inverse of a 4x4 matrix
//
//     -1     
//     A  = ___1__ adjoint A
//          det A
//

inline matrix matrix::inverse() const
{
  matrix out( adjoint() );

  //  calculate the 4x4 determinant
  //         if the determinant is zero, 
  //         then the inverse matrix is not unique.
  miScalar det = det4x4();

  if ( math<float>::fabs( det ) < miSCALAR_EPSILON ) {
    mi_warning("mr::matrix::inverse() could not invert matrix.");
    out.setToIdentity();
    return out;
  }

  // scale the adjoint matrix to get the inverse
  out /= det;

  return out;
}

inline const matrix&  matrix::invert ()
{ 
  *this = this->inverse(); return *this;
}






//
// 3x3 determinant
//
inline
miScalar matrix::det3x3() const
{
  return _det3x3( _m[0], _m[1], _m[2],
		  _m[4], _m[5], _m[6],
		  _m[8], _m[9], _m[10] );
}

// 
//   adjoint3x3()
//
//     calculate the adjoint of a 3x3 matrix
//
//      Let  a   denote the minor determinant of matrix A obtained by
//           ij
//
//      deleting the ith row and jth column from A.
//
//                    i+j
//     Let  b   = (-1)    a
//          ij            ji
//
//    The matrix B = (b  ) is the adjoint of A
//                     ij
//

inline matrix matrix::adjoint3x3() const
{
  register miScalar a1, a2, a3, b1, b2, b3, c1, c2, c3;

  matrix out( kNoInit );

  // assign to individual variable names to aid
  // selecting correct values

  a1 = _m[0]; b1 = _m[1]; 
  c1 = _m[2];

  a2 = _m[4]; b2 = _m[5]; 
  c2 = _m[6]; 

  a3 = _m[8]; b3 = _m[9];
  c3 = _m[10]; 

  // row column labeling reversed since we transpose rows & columns

  out[0][0]  =   _det2x2( b2, b3, c2, c3);
  out[1][0]  = - _det2x2( a2, a3, c2, c3);
  out[2][0]  =   _det2x2( a2, a3, b2, b3);
  out[3][0]  =  0;
        
  out[0][1]  = - _det2x2( b1, b3, c1, c3);
  out[1][1]  =   _det2x2( a1, a3, c1, c3);
  out[2][1]  = - _det2x2( a1, a3, b1, b3);
  out[3][1]  =  0;
        
  out[0][2]  =   _det2x2( b1, b2, c1, c2);
  out[1][2]  = - _det2x2( a1, a2, c1, c2);
  out[2][2]  =   _det2x2( a1, a2, b1, b2);
  out[3][2]  =  0;

  out[0][3]  =
    out[1][3]  =
    out[2][3]  =  0;
  out[3][3]  =  1;

  return out;
}

//
//   inverse3x3()
//
//   Calculate the inverse of a 3x3 matrix, from GemsI
//
//     -1
//     A  = __1__ adjoint A
//          det A
//

inline matrix matrix::inverse3x3() const
{
  matrix out( adjoint3x3() );

  //  calculate the 3x3 determinant
  //         if the determinant is zero, 
  //         then the inverse matrix is not unique.
  miScalar det = det3x3();

  if ( math<float>::fabs( det ) < miSCALAR_EPSILON ) {
    mi_warning("mr::matrix::inverse3x3() could not invert matrix.");
    out.setToIdentity();
    return out;
  }

  // scale the adjoint matrix to get the inverse
  det  = 1.0f/det;
  out *= det;
  return out;
}

inline const matrix& matrix::invert3x3 ()
{ 
  *this = this->inverse3x3(); return *this;
}



inline bool  matrix::isNull () const
{ 
  return ( (_m[0] == 0)&&(_m[1] == 0)&&
	   (_m[2] == 0)&&(_m[3] == 0)&&
	   (_m[4] == 0)&&(_m[5] == 0)&&
	   (_m[6] == 0)&&(_m[7] == 0)&&
	   (_m[8] == 0)&&(_m[9] == 0)&&
	   (_m[10] == 0)&&(_m[11] == 0)&&
	   (_m[12] == 0)&&(_m[13] == 0)&&
	   (_m[14] == 0)&&(_m[15] == 0)
	   );
}

inline bool  matrix::isIdentity () const
{ 
  return ( (_m[0] == 1)&&(_m[1] == 0)&&
	   (_m[2] == 0)&&(_m[3] == 0)&&
	   (_m[4] == 0)&&(_m[5] == 1)&&
	   (_m[6] == 0)&&(_m[7] == 0)&&
	   (_m[8] == 0)&&(_m[9] == 0)&&
	   (_m[10] == 1)&&(_m[11] == 0)&&
	   (_m[12] == 0)&&(_m[13] == 0)&&
	   (_m[14] == 0)&&(_m[15] == 1)
	   );
}




inline void matrix::translate( const miScalar x, const miScalar y,
			       const miScalar z )
{
  _m[12] += x * _m[0] + y * _m[4] + z * _m[8];
  _m[13] += x * _m[1] + y * _m[5] + z * _m[9];
  _m[14] += x * _m[2] + y * _m[6] + z * _m[10];
}

inline void matrix::translate( const miVector v )
{
  translate( v.x, v.y, v.z );
}


inline void matrix::rotate( const miScalar x, const miScalar y,
			    const miScalar z )
{
  miScalar cos_rx = math<miScalar>::cos(x);
  miScalar cos_ry = math<miScalar>::cos(y);
  miScalar cos_rz = math<miScalar>::cos(z);

  miScalar sin_rx = math<miScalar>::sin(x);
  miScalar sin_ry = math<miScalar>::sin(y);
  miScalar sin_rz = math<miScalar>::sin(z);

  miScalar m00, m01, m02;
  miScalar m10, m11, m12;
  miScalar m20, m21, m22;

  m00 =  cos_rz * cos_ry;  
  m01 =  sin_rz * cos_ry;  
  m02 = -sin_ry;
  m10 = -sin_rz *  cos_rx + cos_rz * sin_ry * sin_rx;
  m11 =  cos_rz *  cos_rx + sin_rz * sin_ry * sin_rx;
  m12 =  cos_ry *  sin_rx;
  m20 = -sin_rz * -sin_rx + cos_rz * sin_ry * cos_rx;
  m21 =  cos_rz * -sin_rx + sin_rz * sin_ry * cos_rx;
  m22 =  cos_ry *  cos_rx;

  matrix P ( *this );
  _m[0] = P[0][0] * m00 + P[1][0] * m01 + P[2][0] * m02;
  _m[1] = P[0][1] * m00 + P[1][1] * m01 + P[2][1] * m02;
  _m[2] = P[0][2] * m00 + P[1][2] * m01 + P[2][2] * m02;

  _m[4] = P[0][0] * m10 + P[1][0] * m11 + P[2][0] * m12;
  _m[5] = P[0][1] * m10 + P[1][1] * m11 + P[2][1] * m12;
  _m[6] = P[0][2] * m10 + P[1][2] * m11 + P[2][2] * m12;

  _m[8] = P[0][0] * m20 + P[1][0] * m21 + P[2][0] * m22;
  _m[9] = P[0][1] * m20 + P[1][1] * m21 + P[2][1] * m22;
  _m[10]= P[0][2] * m20 + P[1][2] * m21 + P[2][2] * m22;

}

inline void matrix::rotate( const miVector v )
{
  rotate( v.x, v.y, v.z );
}

inline void matrix::scale( const miScalar x, const miScalar y,
			   const miScalar z )
{
  if ( x != 1.0f ) {
    _m[0] *= x;
    _m[1] *= x;
    _m[2] *= x;
  }
  if ( y != 1.0f ) {
    _m[4] *= y;
    _m[5] *= y;
    _m[6] *= y;
  }
  if ( z != 1.0f ) {
    _m[8] *= z;
    _m[9] *= z;
    _m[10]*= z;
  }
}

inline void matrix::scale    ( const miScalar s )
{
  scale( s, s, s );
}

inline void matrix::scale    ( const miVector& v )
{
  scale( v.x, v.y, v.z );
}


END_NAMESPACE( mr )


#endif // mrMatrix_inl
