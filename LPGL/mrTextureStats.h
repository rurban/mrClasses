////////////////////////////////////////////////////////////////////////
//
// TIFF reading code is basically a port of Pixie's TIFF reading code,
// which is released under LPGL and NOT OpenBSD.
//
// As such, it is included and distributed separately from mrClasses,
// and the files mrTiff.h, mrTiff.cpp, mrTextureStats.h for the time
// being are distributed under LPGL, not OpenBSD.
// If you disagree with the LPGL terms, you can just use mrClasses
// without this tiff class and remain within the OpenBSD agreement.
//
////////////////////////////////////////////////////////////////////////
//
//                             Pixie
//
// Copyright (C) 1999 - 2003, Okan Arikan
//
// Contact: okan@cs.berkeley.edu
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef mrTextureStats_h
#define mrTextureStats_h

#ifndef MR_NO_TIFF

#ifndef SHADER_H
#include "shader.h"
#endif

#ifndef mrMacros_h
#include "mrMacros.h"
#endif

#ifndef mrPlatform_h
#include "mrPlatform.h"
#endif

BEGIN_NAMESPACE( mr )


//! Stores statistics about texture access
struct TextureStats
{
     //! Number of open File Descriptors
     miUint numFileDescriptors; 
     //! The number of texture misses
     miUlong numTextureMisses;
     //! The number of texture references
     miUlong numTextureRef;
     //! The total amount the texture data transmitted 
     miUlong transferredTextureData;
     //! The current number of textures
     miUlong numTextures;
     //! The current amount of textures in memory
     miUlong textureSize;
     //! The peak number of textures
     miUlong numPeakTextures;
     //! The amount of memory at the peak devoted to textures
     miUlong peakTextureSize;
     //! The number of times textures were flushed from memory
     miUlong textureFlushes;

     void Print()
     {
	mi_info("---------------------------------------------");
	mi_info("TIFF Statistics:");
	mi_info("Total Texture Data: %d", transferredTextureData);
	mi_info("Texture Flushes: %d", textureFlushes);
	double avg = ((double)transferredTextureData/(textureFlushes+1))/1024.0;
	mi_info("Ideal Memory Limit: %g", avg);
	mi_info("Peak Textures #: %d", numPeakTextures);
	mi_info("Peak Texture Memory: %d", peakTextureSize);
	mi_info("Texture Accesses: %d", numTextureRef);
	miUlong misses = numTextureMisses - numPeakTextures;
	mi_info("Texture   Misses: %d", misses);
	mi_info("---------------------------------------------");
     }

     void Init()
     {
	numTextures = 0;
	numTextureMisses = 0;
	numTextureRef = 0;
	transferredTextureData = 0;
	textureSize = 0;
	numPeakTextures = 0;
	peakTextureSize = 0;
	textureFlushes = 0;
     }

     TextureStats()
     {
	Init();
     };
     
     ~TextureStats()
     {};
};

extern MR_LIB_EXPORT TextureStats* Stats;

END_NAMESPACE( mr )


#endif // MR_NO_TIFF

#endif // mrTextureStats_h

