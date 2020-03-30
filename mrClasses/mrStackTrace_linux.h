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

#ifndef mrStackTrace_linux_h
#define mrStackTrace_linux_h

#include <stdio.h>
#include <signal.h>
#include <execinfo.h>

/* get REG_EIP from ucontext.h */
#define __USE_GNU
#include <ucontext.h>


BEGIN_NAMESPACE( mr )

class ExceptionHandler
{
     ExceptionHandler();
     ~ExceptionHandler();
     
     static void ShowStack();
   private:
     static void demangle( const char* name );
     static void bt_sighandler(int sig, siginfo_t *info,
			       void *secret);
     int install_signal_handler();
     int restore_signal_handler();

     sigaction oldSIGSEGV;
     sigaction oldSIGUSR1;
     sigaction oldSIGBUS;
     sigaction oldSIGILL;
     sigaction oldSIGFPE;
};

END_NAMESPACE( mr )

// global instance of class
extern MR_LIB_EXPORT mr::ExceptionHandler* gExceptionHandler;

#endif // mrStackTrace_linux_h
