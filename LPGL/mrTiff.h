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
// Due to permission of the author, the files mrTiff.h and mrTiff.cpp
// are also able to be under an OpenBSD license agreement.
//


#ifndef mrTiff_h
#define mrTiff_h

#ifndef MR_NO_TIFF

#ifndef mrTextureStats_h
#include "mrTextureStats.h"
#endif

#ifndef mrFilters_h
#include "mrFilters.h"
#endif

#ifndef mrColor_h
#include "mrColor.h"
#endif


BEGIN_NAMESPACE( mr )


// Some foward defererencing
class	CTexture;
class	CEnvironment;
class	CMadeTexture;


//! This class holds information about a particular texture lookup
struct	CTextureLookup	{
   public:
     //! Lookup filter
     mr::filter::function	filter;
     //! The filter width
     float		swidth,twidth;
     //! Blur amount
     float		blur;
     //! Texture s,t Blur amount
     float		sblur,tblur;
     //! The number of samples to take in the texture
     int		numSamples;
     //! The shadow bias for the lookup
     float		shadowBias;
     //! The start channel for the lookup
     int		channel;
     //! The fill in value for the lookup
     color		fill;
};


//! User options for texture lookups
struct TextureOptions : public CTextureLookup
{
     TextureOptions( const mr::filter::function inFilter = mr::filter::box,
		     const float inSwidth = 0.0f, const float inTwidth = 0.0f,
		     const float inSblur = 0.0f, const float inTblur = 0.0f,
		     const int inSamples = 1, const int inChannel = 0,
		     const color inFill = 0.0f
		    ) 
     {
	filter = inFilter;
	swidth = inSwidth;
	swidth = inSwidth; twidth = inTwidth;
	sblur = inSblur;    tblur = inTblur;
	numSamples = inSamples;
	channel = inChannel;
	fill = inFill;
	shadowBias = 0.0f;
	blur = 0.0f;
     }
};

//! User options for shadow lookups
struct ShadowOptions : public CTextureLookup
{
     ShadowOptions( const mr::filter::function inFilter = mr::filter::box,
		    const float inSwidth = 0.0f, const float inTwidth = 0.0f,
		    const float inBlur = 0.0f, const int inSamples = 4, 
		    const float inShadowBias = 0.03f
		   )
     {
	filter = inFilter;
	swidth = inSwidth; twidth = inTwidth;
	sblur = tblur = 0.0f;
	numSamples = inSamples;
	blur = inBlur;
	shadowBias = inShadowBias;
	channel = 0;
	fill = 0.0f;
     }
};

//! This class holds information about a particular texture block
class	CTextureBlock
{
   public:
     //! Where the block data is stored (NULL if the block has been paged out)
     void		*data;
     //! Size of the block in bytes
     int		size;
     //! Last time this block was referenced
     int		lastRefNumber;
     //! Pointer to the next used / empty block
     CTextureBlock	*next;
};

//! Texture wrapping mode
typedef enum {
kTEXTURE_PERIODIC,
kTEXTURE_BLACK,
kTEXTURE_CLAMP
} TTextureMode;


//! This class encapsulates a single 2D texture layer in a file
class	CTextureLayer  {
   public:
     CTextureLayer(const char *,int,int,int,int,int,int);
     virtual				~CTextureLayer();
     //! Color lookup
     void	lookup(const miState* const, float *,float,float,
		       const CTextureLookup& );
     //! Depth lookup
     void	lookupz(const miState* const, float *,float,float,
			const CTextureLookup& );

     char*		name;	//<- The filename of the texture
     int	   directory;	//<- The directory index in the tiff file
     int	width,height,numSamples;  //<- The image info
     int	fileWidth,fileHeight;	  //<- The physical size in the file
   protected:
     //! Lookup 4 pixel values
     //! This function must be overriden by the child class
     virtual void lookupPixel(float *,int,int,const CTextureLookup& ) = 0;
};


//! This class the the base of all texture types
class	CTexture {
   public:

     CTexture(const char *,int,int,TTextureMode,TTextureMode);
     virtual ~CTexture();
     
     //! Point access
     virtual void lookup(const miState* const, float*, float, float,
			 const CTextureLookup& );
     
     //! Area access
     virtual void lookup4(const miState* const, float*, const float*,
			  const float*, const CTextureLookup& );
     //! The dimensions of the texture (used to figure out the blur amount)
     int  width,height;
     //! The texture wrapping mode
     TTextureMode		sMode,tMode;
     //! Name of the file
     char* name;
};

//! Pyramid Texture
class	CMadeTexture : public CTexture {
public:
     CMadeTexture(const char *,int,int,TTextureMode,TTextureMode);
     virtual	~CMadeTexture();
     
     void	lookup(const miState* const, float *,float,float,
		       const CTextureLookup& );
     void	lookup4(const miState* const, float *,const float *,
			const float *, const CTextureLookup& );
     
     //! The number of layers (pyramids)
     int	     numLayers;
     CTextureLayer** layers;
};


//! A regular texture
class	CRegularTexture : public CTexture {
   public:
     CRegularTexture(const char *,int,int,TTextureMode,TTextureMode);
     virtual ~CRegularTexture();

     void lookup(const miState* const, float *,float,float,
		 const CTextureLookup& );
     void lookup4(const miState* const, float *,const float *,const float *,
		  const CTextureLookup& lookup);
     
     // There's only one layer
     CTextureLayer*      layer;
};


//! An environment map (also encapsulates shadow maps)
class	CEnvironment {
   public:

     CEnvironment(const char *);
     virtual ~CEnvironment();

     virtual void lookup(const miState* const,
			 float *,const float *,const float *,
			 const float *,const CTextureLookup& );
     char*		name;	//<- The filename of the texture
};

//! A single sided shadow map
class	CShadow : public CEnvironment{
   public:

     CShadow(const char *,float *,CTextureLayer *s);
     virtual ~CShadow();

     void lookup(const miState* const, float *,const float *,const float *,
		 const float *,const CTextureLookup& );

     CTextureLayer*              side;
     miMatrix			toNDC;
};



//! The deep shadow map header
class	CDeepShadowHeader {
   public:
     //! The resolution of the file
     int       xres,yres;
     //! The number of tiles
     int   xTiles,yTiles;
     //! The tile dimensions
     int	tileSize;
     //! The tile shift
     int       tileShift;
     miMatrix	   toNDC;
};

//! A deep shadow map
class	CDeepShadow : public CEnvironment{
     class	CDeepTile {
	public:
	  float			**data;
	  float			**lastData;
	  CTextureBlock	*block;
     };

   public:

     CDeepShadow(const char *,const char *,const float *,FILE *);
     virtual ~CDeepShadow();

     void lookup(const miState* const, float *,const float *,const float *,
		 const float *,const CTextureLookup& );

   private:
     void  loadTile(int,int);

     char	*fileName;
     CDeepTile	**tiles;
     int	*tileIndices;		// The tile index

     CDeepShadowHeader	header;		// The header
     int	fileStart;		// The offset in the file
};



//! A Cubic environment map
class	CCubicEnvironment : public CEnvironment{
   public:

     CCubicEnvironment(const char *,CTexture **s);
     virtual ~CCubicEnvironment();

     void lookup(const miState* const, float *,const float *,const float *,
		 const float *,const CTextureLookup& );

     CTexture*   sides[6];
};


//! A spherical environment map
class	CSphericalEnvironment : public CEnvironment{
   public:

     CSphericalEnvironment(const char *,CTexture *s);
     virtual ~CSphericalEnvironment();

     void lookup(const miState* const, float *,const float *,const float *,
		 const float *,const CTextureLookup& );
     
     CTexture* side;
};

struct TSearchpath;  // we don't use this for now

MR_LIB_EXPORT void textureInit(int maxMemory);
MR_LIB_EXPORT void textureShutdown();

MR_LIB_EXPORT
CTexture* textureLoad(const char *name,TSearchpath *path = NULL);

END_NAMESPACE( mr )

#endif // MR_NO_TIFF

#endif // mrTiff_h
