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

#ifndef MR_MEM_CHECK
#error When compiling mrLibrary, you need to have MR_MEM_CHECK set
#endif

#ifndef MR_LIBRARY_EXPORT
#error When compiling mrLibrary, you need to have MR_LIBRARY_EXPORT set
#endif

#include "mrStream.h"
#include "mrMemory.h"
#include "mrAssert.h"

#include "mrTiff.h"

BEGIN_NAMESPACE( mr )

#ifndef MR_NO_TIFF
MR_LIB_EXPORT TextureStats* Stats;
#endif

MR_LIB_EXPORT mutex*        streamMutex;
#define NEW_STREAM(X)  MR_LIB_EXPORT X ## stream* X
NEW_STREAM(info);
NEW_STREAM(warning);
NEW_STREAM(error);
NEW_STREAM(fatal);
NEW_STREAM(progress);

END_NAMESPACE( mr )



#define INIT_STREAM(X)  ::mr:: ## X = new X ## stream; \

//!
//! This routine gets called whenever mental ray loads the DSO.
//! It allows initializing elements in a safer way than using
//! the constructors of static classes.
//!
//! This feature does not work on IBM servers, thou.
//!
EXTERN_C DLLEXPORT void module_init()
{
   mi_debug("mrLibrary: module_init start");

   using namespace mr;
   
   // Initialize memory tracking
   Start_MemoryDebug();
   
   gExceptionHandler = new ExceptionHandler;

#ifndef MR_NO_TIFF
   Stats = new TextureStats;
#endif
   
   // Initialize a mutex to lock printing
   streamMutex = new mutex;

   // Initialize all the stream classes
   INIT_STREAM( info );
   INIT_STREAM( warning );
   INIT_STREAM( error );
   INIT_STREAM( fatal );
   INIT_STREAM(	progress );

   mi_debug("mrLibrary: module_init end");
}


EXTERN_C DLLEXPORT void module_exit()
{
   mi_debug("mrLibrary: module_exit start");

   using namespace mr;
   
   delete info;
   delete warning;
   delete error;
   delete fatal;
   delete progress;
   
   delete streamMutex;
   
#ifndef MR_NO_TIFF
   delete Stats;
#endif
   
   delete gExceptionHandler;

   End_MemoryDebug();
   mi_debug("mrLibrary: module_exit end");
}
