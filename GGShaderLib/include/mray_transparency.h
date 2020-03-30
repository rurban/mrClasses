
#ifndef mrColor_h
#include "mrColor.h"
#endif


BEGIN_NAMESPACE( mr )

  inline void
  trace_transparent( miColor* const result, 
		     const miState* const state )
  {
    if ( result->a < 1.0f )
      {
	 // if (rapid)
//  	mi_opacity_set( state, result.a );
//        }
//      else
//        {
	miColor behind;

	mi_trace_transparent( &behind, const_cast< miState* >( state ) );

	miScalar Ti = 1.0f - result->a;
	behind   *= Ti;
	behind.a *= Ti;

	*result   += behind;
	result->a += behind.a;
      }
  }

END_NAMESPACE( mr )
