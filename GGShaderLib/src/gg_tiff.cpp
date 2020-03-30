/******************************************************************************
 * Created:	07.05.03
 * Module:	gg_tiff
 *
 * Exports:
 *      gg_tiff()
 *      gg_ctiff()
 *
 * Requires:
 *      mrClasses
 *
 * History:
 *      07.05.03: initial version
 *
 * Description:
 *      Tiff returning either a scalar or a color.
 *
 *****************************************************************************/

#ifndef MR_NO_TIFF

#include "tiffio.h"
#include "mrGenerics.h"
#include "mrTiff.h"
#include "mrRman.h"
#include "mrRman_macros.h"

//! Constant for the texture cache (x 1024 bytes)
const miUint kMEMORY_LIMIT = 16384;


using namespace mr;
using namespace rsl;

struct gg_tiff_t
{
     miTag     filename;
     miInteger   filter;
     miInteger  samples;
     miScalar     sblur;
     miScalar     tblur;
     miScalar    swidth;
     miScalar    twidth;
     miInteger  channel;
     miScalar      fill;
};

struct gg_ctiff_t
{
     miTag     filename;
     miInteger   filter;
     miInteger  samples;
     miScalar     sblur;
     miScalar     tblur;
     miScalar    swidth;
     miScalar    twidth;
     miInteger  channel;
     miColor       fill;
};


struct tiffCache
{
     tiffCache() {}
     ~tiffCache()
     {
	delete txt;
	delete opts;
     }

     CTexture*         txt;
     TextureOptions*  opts;
};





EXTERN_C DLLEXPORT int gg_tiff_version(void) {return(1);}


EXTERN_C DLLEXPORT void
gg_tiff_init(
	      miState* const        state,
	      struct gg_tiff_t* p,
	      miBoolean* req_inst
	      )
{
  if ( !p ) {  // global shader init, request per instance init
     
     textureInit( kMEMORY_LIMIT * 1024 );
     *req_inst = miTRUE; return;
  }
  
   miTag fileNameTag = *mi_eval_tag( &p->filename );
   
   const char* name = (char*) mi_db_access( fileNameTag );
   
   CTexture* txt = textureLoad( name );
   mi_db_unpin( fileNameTag );
   if ( !txt )  return;
   
   tiffCache* cache = new tiffCache;

   cache->txt = txt;
   
   miScalar swidth = mr_eval( p->swidth );
   miScalar twidth = mr_eval( p->twidth );
   miUint samples = mr_eval( p->samples );
   miScalar sblur = mr_eval( p->sblur ) / 100.0f;
   miScalar tblur = mr_eval( p->tblur ) / 100.0f;
   miUint  channel = mr_eval( p->channel );
   miScalar fill   = mr_eval( p->fill );

   mr::filter::types flt = (mr::filter::types) mr_eval( p->filter );
   mr::filter::function filter = mr::filter::fromEnumeration( flt );
   
   cache->opts = new TextureOptions( filter, swidth,
				     twidth, sblur, tblur, samples,
				     channel, fill );
   
   void **user;
   mi_query(miQ_FUNC_USERPTR, state, 0, &user);
   *user = cache;
}

EXTERN_C DLLEXPORT void
gg_tiff_exit(
	      miState* const        state,
	      struct gg_tiff_t* p
	      )
{
  if ( !p )
  {
     Stats->Print();
     Stats->Init();
     textureShutdown();
     return;
  }
  
  void **user;
  mi_query(miQ_FUNC_USERPTR, state, 0, &user);
  
  tiffCache* cache = static_cast<tiffCache*>( *user );
  if (cache) delete cache;
}


EXTERN_C DLLEXPORT miBoolean 
gg_tiff(
	 miScalar* const result,
	 miState* const state,
	 struct gg_tiff_t* p
	 )
{
   void **user;
   mi_query(miQ_FUNC_USERPTR, state, 0, &user);
   tiffCache* cache = static_cast<tiffCache*>( *user );
   if (!cache)
   {
      *result = 0.0f;
      return miTRUE;
   }

   // This is done to match maya
   state->tex.x = 1.0f - state->tex_list[0].y;
   state->tex.y = state->tex_list[0].x;
      
   miScalar val[3];
   if ( cache->opts->numSamples == 1 )
      cache->txt->lookup( state, val, state->tex.x, state->tex.y,
			  *cache->opts );
   else
   {
      //! todo... DsuDtv still not right 
#ifdef ANISOTROPIC
      miScalar DuS, DvS, DuT, DvT;
      DsuDtv( state, DuS, DvS, DuT, DvT );

      miScalar ds = math<float>::fabs(DuS + DvS);
      miScalar dt = math<float>::fabs(DuT + DvT);
#else
      miScalar    ds = area(state);
      miScalar    dt = ds;
#endif
      
      miScalar sw = cache->opts->swidth;
      miScalar tw = cache->opts->twidth;
      miScalar samt = (ds * sw + cache->opts->sblur) * 0.5f;
      miScalar tamt = (dt * tw + cache->opts->tblur) * 0.5f;
      
      miScalar S[4],T[4];
      S[0] = S[3] = state->tex.x - samt;
      S[1] = S[2] = state->tex.x + samt;
      T[0] = T[1] = state->tex.y - tamt;
      T[2] = T[3] = state->tex.y + tamt;
      cache->txt->lookup4( state, val, S, T, *cache->opts );
   }
   
   *result = val[0];
   return(miTRUE);
}





EXTERN_C DLLEXPORT int gg_ctiff_version(void) {return(1);}


EXTERN_C DLLEXPORT void
gg_ctiff_init(
	      miState* const        state,
	      struct gg_ctiff_t* p,
	      miBoolean* req_inst
	      )
{
  if ( !p ) {  // global shader init, request per instance init
     textureInit( 8192 * 1024 );

    *req_inst = miTRUE; return;
  }
  
   miTag fileNameTag = *mi_eval_tag( &p->filename );
   const char* name = (char*) mi_db_access( fileNameTag );
   
   CTexture* txt = textureLoad( name );
   mi_db_unpin( fileNameTag );
   if ( !txt )  return;

   tiffCache* cache = new tiffCache;

   cache->txt = txt;
   
   miScalar swidth = mr_eval( p->swidth );
   miScalar twidth = mr_eval( p->twidth );
   miUint samples = mr_eval( p->samples );
   miScalar sblur = mr_eval( p->sblur ) / 100.0f;
   miScalar tblur = mr_eval( p->tblur ) / 100.0f;
   miUint  channel = mr_eval( p->channel );
   miColor  fill   = mr_eval( p->fill );

   mr::filter::types flt = (mr::filter::types) mr_eval( p->filter );
   mr::filter::function filter = mr::filter::fromEnumeration( flt );
   
   cache->opts = new TextureOptions( filter, swidth,
				     twidth, sblur, tblur, samples,
				     channel, fill );
   
   void **user;
   mi_query(miQ_FUNC_USERPTR, state, 0, &user);
   *user = cache;
}

EXTERN_C DLLEXPORT void
gg_ctiff_exit(
	      miState* const        state,
	      struct gg_ctiff_t* p
	      )
{
  if ( !p )
  {
     Stats->Print();
     Stats->Init();
     textureShutdown();
     return;
  }
  
  void **user;
  mi_query(miQ_FUNC_USERPTR, state, 0, &user);
  
  tiffCache* cache = static_cast<tiffCache*>( *user );
  if (cache) delete cache;
}


EXTERN_C DLLEXPORT miBoolean 
gg_ctiff(
	 color* const result,
	 miState* const state,
	 struct gg_ctiff_t* p
	 )
{
   void **user;
   mi_query(miQ_FUNC_USERPTR, state, 0, &user);
   tiffCache* cache = static_cast<tiffCache*>( *user );
   if (!cache)
   {
      result->r = result->g = result->b = result->a = 0.0f;
      return miTRUE;
   }

   // This is used to match maya
   state->tex.x = 1.0f - state->tex_list[0].x;
   state->tex.y = state->tex_list[0].y;

   
   miScalar val[4];
   
   if ( cache->opts->numSamples == 1 )
      cache->txt->lookup( state, val, state->tex.x, state->tex.y,
			  *cache->opts );
   else
   {
      //! todo... DsuDtv still not right 


#ifdef ANISOTROPIC
      miScalar DuS, DvS, DuT, DvT;
      DsuDtv( state, DuS, DvS, DuT, DvT );

      miScalar ds = math<float>::fabs(DuS + DvS);
      miScalar dt = math<float>::fabs(DuT + DvT);
#else
      miScalar    ds = area(state);
      miScalar    dt = ds;
#endif
      
      miScalar sw = cache->opts->swidth;
      miScalar tw = cache->opts->twidth;
      miScalar samt = (ds * sw + cache->opts->sblur) * 0.5f;
      miScalar tamt = (dt * tw + cache->opts->tblur) * 0.5f;
      
      
      miScalar S[4],T[4];
      S[0] = S[3] = state->tex.x - samt;
      S[1] = S[2] = state->tex.x + samt;
      T[0] = T[1] = state->tex.y - tamt;
      T[2] = T[3] = state->tex.y + tamt;
      cache->txt->lookup4( state, val, S, T, *cache->opts );
   }
   
   result->r = val[0];
   result->g = val[1];
   result->b = val[2];
   result->a = 1.0f;
   
   return(miTRUE);
}




#endif

