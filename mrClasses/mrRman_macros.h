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

//!
//! mrRman_macros.h
//!
//! Make sure this is the LAST include you use, as these macros will
//! easily corrupt any other file included afterwards.
//! Also, make sure you DO NOT use any variables with the names of
//! prman built-in variables (s,t,N,P,Ci,etc.)
//!
#ifndef mrRman_macros_h
#define mrRman_macros_h


//! color, needs shader to use color* result in definition
#ifndef undef_Ci
#define Ci (*result)
#endif

//! opacity when in rapid motion, alpha when not.
#ifndef undef_Oi
#define Oi (result->a)
#endif

//! origin of the ray/camera
#ifndef undef_E
#define E  (state->org)
#endif

//! point being shaded
#ifndef undef_P
#define P  (state->point)
#endif

//! interpolated normal (already normalized usually)
#ifndef undef_N
#define N  (state->normal)
#endif

//! geometric normal (already normalized usually)
#ifndef undef_Ng
#define Ng (state->normal_geom)
#endif

//! Note that state->dir is both I and rayinfo("dir")
#ifndef undef_I
#define I  (state->dir)
#endif

//! dPdu is available only for surfaces.
#ifndef undef_dPdu
#define dPdu  (state->bump_list[0])
#endif

//! dPdv is available only for surfaces.
#ifndef undef_dPdv
#define dPdv  (state->bump_list[1])
#endif

//! s and u,  v and t are equal in mray, as mray has no u,v.
#ifndef undef_u
#define u  (state->tex_list[0].x)
#endif

//! s and u,  v and t are equal in mray, as mray has no u,v.
#ifndef undef_v
#define v  (state->tex_list[0].y)
#endif

// I am not going to make s and t macros, as their context
// can be either state->tex_list[0] or state->tex or even
// a shader parameter, which is more common.
// Also, s is often used as shortcut for miState loops, too.
//
//  #ifndef undef_s
//  #define s  (state->tex_list[0].x)
//  #endif

//  #ifndef undef_t
//  #define t  (state->tex_list[0].y)
//  #endif

//! number of components in a color
#ifndef undef_ncomps
#define ncomps  (4)
#endif

//! time of ray
#ifndef undef_time
#define time (state->time)
#endif

//! opacity (alpha) of light
#ifndef undef_Ol
#define Ol  (Cl.a)
#endif

//! motion vector during shutter
#ifndef undef_dPdtime
#define dPdtime  (state->motion)
#endif

// L is not defined as a macro.  Definition changes based on surface or
// light shader.  Ps is not defined, as it is == P, as it should be.

//! Simple function to get an array of anything from a mray
//! parameter definition
#define mr_get_array( iType, iParams, iName ) \
    miInteger i_ ## iName = *mi_eval_integer( iParams->i_ ## iName ); \
    miInteger n_ ## iName = *mi_eval_integer( iParams->n_ ## iName ); \
    const iType*    iName = mr_eval( iParams-> ## iName ) + i_ ## iName;

//! Simple macro to create an illuminance loop.
//! If called multiple times, make sure to encompass it in brackets,
//! so that the local variables are not defined twice.
//!
//! \code
//!
//!  {
//!    // sample lights only within PI/2 of the normal
//!    illuminance( params ) {
//!        samplelight(state) {
//!           ....lots of stuff here...
//!           ....lgt is the light id (loop).
//!        }
//!    }
//!  } //<--- this kills all local variables
//!
//!  {
//!    // sample all lights without taking normal into account
//!    illuminancePI( params ) {
//!        samplelight(state) {
//!           ....lots of stuff here...
//!           ....lgt is the light id (loop).
//!        }
//!    }
//!    state->pri = pri;
//!  }
//!
//! \endcode


//! Pass one parameters to illuminance, to sample iParams->lights.
//! Sample lights only in front of normal.
#define illuminance( iParams ) \
    mr_get_array( miTag, iParams, lights ); \
    mr::color Cl( kNoInit ); miScalar NdL; mr::vector L( kNoInit ); \
    for (int lgt = 0; lgt < n_lights; ++lgt)

//! Pass one parameters to illuminance, to sample iParams->lights.
//! Sample lights in front and behind normal.
#define illuminancePI( iParams ) \
    mr_get_array( miTag, iParams, lights ); \
    void*   pri = state->pri; \
    state->pri  = NULL; \
    mr::color Cl( kNoInit ); miScalar NdL; mr::vector L( kNoInit ); \
    for (int lgt = 0; lgt < n_lights; ++lgt)

//! Pass two parameters to illuminance, to sample iParams->iLights.
//! Sample lights only in front of normal.
#define illuminance2( iParams, iLights ) \
    mr_get_array( miTag, iParams, iLights ); \
    mr::color Cl( kNoInit ); miScalar NdL; mr::vector L( kNoInit ); \
    for (int lgt = 0; lgt < n_lights; ++lgt)

//! Pass one parameters to illuminance, to sample iParams->lights.
//! Sample lights in front and behind normal.
#define illuminance2PI( iParams ) \
    mr_get_array( miTag, iParams, lights ); \
    void*   pri = state->pri; \
    state->pri  = NULL; \
    mr::color Cl( kNoInit ); miScalar NdL; mr::vector L( kNoInit ); \
    for (int lgt = 0; lgt < n_lights; ++lgt)

//!
//! Simple macro to sample a light within an illuminance loop.
//!
//! \code
//!
//!  illuminance( params )
//!  {
//!          samplelight( state )
//!          {
//!          .... do stuff with usual prman variables (Cl, L, etc.) here.
//!          .... note that unlike prman, L is normalized already and
//!          .... NdL already contains N.L unless illuminancePI is used.
//           .... (like in volume shader).
//!          }
//!  }
//!
//! \endcode
#define samplelight( iState ) \
    miInteger samples = 0; \
    while( mi_sample_light( &Cl, &L, &NdL, iState, lights[lgt], &samples ) )

#endif  // mrRman_macros_h
