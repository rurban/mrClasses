/******************************************************************************
 * Created:	07.05.03
 * Module:	gg_ambientocclusion
 *
 * Exports:
 *      gg_ambientocclusion()
 *
 * Requires:
 *      mrClasses
 *
 * History:
 *      07.05.03: initial version
 *
 * Description:
 *      Create an ambient occlusion (final gather) pass with/without
 *      normals calculation.
 *      Normals are spitted out in the normal buffer (using an output
 *      shader, like gg_buffers).
 *
 *****************************************************************************/

#include "mrGenerics.h"
#include "mrRman.h"
#include "mray_transparency.h"
#include "mrRman_macros.h"

using namespace mr;
using namespace rsl;


struct gg_ambientocclusion_t
{
  miBoolean blocker;
  miInteger nearSamples;
  miScalar  nearDistance;
  miInteger farSamples;
  miScalar  farDistance;
  miScalar  angle;
  miScalar  minFalloff;
  miScalar  maxFalloff;
  miBoolean calculateNormal;
  miBoolean useProbes;
  miScalar  transparency;
  miVector  normalCamera;
  miInteger normalSpace;
};


struct shaderCache
{
  miBoolean       blocker;
  miUint      nearSamples;
  miScalar   nearDistance;
  miUint       farSamples;
  miScalar    farDistance;
  miScalar  spherePercent;
  miScalar         cosine;
  space::type normalSpace;
  miBoolean    calcNormal;
  miBoolean         useFG;

  miBoolean     useProbes;

  double maxFalloff;
  double minFalloff;
};




EXTERN_C DLLEXPORT int gg_ambientocclusion_version(void) {return(1);}



EXTERN_C DLLEXPORT void
gg_ambientocclusion_init(
			    miState* const        state,
			    struct gg_ambientocclusion_t* p,
			    miBoolean* req_inst
			    )
{
   if ( !p ) {  // global shader init, request per instance init
      if ( state->options->finalgather )
	 mi_info("gg_ambientocclusion: using finalgather for "
		 "occlusion cache.");
      *req_inst = miTRUE; return;
   }


  // Cache all non-shading network parameters 
  // (like 'uniform' variables)
  shaderCache* cache = new shaderCache;
 
  cache->calcNormal = ( state->type == miRAY_EYE      ?
			mr_eval( p->calculateNormal ) :
			miFALSE );


  cache->blocker = mr_eval( p->blocker );
  if ( cache->blocker ) return;

  cache->nearSamples = (miUint) mr_eval( p->nearSamples );
  cache->farSamples  = (miUint) mr_eval( p->farSamples );


  cache->nearDistance = mr_eval( p->nearDistance );
  cache->farDistance  = mr_eval( p->farDistance  );

  if ( cache->nearDistance > cache->farDistance )
    {
      std::swap( cache->nearDistance, cache->farDistance );
      std::swap( cache->nearSamples,  cache->farSamples  );
    }

  float angle = mr_eval( p->angle );
  cache->spherePercent = angle / 360.0f;

  cache->cosine     = math<float>::cos( radians(angle) * 0.5f );
  cache->cosine     = 1.0f - cache->cosine;


  cache->useFG      = state->options->finalgather > 0? miTRUE : miFALSE;

  cache->maxFalloff = static_cast< double >(mr_eval( p->maxFalloff ));
  cache->minFalloff = static_cast< double >(mr_eval( p->minFalloff ));

  if ( cache->maxFalloff < cache->minFalloff )
    {
      std::swap( cache->minFalloff, cache->maxFalloff );
    }
  
  if ( state->options->reflection_depth < 2 ) cache->useProbes = miTRUE;
  else cache->useProbes  = mr_eval( p->useProbes );

  if ( cache->useFG && cache->maxFalloff )
    {
      state->options->fg_falloff_start = 
		  static_cast< miScalar >( cache->minFalloff );
      state->options->fg_falloff_stop  = 
		  static_cast< miScalar >( cache->maxFalloff );
    }

  cache->normalSpace = (space::type)mr_eval( p->normalSpace );

  void **user;
  mi_query(miQ_FUNC_USERPTR, state, 0, &user);
  *user = cache;

}



EXTERN_C DLLEXPORT void
gg_ambientocclusion_exit(
			 miState* const        state,
			 struct gg_ambientocclusion_t* p
			 )
{
  if ( !p ) return;

  void **user;
  mi_query(miQ_FUNC_USERPTR, state, 0, &user);
  delete *user;
}



EXTERN_C DLLEXPORT miBoolean 
gg_ambientocclusion(
		    color* const result,
		    miState* const state,
		    struct gg_ambientocclusion_t* p
		    )
{
  miScalar Ti = mr_eval( p->transparency );
  result->a   = 1.0f - Ti;


  // if finalgather or reflection ray, get out of here quickly...
  if ( (state->type == miRAY_FINALGATHER) || state->reflection_level || 
       (state->type == miRAY_SHADOW) )
    { 
      Ci = result->a;
      trace_transparent( result, state );
      return miTRUE;
    }

  // Get user cache (ie. uniform variables)
  void **user;
  mi_query(miQ_FUNC_USERPTR, state, 0, &user);
  shaderCache* cache = static_cast< shaderCache* >(*user);

  // Early exit if totally transparent or a blocker
  if ( (result->a < miSCALAR_EPSILON) || ( cache->blocker ) )
    {
      Ci = 0.0f;
      trace_transparent( result, state );
      return miTRUE;
    }

  // Do bump mapping
  const vector& Nbump  = mr_eval( p->normalCamera );
  if ( Nbump != 0.0f )
    {
      N = Nbump;
      state->dot_nd = state->dir % Nbump; 
    }


  double minFalloff = cache->minFalloff;
  double maxFalloff = cache->maxFalloff;
  if ( maxFalloff )
    mi_ray_falloff( state, &minFalloff, &maxFalloff );

  miColor hitColor;
  normal avgNormal( kNoInit );
  float hits;

  if ( cache->useFG )
    {
      mi_compute_irradiance( &hitColor, state );
      
      hits = 1.0f - hitColor.r / static_cast< float >( M_PI );


      if ( cache->calcNormal )
	{
	   // Based on distance travelled, figure out how many samples to use
	   miScalar totaldist = static_cast< float >( state->dist );
	   miState* st = state;
	   while ( st = st->parent )
	      totaldist += static_cast< float >( st->dist );

	   miScalar x = linear( cache->nearDistance, cache->farDistance, 
				totaldist );
	   miUint   samples = mix( cache->nearSamples, cache->farSamples, x );
	   
	   sphereSampler g( N, samples, cache->spherePercent );
	   
	   avgNormal = 0.0f;
	   
	   if ( cache->useProbes )
	   {
	      while ( g.uniform( state ) )
	      {
		 if ( !mi_trace_probe( state, &g.direction(),
				       &state->point ) )
		    avgNormal += g.direction();
	      }
	   }
	   else
	   {
	      while ( g.uniform( state ) )
	      {
		 if ( !mi_trace_reflection( &hitColor, state,
					    &g.direction() ) )
		    avgNormal += g.direction();
	      }
	   }
	}
    }
  else
    {

      // Based on distance travelled, figure out how many samples to use
      miScalar totaldist = static_cast< float >( state->dist );
      miState* st = state;
      while ( st = st->parent )
	totaldist += static_cast< float >( st->dist );

      miScalar x = linear( cache->nearDistance, cache->farDistance, 
			   totaldist );


      miUint   samples = mix( cache->nearSamples, cache->farSamples, x );

      hits = 0.0f;

      if ( cache->calcNormal )
	{
	  sphereSampler g( N, samples, cache->spherePercent );
	  avgNormal = 0.0f;

	  if ( cache->useProbes )
	    {
	      if ( cache->maxFalloff > cache->minFalloff )
		{
		  while ( g.uniform( state ) )
		    {
		      if ( mi_trace_probe( state, &g.direction(),
					   &state->point ) )
			hits += 1.0f - 
				static_cast< float >( 
					linear( cache->minFalloff,
						cache->maxFalloff, 
						state->child->dist ) 
					);
		      else
			avgNormal += g.direction();
		    }
		}
	      else
		{
		  while ( g.uniform( state ) )
		    {
		      if ( mi_trace_probe( state, &g.direction(),
					   &state->point ) )
			++hits;
		      else
			avgNormal += g.direction();
		    }
		}
	    }
	  else
	    {
	      while ( g.uniform( state ) )
		{
		  if ( mi_trace_reflection( &hitColor, state,
					    &g.direction() )  )
		    hits += hitColor.r;
		  else
		    avgNormal += g.direction();
		}
	    }

	  hits /= g.count();
	}
      else
	{

	  sphereSampler g( N, samples, cache->spherePercent );
	  if ( cache->useProbes )
	    {
	      if ( cache->maxFalloff > cache->minFalloff )
		{
		  while ( g.uniform( state ) )
		    {
		      if ( mi_trace_probe( state, &g.direction(),
					   &state->point ) )
			hits += 1.0f - static_cast< float >( 
				linear( cache->minFalloff, cache->maxFalloff, 
					state->child->dist ) );
		    }
		}
	      else
		{
		  while ( g.uniform( state ) )
		    {
		      if ( mi_trace_probe( state, &g.direction(),
					   &state->point ) )
		      {
			++hits;
		      }
		    }
		}
	    }
	  else
	    {
	      while ( g.uniform( state ) )
		{
		  if ( mi_trace_reflection( &hitColor, state,
					    &g.direction() )  )
		    hits += hitColor.r;
		}
	    }

	  hits /= g.count();
	}

      hits = 1.0f - hits;
    }

  // restore old falloff
  if ( cache->maxFalloff )
    mi_ray_falloff( state, &minFalloff, &maxFalloff );



  Ci = hits * Oi;
  
  if ( cache->calcNormal )
    {
       avgNormal.normalize();
       avgNormal -= N;

       if ( cache->normalSpace != space::kInternal )
       {
	  avgNormal.to( state, cache->normalSpace );
	  avgNormal.normalize();
       }

       state->normal.x = 0.5f * avgNormal.x + 0.5f;
       state->normal.y = 0.5f * avgNormal.y + 0.5f;
       state->normal.z = 0.5f * avgNormal.z + 0.5f;
    }
  
  trace_transparent( result, state );
  return miTRUE;
}
