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
//

#ifndef MR_NO_TIFF

//
// To use this file, you need to link against libtiff.
// libtiff is a freely available TIFF library, which can be gotten from
// the web.
//

#include "tiffio.h"

#ifndef mrTiff_h
#include "mrTiff.h"
#endif

#ifndef mrAux_h
#include "mrAux.h"
#endif

#ifndef mrVector_h
#include "mrVector.h"
#endif

#ifndef mrMath_h
#include "mrMath.h"
#endif

extern "C"
{

static	void
tiffErrorHandler(const char *module,const char *fmt, va_list ap)
{
   mi_error(fmt,ap);
}


static	void
tiffWarningHandler(const char *module,const char *fmt, va_list ap)
{
   mi_warning(fmt,ap);
}

}


BEGIN_NAMESPACE( mr )

#define INFINITY                  (float) 1e30

// Stuff for fast caching
static	miUint	refNumber = 0;	//<- The last reference number
static	CTextureBlock	*usedBlocks = NULL;	//<- All blocks currently in use
static	CTextureBlock	*freeBlocks = NULL;	//<- Free blocks
static	miUint	numUsedBlocks =	0;	//<- Number of used blocks
static	miUint	usedTextureMemory = 0;	//<- The amount of texture memory in use
static	miUint	maxTextureMemory = 0;	//<- The maximum texture memory

const	float	inv255 = 1.0f / 255.0f;

#define initvf( v, f   ) v[0] = v[1] = v[2] = f;
#define initv( v, p   ) v[0] = p[0]; v[1] = p[1]; v[2] = p[2];
#define mulvf( v, f   ) v[0] *= f; v[1] *= f; v[2] *= f;
#define addvv( r, v1, v2 ) \
       r[0] = v1[0] + v2[0]; \
       r[1] = v1[1] + v2[1]; \
       r[2] = v1[2] + v2[2];

static const int OS_MAX_PATH_LENGTH = 1024;

miLock tiffLock;

const char* const RI_PERIODIC	 = "periodic";
const char* const RI_CLAMP	 = "clamp";
const char* const RI_BLACK	 = "black";



///////////////////////////////////////////////////////////////////////
//
// Misc texture block management functions
//
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// Function  :	textureQuickSort
// Description 	:	Sort the textures in the order of increasing last ref number
static	void	textureQuickSort(CTextureBlock **activeBlocks,
				 int start,int end)
{
   int  pivot = (rand() % (end - start)) + start;
   int  i,last;
   CTextureBlock* cBlock;

   cBlock  = activeBlocks[start];
   activeBlocks[start] = activeBlocks[pivot];
   activeBlocks[pivot] = cBlock;

   for (last=start,i=start+1;i<=end;i++) {
      if (activeBlocks[i]->lastRefNumber < activeBlocks[last]->lastRefNumber)
      {
	 cBlock   = activeBlocks[i];
	 activeBlocks[i]  = activeBlocks[last];
	 activeBlocks[last++] = cBlock;
      }
   }

   cBlock  = activeBlocks[last];
   activeBlocks[last] = activeBlocks[start];
   activeBlocks[start] = cBlock;

   if ((last-1) > start)
      textureQuickSort(activeBlocks,start,last-1);

   if (end > (last+1))
      textureQuickSort(activeBlocks,last+1,end);
}

///////////////////////////////////////////////////////////////////////
// Function  :	textureMemFlush
// Description 	:	Try to deallocate some textures from memory
static int textureMemFlush(CTextureBlock *entry) {
   int  i,j;
   CTextureBlock	*cBlock;
   CTextureBlock	**activeBlocks;
   int  flushed = miFALSE;
   
   Stats->textureFlushes++;
   
   if (usedBlocks == NULL)	return miFALSE;

   for (cBlock=usedBlocks,i=0;cBlock!=NULL;cBlock=cBlock->next)
   {
      if (cBlock->data != NULL) {
	 i++;
      }
   }

//     activeBlocks = (CTextureBlock **) ralloc(i*sizeof(CTextureBlock *));
   activeBlocks = (CTextureBlock **) mi_mem_allocate(i*sizeof(CTextureBlock *));

   for (cBlock=usedBlocks,i=0;cBlock!=NULL;cBlock=cBlock->next)
   {
      if (cBlock->data != NULL)
      {
	 if (cBlock != entry)
	 {
	    activeBlocks[i++] = cBlock;
	 }
      }
   }

   if (i > 1)
      textureQuickSort(activeBlocks,0,i-1);

   for (j=0;(j<i) && (usedTextureMemory > (maxTextureMemory/2));j++)
   {
      cBlock = activeBlocks[j];

      Stats->textureSize   -= cBlock->size;
      usedTextureMemory	   -= cBlock->size;
      delete [] (unsigned char *) cBlock->data;
      cBlock->data = NULL;

      flushed = miTRUE;
   }

   mi_mem_release( activeBlocks );

   return flushed;
}

///////////////////////////////////////////////////////////////////////
// Function  :	textureLoadBlock
// Description 	:	Read a block of texture from disk
// Return Value 	:	Pointer to the new texture
static void	textureAllocateBlock(CTextureBlock *entry)
{
   Stats->textureSize  += entry->size;
   
   Stats->transferredTextureData += entry->size;
   usedTextureMemory  += entry->size;

   entry->data   = new unsigned char[entry->size];

   // If we exceeded the maximum texture memory, phase out the last texture
   if (usedTextureMemory > maxTextureMemory)
      textureMemFlush(entry);
   
   if (Stats->textureSize > Stats->peakTextureSize)
      Stats->peakTextureSize = Stats->textureSize;
}

///////////////////////////////////////////////////////////////////////
// Function  :	textureLoadBlock
// Description 	:	Read a block of texture from disk
// Return Value 	:	Pointer to the new texture
static void	textureLoadBlock(CTextureBlock *entry,char *name,
				 int x,int y,int w,int h,int dir)
{
   void 	*data = NULL;
   TIFF 	*in;

   // Update the state
   Stats->numTextureMisses++;

   // Set the error handler so we don't crash
   TIFFSetErrorHandler(tiffErrorHandler);
   TIFFSetWarningHandler(tiffWarningHandler);
     
   in = TIFFOpen(name,"r");

   if (in != NULL)
   { // Error, we opened this file before
      // The stupid user must have deleted the 
      // file or unmounted the drive while in progress
      uint32	width,height;
      uint16	numSamples;
      int pixelSize;
      int i;
      uint16	bitspersample;
      int tiled = TIFFIsTiled(in);

      TIFFSetDirectory(in,dir);

      // Get the texture properties
      TIFFGetFieldDefaulted(in,TIFFTAG_IMAGEWIDTH,      &width);
      TIFFGetFieldDefaulted(in,TIFFTAG_IMAGELENGTH,     &height);
      TIFFGetFieldDefaulted(in,TIFFTAG_SAMPLESPERPIXEL, &numSamples);
      TIFFGetFieldDefaulted(in,TIFFTAG_BITSPERSAMPLE,   &bitspersample);

      if (bitspersample == 8) {
	 pixelSize = numSamples*sizeof(unsigned char);
      } else {
	 pixelSize = numSamples*sizeof(float);
      }

      // Allocate space for the texture
      textureAllocateBlock(entry); 
      data = (unsigned char *) entry->data;

      // Do we need to read the entire texture ?
      if ((x != 0) || (y != 0) || (w != (int) width) || (h != (int) height)) {
	 // No , is the file tiled ?
	 if (!tiled) {
	    // No, read the required portion
	    unsigned char	*tdata;

//  	    tdata = (unsigned char *) ralloc(width*height*pixelSize);
	    tdata = (unsigned char *) mi_mem_allocate(width*height*pixelSize);

	    // Read the entire image
	    mrASSERT((int) (pixelSize*width) == TIFFScanlineSize(in));
	    for (i=0;i<(int) height;i++) {
	       TIFFReadScanline(in,&tdata[pixelSize*i*width],i,0);
	       if (i >= (y+h))	break;	// If we read the last required scanline, break
	    }

	    for (i=0;i<h;i++) {
	       memcpy(&((unsigned char *) data)[i*pixelSize*w],
		      &tdata[((y+i)*width + x)*pixelSize],
		      w*pixelSize);
	    }

	    mi_mem_release(tdata);
	    
	 } else {
	    uint32	tileWidth,tileHeight;

	    TIFFGetFieldDefaulted(in,TIFFTAG_TILEWIDTH , &tileWidth);
	    TIFFGetFieldDefaulted(in,TIFFTAG_TILELENGTH, &tileHeight);
	    mrASSERT(tileWidth == (uint32) w);
	    mrASSERT(tileHeight == (uint32) h);
	    mrASSERT((x % tileWidth) == 0);
	    mrASSERT((y % tileHeight) == 0);

	    int	k = TIFFReadTile(in,data,x,y,0,0);
	 }
      } else {
	 if (tiled) {
	    uint32	tileWidth,tileHeight;

	    TIFFGetFieldDefaulted(in,TIFFTAG_TILEWIDTH , &tileWidth);
	    TIFFGetFieldDefaulted(in,TIFFTAG_TILELENGTH, &tileHeight);

	    if ((x != 0) || (y != 0) || (w != (int) tileWidth) ||
		(h != (int) tileHeight))
	    {
	       mi_error("Tiled unmade texture.");
//  	       error(CODE_BUG,"Tiled unmade texture.");
	    } else {
	       TIFFReadTile(in,data,x,y,0,0);
	    }
	 } else {
	    int	i;

	    // Read the entire image
	    mrASSERT((int) (pixelSize*width) == TIFFScanlineSize(in));
	    for (i=0;i<(int) height;i++) {
	       TIFFReadScanline(in,&((unsigned char *) data)[pixelSize*i*width],i,0);
	    }
	 }
      }


      TIFFClose(in);
   } else {
      int	i;

      data = new unsigned char[entry->size];

      for (i=0;i<entry->size;i++) {
	 ((unsigned char *) data)[i] = 0;
      }
   }

   entry->data = data;
}

//////////////////////////////////////////////////////////////////////
// Function  :	textureNewBlock
// Description 	:	Create a new texture block
// Return Value 	:	Pointer to the new block
static CTextureBlock	*textureNewBlock(int size) {
   CTextureBlock	*cEntry = NULL;
	
   if (freeBlocks != NULL)
   {
      cEntry = freeBlocks;
      freeBlocks = cEntry->next;
   }

   if (cEntry == NULL)
   {
      cEntry = new CTextureBlock;
   }

   cEntry->next = usedBlocks;
   usedBlocks   = cEntry;

   cEntry->data = NULL;
   cEntry->lastRefNumber = refNumber;
   cEntry->size = size;

   return cEntry;
}

//////////////////////////////////////////////////////////////////////
// Function  :	textureDeleteBlock
// Description 	:	Delete a texture block
// Return Value 	:	Pointer to the new block
// Comments  :
// Date last edited :	7/7/2001
static void textureDeleteBlock(CTextureBlock *cEntry)
{
   CTextureBlock	*pBlock,*cBlock;

   for (pBlock=NULL,cBlock=usedBlocks; cBlock != NULL;
	pBlock = cBlock,cBlock = cBlock->next)
   {
      if (cBlock == cEntry)
      {
	 if (pBlock == NULL)
	    usedBlocks = cBlock->next;
	 else
	    pBlock->next = cBlock->next;

	 if (cBlock->data != NULL)
	 {
	    Stats->textureSize	-= cBlock->size;
	    usedTextureMemory	-= cBlock->size;
	    delete [] (unsigned char *) cBlock->data;
	 }
 	
	 cBlock->next = freeBlocks;
	 freeBlocks  = cBlock;

	 return;
      }
   }

   mrASSERT(miFALSE);
}
















//! Thic class holds a basic uncached texture
template <class T> class CBasicTexture : public CTextureLayer
{
   public:
     // Description 	:	Ctor
     CBasicTexture(const char *name,int directory,int width,int height,
		   int numSamples,int fileWidth,int fileHeight) :
     CTextureLayer(name,directory,width,height,numSamples,
		   fileWidth,fileHeight)
     {
	this->dataBlock = textureNewBlock(width*height*numSamples*sizeof(T));
     }

     // Description 	:	Dtor
     ~CBasicTexture() {
	textureDeleteBlock(dataBlock);
     }

   protected:
     // The pixel lookup
     void	lookupPixel(float *,int,int,const CTextureLookup& );
     
     CTextureBlock *dataBlock;
};

//! This class holds info about a tiled texture
//! (tiled textures are cached in tiles)
template <class T> class CTiledTexture : public CTextureLayer {
   public:
     // Description 	:	Ctor
     CTiledTexture(const char *name,int directory,int width,int height,
		   int numSamples,int fileWidth,int fileHeight,
		   int tileSize,int tileSizeShift) :
     CTextureLayer(name,directory,width,height,numSamples,
		   fileWidth,fileHeight)
     {
	int	i,j;
	int	tileLength;

	this->tileSize = tileSize;
	this->tileSizeShift = tileSizeShift;
	tileLength  = tileSize*tileSize*numSamples*sizeof(T);

	xTiles = (int) ceil((float) width / (float) tileSize);
	yTiles = (int) ceil((float) height / (float) tileSize);

	dataBlocks  = new CTextureBlock**[yTiles];
	for (i=0;i<yTiles;i++) {
	   dataBlocks[i] = new CTextureBlock*[xTiles];

	   for (j=0;j<xTiles;j++) {
	      dataBlocks[i][j] = textureNewBlock(tileLength);
	   }
	}
     }

     // Description 	:	Dtor
     ~CTiledTexture() {
	int i,j;

	for (i=0;i<yTiles;i++) {
	   for (j=0;j<xTiles;j++) {
	      textureDeleteBlock(dataBlocks[i][j]);
	   }

	   delete [] dataBlocks[i];
	}

	delete [] dataBlocks;
     }

   protected:
     //! Pixel lookup
     void 	lookupPixel(float *,int,int,const CTextureLookup& );

     CTextureBlock	***dataBlocks;
     int  xTiles,yTiles;
     int  tileSize,tileSizeShift;
};




///////////////////////////////////////////////////////////////////////
// Class  :	CTextureLayer
// Method  :	CTextureLayer
// Description 	:	Ctor
CTextureLayer::CTextureLayer(const char *n,int dir,int w,int h,
			     int ns,int fw,int fh) :
directory( dir ),
width( w ),
height( h ),
numSamples( ns ),
fileWidth( fw ),
fileHeight( fh ),
name( mi_mem_strdup(n) )
{
}

///////////////////////////////////////////////////////////////////////
// Class  :	CTextureLayer
// Method  :	CTextureLayer
// Description 	:	Dtor
CTextureLayer::~CTextureLayer()
{
   mi_mem_release(name);
}



///////////////////////////////////////////////////////////////////////
// Class  :	CTextureLayer
// Method  :	lookup
// Description 	:	Lookup a pixel in the texture (bilinear lookup)
// Return Value 	:	Color in r
// Comments  :	0 <= (x,y) <= 1
void CTextureLayer::lookup( const miState* const state,
			    float* r,float x,float y, const CTextureLookup& l)
{
   int xi;
   int yi;
   float	dx;
   float	dy;
   float	res[4*3];
   float	tmp;

   x *= width;   // To the pixel space
   y *= height;
   xi = fastmath<float>::floor(x);  // The integer pixel coordinates
   yi = fastmath<float>::floor(y);
   dx = x - xi;
   dy = y - yi;

   if (xi >= width)  xi -= width;
   if (yi >= height) yi -= height;

   lookupPixel(res,xi,yi,l);

   tmp = (1-dx)*(1-dy);  // Bilinear interpolation
   r[0] = res[0]*tmp;
   r[1] = res[1]*tmp;
   r[2] = res[2]*tmp;

   tmp = dx*(1-dy);
   r[0] += res[3]*tmp;
   r[1] += res[4]*tmp;
   r[2] += res[5]*tmp;

   tmp = (1-dx)*dy;
   r[0] += res[6]*tmp;
   r[1] += res[7]*tmp;
   r[2] += res[8]*tmp;

   tmp = dx*dy;
   r[0] += res[9]*tmp;
   r[1] += res[10]*tmp;
   r[2] += res[11]*tmp;
}


///////////////////////////////////////////////////////////////////////
// Class  :	CTextureLayer
// Method  :	lookupz
// Description 	:	Lookup a pixel in the texture
// Return Value 	:	Depth in r
// Comments  :
// Date last edited :	2/28/2002
void CTextureLayer::lookupz(const miState* const state,
			    float *r,float x,float y, const CTextureLookup& l)
{
   int xi;
   int yi;
   float	res[4*3];

   x *= width;
   y *= height;
   x -= (float) 0.5; // Make sure we look up the pixel center
   y -= (float) 0.5;
   xi = fastmath<float>::floor(x);
   yi = fastmath<float>::floor(y);

   if ((xi < 0)	|| (yi < 0) || (xi >= (width-1)) || (yi >= (height-1))) {
      r[0] = INFINITY;
      r[1] = INFINITY;
      r[2] = INFINITY;
      return;
   }
	
   lookupPixel(res,xi,yi,l);

   r[0] = max(res[0],res[3]);
   r[0] = max(r[0],res[6]);
   r[0] = max(r[0],res[9]);
}

///////////////////////////////////////////////////////////////////////
// Class  :	CBasicTexture
// Method  :	lookup
// Description 	:	Lookup a pixel in the texture
// Return Value 	:
// Comments  :	0 <= (x,y) <= 1
// Date last edited :	2/28/2002
void	CBasicTexture<float>::lookupPixel(float *res,int x,int y,
					  const CTextureLookup& l)
{
   const float	*data;
   int 	i,j;
   int 	xi,yi;

   if (dataBlock->data == NULL)
   {
      // The data is cached out
      textureLoadBlock(dataBlock,name,0,0,width,height,directory);
   }

   // Texture cache management
   refNumber++;
   Stats->numTextureRef++;
   dataBlock->lastRefNumber = refNumber;

   i = min(numSamples - l.channel,3);
   xi = x+1;
   yi = y+1;
   if (xi >= width)	xi	-= width;
   if (yi >= height)	yi	-= height;

#define access(__x,__y) 	\
	res[0] = l.fill[0]; res[1] = l.fill[1]; res[2] = l.fill[2]; \
	data = &((float *) dataBlock->data)[(__y*fileWidth+__x)*numSamples+l.channel];	\
	for (j=0;j<i;j++) { 	\
            res[j] = data[j];	\
	}   	\
	res += 3;

   access(x,y);
   access(xi,y);
   access(x,yi);
   access(xi,yi);

#undef access
}

///////////////////////////////////////////////////////////////////////
// Class  :	CBasicTexture
// Method  :	lookup
// Description 	:	Lookup a pixel in the texture
// Return Value 	:
// Comments  :	0 <= (x,y) <= 1
// Date last edited :	2/28/2002
void	CBasicTexture<unsigned char>::lookupPixel(float *res,int x,int y,
						  const CTextureLookup& l)
{
   const unsigned char	*data;
   int  	i,j;
   int  	xi,yi;

   if (dataBlock->data == NULL)
   {
      // The data is cached out
      textureLoadBlock(dataBlock,name,0,0,width,height,directory);
   }

   // Texture cache management
   refNumber++;
   Stats->numTextureRef++;
   dataBlock->lastRefNumber = refNumber;

   i = min(numSamples-l.channel,3);
   xi = x+1;
   yi = y+1;
   if (xi >= width)	xi	-= width;
   if (yi >= height)	yi	-= height;

   mrASSERT(x < width);
   mrASSERT(y < height);
   mrASSERT(xi < width);
   mrASSERT(yi < height);

#define access(__x,__y)  \
	res[0] = l.fill[0]; res[1] = l.fill[1]; res[2] = l.fill[2]; \
	data = &((unsigned char *) dataBlock->data)[(__y*fileWidth+__x)*numSamples+l.channel];	\
	for (j=0;j<i;j++) {  \
            res[j] = data[j]*inv255;	\
	}    \
	res += 3;

   access(x,y);
   access(xi,y);
   access(x,yi);
   access(xi,yi);

#undef access
}

///////////////////////////////////////////////////////////////////////
// Class  :	CTiledTexture
// Method  :	lookup
// Description 	:	Lookup a pixel in the texture
// Return Value 	:
// Comments  :	0 <= (x,y) <= 1
// Date last edited :	2/28/2002
void	CTiledTexture<float>::lookupPixel(float *res,int x,int y,
					  const CTextureLookup& l)
{
   int  	xTile;
   int  	yTile;
   CTextureBlock *block;
   int  	i,j,t;
   const float 	*data;
   int  	xi,yi;

   refNumber++;
   Stats->numTextureRef++;

   i = min(numSamples-l.channel,3);
   t = (1 << tileSizeShift) - 1;
   xi = x+1;
   yi = y+1;
   if (xi >= width)	xi	-= width;
   if (yi >= height)	yi	-= height;

#define	access(__x,__y)     \
	xTile = __x >> tileSizeShift;   \
	yTile = __y >> tileSizeShift;   \
	block = dataBlocks[yTile][xTile];  	\
       	\
	if (block->data == NULL) {    \
 textureLoadBlock(block,name,xTile << tileSizeShift,yTile << tileSizeShift,1 << tileSizeShift,1 << tileSizeShift,directory);	\
	}       \
       	\
	block->lastRefNumber = refNumber;  	\
       	\
	res[0] = l.fill[0]; res[1] = l.fill[1]; res[2] = l.fill[2]; \
	data = &((float *) block->data)[(((__y & t) << tileSizeShift)+(__x & t))*numSamples+l.channel];	\
	for (j=0;j<i;j++) {     \
             res[j] = data[j];    \
	}       \
	res += 3;

   access(x,y);
   access(xi,y);
   access(x,yi);
   access(xi,yi);

#undef access
}

///////////////////////////////////////////////////////////////////////
// Class  :	CBasicTexture
// Method  :	lookup
// Description 	:	Lookup a pixel in the texture
// Return Value 	:
// Comments  :	0 <= (x,y) <= 1
// Date last edited :	2/28/2002
void	CTiledTexture<unsigned char>::lookupPixel(float *res,int x,int y,
						  const CTextureLookup& l)
{
   int   xTile;
   int   yTile;
   CTextureBlock 	*block;
   int   i,j,t;
   const unsigned char *data;
   int   xi,yi;

   refNumber++;
   Stats->numTextureRef++;

   i = min(numSamples-l.channel,3);
   t = (1 << tileSizeShift) - 1;
   xi = x+1;
   yi = y+1;
   if (xi >= width)	xi	-= width;
   if (yi >= height)	yi	-= height;

#define	access(__x,__y)     \
	xTile = __x >> tileSizeShift;   \
	yTile = __y >> tileSizeShift;   \
	block = dataBlocks[yTile][xTile];  	\
       	\
	if (block->data == NULL) {    \
 textureLoadBlock(block,name,xTile << tileSizeShift,yTile << tileSizeShift,1 << tileSizeShift,1 << tileSizeShift,directory);	\
	}       \
       	\
	block->lastRefNumber = refNumber;  	\
       	\
	res[0] = l.fill[0]; res[1] = l.fill[1]; res[2] = l.fill[2]; \
	data = &((unsigned char *) block->data)[(((__y & t) << tileSizeShift)+(__x & t))*numSamples+l.channel];	\
	for (j=0;j<i;j++) {     \
            res[j] = data[j]*inv255;   	\
	}       \
	res += 3;

   access(x,y);
   access(xi,y);
   access(x,yi);
   access(xi,yi);

#undef access
}


///////////////////////////////////////////////////////////////////////
// Class  :	 CTexture
// Method  :	 CTexture
// Description : Ctor
CTexture::CTexture(const char *n,int w,int h,
		   TTextureMode s,TTextureMode t) :
name( mi_mem_strdup(n) )
{
   Stats->numTextures++;
   if (Stats->numTextures > Stats->numPeakTextures)
      Stats->numPeakTextures = Stats->numTextures;

   this->width  = w;
   this->height = h;
   this->sMode  = s;
   this->tMode  = t;
}

///////////////////////////////////////////////////////////////////////
// Class  :	CTexture
// Method  :	~CTexture
// Description 	:	Dtor
CTexture::~CTexture() {
     Stats->numTextures--;   // we comment this as _exit() shader will call it
}


///////////////////////////////////////////////////////////////////////
// Class  :	CTexture
// Method  :	lookup
// Description 	:	Area lookup
void CTexture::lookup( const miState* const state,
		       float *s,float u,float v,const CTextureLookup& l)
{
   initv(s,l.fill);
}

///////////////////////////////////////////////////////////////////////
// Class  :	CTexture
// Method  :	lookup
// Description 	:	Area lookup
void CTexture::lookup4( const miState* const state,
			float *s,const float *,const float *,
			const CTextureLookup& l)
{
   initv(s,l.fill);
}


///////////////////////////////////////////////////////////////////////
// Class				:	CMadeTexture
// Method				:	CMadeTexture
// Description			:	Ctor
// Return Value			:	-
// Comments				:
// Date last edited		:	2/28/2002
CMadeTexture::CMadeTexture(const char *name,int w,int h,
			   TTextureMode s,TTextureMode t) :
CTexture(name,w,h,s,t)
{
	numLayers	=	0;
	layers		=	NULL;
}

///////////////////////////////////////////////////////////////////////
// Class				:	CMadeTexture
// Method				:	~CMadeTexture
// Description			:	Dtor
// Return Value			:	-
// Comments				:
// Date last edited		:	2/28/2002
CMadeTexture::~CMadeTexture()
{
   if (layers != NULL) {
      for (int i=0;i<numLayers;i++)
	 delete layers[i];
      delete [] layers;
   }
}


///////////////////////////////////////////////////////////////////////
// Class				:	CMadeTexture
// Method				:	lookup
// Description			:	Point lookup
// Return Value			:	-
// Comments				:
// Date last edited		:	2/28/2002
void	 CMadeTexture::lookup(const miState* const state,
			      float *result,float s,float t,
			      const CTextureLookup& l)
{
   // Do the s mode
   switch(sMode) {
      case kTEXTURE_PERIODIC:
	 s = (float) math<float>::fmod(s,1);
	 if (s < 0) s += 1;
	 break;
      case kTEXTURE_BLACK:
	 if ((s < 0) || (s > 1)) {
	    initv(result,l.fill);
	    return;
	 }
	 break;
      case kTEXTURE_CLAMP:
	 if (s < 0)  s = 0;
	 if (s > 1)  s = 1;
	 break;
   }

   // Do the t mode
   switch(tMode) {
      case kTEXTURE_PERIODIC:
	 t = (float) math<float>::fmod(t,1);
	 if (t < 0) t += 1;
	 break;
      case kTEXTURE_BLACK:
	 if ((t < 0) || (t > 1)) {
	    initv(result,l.fill);
	    return;
	 }
	 break;
      case kTEXTURE_CLAMP:
	 if (t < 0)  t = 0;
	 if (t > 1)  t = 1;
	 break;
   }

   layers[0]->lookup(state, result,s,t,l);
}


///////////////////////////////////////////////////////////////////////
// Class    : CMadeTexture
// Method    : lookup4
// Description   : Area lookup
// Return Value   : -
// Comments    :
// Date last edited  : 2/28/2002
void  CMadeTexture::lookup4(const miState* const state,
			    float *result,const float *u,const float *v,
			    const CTextureLookup& lookup)
{
   int    i;
   float   totalContribution = 0;
   CTextureLayer *layer0,*layer1;
   float   offset;
   float   l;
   float   diag;
   const float  cs = (u[0] + u[1] + u[2] + u[3]) * (float) 0.25;
   const float  ct = (v[0] + v[1] + v[2] + v[3]) * (float) 0.25;
   float   ds,dt,d;

   ds  = u[0] - cs;
   dt  = v[0] - ct;
   diag = ds*ds*width*width + dt*dt*height*height;

   ds  = u[1] - cs;
   dt  = v[1] - ct;
   d  = ds*ds*width*width + dt*dt*height*height;
   diag = min(d,diag);

   ds  = u[2] - cs;
   dt  = v[2] - ct;
   d  = ds*ds*width*width + dt*dt*height*height;
   diag = min(d,diag);

   ds  = u[3] - cs;
   dt  = v[3] - ct;
   d  = ds*ds*width*width + dt*dt*height*height;
   diag = min(d,diag);
   // Find the layer that we want to probe
   l   = (float) (math<float>::log(diag)*0.5 / math<float>::log(2));
   l   = max(l,0.f);
   i   = (int) floor(l);
   if (i >= (numLayers-1)) i = numLayers-2;

   layer0  = layers[i];
   layer1  = layers[i+1];
   offset  = l - i;
   offset  = min(offset,1.0f);

   initvf(result,0);     // Result is black

   int counter = 0;
   double r[2];
   miUint samples = lookup.numSamples;
   while (mi_sample( r, &counter, const_cast< miState* >( state ), 
		     2, &samples ) )
   {
      float   s,t;
      vector   C,CC0,CC1;
      float   contribution;

      s     = ( (u[0]*(1.0f-(float)r[0]) +
		 u[1]*(float)r[0])*(1.0f-(float)r[1]) +
		(u[2]*(1.0f-(float)r[0]) + u[3]*(float)r[0]) * (float)r[1] );
      t     = ( (v[0]*(1.0f-(float)r[0]) + v[1]*(float)r[0]) *
		(1.0f-(float)r[1]) +
		(v[2]*(1.0f-(float)r[0]) + v[3]*(float)r[0]) * (float)r[1] );
      contribution  = lookup.filter((float) r[0]-0.5f,(float) r[1]-0.5f,1,1);
      totalContribution += contribution;

      // Do the s mode
      switch(sMode) {
	 case kTEXTURE_PERIODIC:
	    s = (float) fmod(s,1);
	    if (s < 0) s += 1;
	    break;
	 case kTEXTURE_BLACK:
	    if ((s < 0) || (s > 1)) {
	       continue;
	    }
	    break;
	 case kTEXTURE_CLAMP:
	    if (s < 0)  s = 0;
	    if (s > 1)  s = 1;
	    break;
      }

      // Do the t mode
      switch(tMode) {
	 case kTEXTURE_PERIODIC:
	    t = (float) fmod(t,1);
	    if (t < 0) t += 1;
	    break;
	 case kTEXTURE_BLACK:
	    if ((t < 0) || (t > 1)) {
	       continue;
	    }
	    break;
	 case kTEXTURE_CLAMP:
	    if (t < 0)  t = 0;
	    if (t > 1)  t = 1;
	    break;
      }

      // lookup (s,t) and add it to the result
      layer0->lookup(state, (float*)&CC0,s,t,lookup);
      layer1->lookup(state, (float*)&CC1,s,t,lookup);

      miScalar one_offset = 1.0f - offset;
      C.x = CC0.x * one_offset + CC1.x * offset;
      C.y = CC0.y * one_offset + CC1.y * offset;
      C.z = CC0.z * one_offset + CC1.z * offset;

      result[0]  += C[0]*contribution;
      result[1]  += C[1]*contribution;
      result[2]  += C[2]*contribution;
   }

   float tmp = 1 / totalContribution;
   mulvf(result,tmp);
}


///////////////////////////////////////////////////////////////////////
// Class  :	CRegularTexture
// Method  :	CRegularTexture
// Description 	:	Ctor
// Return Value 	:	-
// Comments  :
// Date last edited :	2/28/2002
CRegularTexture::CRegularTexture(const char *name,int w,int h,
				 TTextureMode s,TTextureMode t) :
CTexture(name,w,h,s,t),
layer( NULL )
{
}

///////////////////////////////////////////////////////////////////////
// Class  :	CRegularTexture
// Method  :	~CRegularTexture
// Description 	:	Dtor
// Return Value 	:	-
// Comments  :
// Date last edited :	2/28/2002
CRegularTexture::~CRegularTexture()
{
   delete layer;
}

///////////////////////////////////////////////////////////////////////
// Class  :	CRegularTexture
// Method  :	lookup
// Description 	:	Point lookup
// Return Value 	:	-
// Comments  :
// Date last edited :	2/28/2002
void  CRegularTexture::lookup( const miState* const state,
			       float *result,float s,float t,
			       const CTextureLookup& l)
{
   // Do the s mode
   switch(sMode) {
      case kTEXTURE_PERIODIC:
	 s = (float) fmod(s,1);
	 if (s < 0)	s += 1;
	 break;
      case kTEXTURE_BLACK:
	 if ((s < 0) || (s > 1)) {
	    initv(result,l.fill);
	    return;
	 }
	 break;
      case kTEXTURE_CLAMP:
	 if (s < 0) s = 0;
	 if (s > 1) s = 1;
	 break;
   }

   // Do the t mode
   switch(tMode) {
      case kTEXTURE_PERIODIC:
	 t = (float) fmod(t,1);
	 if (t < 0)	t += 1;
	 break;
      case kTEXTURE_BLACK:
	 if ((t < 0) || (t > 1)) {
	    initv(result,l.fill);
	    return;
	 }
	 break;
      case kTEXTURE_CLAMP:
	 if (t < 0) t = 0;
	 if (t > 1) t = 1;
	 break;
   }

   layer->lookup( state, result,s,t,l);
}


///////////////////////////////////////////////////////////////////////
// Class  :	CRegularTexture
// Method  :	lookup
// Description 	:	Area lookup
// Return Value 	:	-
// Comments  :	Default to point lookup
// Date last edited :	2/28/2002
void  CRegularTexture::lookup4( const miState* const state,
				float *result,const float *u,
				const float *v,const CTextureLookup& lookup)
{
   float totalContribution = 0;

   initvf(result,0); // Result is black

   int counter = 0;
   double	r[2];
   miUint samples = lookup.numSamples;
   while (mi_sample( r, &counter, const_cast< miState* >( state ), 
		     2, &samples ) )
   {
      float 	s,t;
      vector 	C;
      float 	contribution;

      s   = ( (u[0]*(1.0f-(float)r[0]) + u[1]*(float)r[0])*(1.0f-(float)r[1]) +
	      (u[2]*(1.0f-(float)r[0]) + u[3]*(float)r[0])*(float)r[1] );
      t   = ( (v[0]*(1.0f-(float)r[0]) + v[1]*(float)r[0])*(1.0f-(float)r[1]) +
	      (v[2]*(1.0f-(float)r[0]) + v[3]*(float)r[0])*(float)r[1] );
      contribution = lookup.filter((float)r[0]-0.5f,(float)r[1]-0.5f,
				    1.0f,1.0f);
      totalContribution += contribution;

      // Do the s mode
      switch(sMode) {
	 case kTEXTURE_PERIODIC:
	    s = (float) fmod(s,1);
	    if (s < 0.0f) s += 1.0f;
	    break;
	 case kTEXTURE_BLACK:
	    if ((s < 0.0f) || (s > 1.0f)) {
	       continue;
	    }
	    break;
	 case kTEXTURE_CLAMP:
	    if (s < 0.0f) s = 0.0f;
	    if (s > 1.0f) s = 1.0f;
	    break;
      }

      // Do the t mode
      switch(tMode) {
	 case kTEXTURE_PERIODIC:
	    t = (float) fmod(t,1);
	    if (t < 0.0f) t += 1.0f;
	    break;
	 case kTEXTURE_BLACK:
	    if ((t < 0.0f) || (t > 1.0f)) {
	       continue;
	    }
	    break;
	 case kTEXTURE_CLAMP:
	    if (t < 0.0f) t = 0.0f;
	    if (t > 1.0f) t = 1.0f;
	    break;
      }
      // lookup (s,t) and add it to the result
      layer->lookup( state, (float*)&C, s, t, lookup);

      result[0] += C[0]*contribution;
      result[1] += C[1]*contribution;
      result[2] += C[2]*contribution;
   }

   float	tmp = 1.0f / totalContribution;
   mulvf(result,tmp);
}




// For side access
typedef enum {
PX,
PY,
PZ,
NX,
NY,
NZ,
} ESide;

typedef enum {
XYZ,
XZY,
YXZ,
YZX,
ZXY,
ZYX
} EOrder;


///////////////////////////////////////////////////////////////////////
// Class  :	CEnvironment
// Method  :	CEnvironment
// Description 	:	Ctor
CEnvironment::CEnvironment(const char *n) :
name( mi_mem_strdup(n) )
{
}

///////////////////////////////////////////////////////////////////////
// Class  :	CEnvironment
// Method  :	~CEnvironment
// Description 	:	Dtor
CEnvironment::~CEnvironment()
{
}

///////////////////////////////////////////////////////////////////////
// Class  :	CEnvironment
// Method  :	Lookup
// Description 	:	Environment lookup
void 	CEnvironment::lookup( const miState* const state,
			      float *result,const float *D,const float *Du,
			      const float *Dv,const CTextureLookup& lookup)
{
   result[0] = 1.0f-lookup.fill[0];
   result[1] = 1.0f-lookup.fill[1];
   result[2] = 1.0f-lookup.fill[2];
}

CShadow::CShadow(const char *n,float *em,CTextureLayer *s) :
CEnvironment(n)
{
//     movmm(toNDC,em);
   memcpy(toNDC, em, sizeof( miMatrix ) );
   side  = s;
}

CShadow::~CShadow()
{
   if (side != NULL)	delete side;
}

//! Shadow lookup
void 	CShadow::lookup( const miState* const state, float *result,
			 const float *D,const float *Du,
			 const float *Dv,const CTextureLookup& lookup)
{
   float totalContribution = 0;
   float blur  = 1 + lookup.blur;

   result[0] = 0;
   
   int    counter = 0;
   double 	r[2];
   miUint samples = lookup.numSamples;
   while (mi_sample( r, &counter, const_cast< miState* >( state ), 
		     2, &samples ) )
   {
      float	x,y;
      float	s,t;
      float	C;
      float	contribution;

      x   = (float)r[0] - 0.5f;	// Assume x,y are gaussian samples
      y   = (float)r[1] - 0.5f;
      contribution = lookup.filter(x,y,1.0f,1.0f);
      totalContribution += contribution;

      x *= blur;
      y *= blur;

      //        float	tmp[4],cP[4];
      //        mulmp4(tmp,toNDC,cP);
      //        s   = tmp[0] / tmp[3];
      //        t   = tmp[1] / tmp[3];
      //
      // Note:  cP.z was not / w (tmp[3])
      point cP( D[0] +	Du[0]*x + Dv[0]*y,
		D[1] +	Du[1]*x + Dv[1]*y,
		D[2] +	Du[2]*x + Dv[2]*y );

      
      cP *= toNDC;
      s = cP.x;
      t = cP.y;
      if ((s < 0) || (s > 1) || (t < 0) || (t > 1)) {
	 continue;
      }

      side->lookupz(state, &C,s,t,lookup);

      // Note:  cP.z was not / w (tmp[3])
      if ((cP.z - lookup.shadowBias) > C)	{
	 result[0] += contribution;
      }
   }


   result[0]	/= totalContribution;
   result[1] = result[0];
   result[2] = result[0];
}


///////////////////////////////////////////////////////////////////////
// Class  :	CDeepShadow
// Method  :	CDeepShadow
// Description 	:	Ctor
CDeepShadow::CDeepShadow(const char *n,const char *fn,
			 const float *toWorld,FILE *in) :
CEnvironment(n)
{
   int i,k;
   miMatrix	mtmp;

   fileName = strdup(fn);

   // Read the header
   fread(&header,sizeof(CDeepShadowHeader),1,in);

//     mulmm(mtmp,header.toNDC,toWorld);
//     movmm(header.toNDC,mtmp);
   mi_matrix_prod( mtmp, header.toNDC, toWorld );
   memcpy(header.toNDC, mtmp, sizeof( miMatrix ) );

   // Read the tile end indices
   tileIndices = new int[header.xTiles*header.yTiles];
   fread(tileIndices,sizeof(int),header.xTiles*header.yTiles,in);

   // Save the index start
   fileStart = ftell(in);

   // Init the tiles
   tiles = new CDeepTile*[header.yTiles];
   for (k=0,i=0;i<header.yTiles;i++) {
      int	j;

      tiles[i] = new CDeepTile[header.xTiles];

      for (j=0;j<header.xTiles;j++,k++) {
	 CDeepTile	*cTile = tiles[i]+j;
	 int 	size;

	 if (k == 0)	size = tileIndices[k] - fileStart;
	 else size = tileIndices[k] - tileIndices[k-1];

	 cTile->block = textureNewBlock(size);
	 cTile->data  = new float*[header.tileSize*header.tileSize];
	 cTile->lastData = new float*[header.tileSize*header.tileSize];
      }
   }

   fclose(in);
}

///////////////////////////////////////////////////////////////////////
// Class  :	CDeepShadow
// Method  :	~CDeepShadow
// Description 	:	Dtor
CDeepShadow::~CDeepShadow()
{
   int	i,j;

   for (j=0;j<header.yTiles;j++) {
      for (i=0;i<header.xTiles;i++) {
	 textureDeleteBlock(tiles[j][i].block);
	 delete [] tiles[j][i].lastData;
	 delete [] tiles[j][i].data;
      }
      delete [] tiles[j];
   }
   delete [] tiles;

   delete [] tileIndices;

   free(fileName);
}


///////////////////////////////////////////////////////////////////////
// Class  :	CDeepShadow
// Method  :	loadTile
// Description 	:	Cache in a tile
void	CDeepShadow::loadTile(int x,int y)
{
   int 	index = y*header.xTiles+x;
   CDeepTile	*cTile = tiles[y]+x;
   FILE *in = fopen(fileName,"rb");
   float **cData;
   float **cLastData;
   float *data;
   int 	i;
   int 	startIndex;

   mrASSERT(in != NULL);

   if (index == 0)	startIndex = fileStart;
   else 	startIndex = tileIndices[index-1];

   textureAllocateBlock(cTile->block);
   fseek(in,startIndex,SEEK_SET);
   fread(cTile->block->data,sizeof(unsigned char),cTile->block->size,in);
   fclose(in);

   data  = (float *) cTile->block->data;
   cLastData = cTile->lastData;
   cData  = cTile->data;
   for (i=header.tileSize*header.tileSize;i>0;i--) {
      cData[0] = data;
      cLastData[0] = data;
      cData++;
      cLastData++;

      if (i != 1) {
	 data += 4;
	 while(*data != -INFINITY)	data += 4;
      }
   }
}

///////////////////////////////////////////////////////////////////////
// Class  :	CDeepShadow
// Method  :	lookup
// Description 	:	shadow lookup
void	CDeepShadow::lookup( const miState* const state,
			     float *result,const float *D,const float *Du,
			     const float *Dv,const CTextureLookup& lookup)
{
   float totalContribution = 0;

   result[0] = 0;
   result[1] = 0;
   result[2] = 0;
   
   int    counter = 0;
   double 	r[2];
   miUint samples = lookup.numSamples;
   while (mi_sample( r, &counter, const_cast< miState* >( state ), 
		     2, &samples ) )
   {
      float x,y; // Assume x,y are gaussian samples
      float s,t,w;
      float contribution;
      int 	px,py;
      int 	bx,by;
      CDeepTile	*cTile;
      float *cPixel;

      x   = (float)r[0] - 0.5f;
      y   = (float)r[1] - 0.5f;
      contribution = lookup.filter(x,y,1.0f,1.0f);
      totalContribution += contribution;


      //        float	tmp[4],cP[4];
      //        mulmp4(tmp,header.toNDC,cP);
      //        s   = tmp[0] / tmp[3];
      //        t   = tmp[1] / tmp[3];
      //
      // Note:  cP.z was not / w (tmp[3])
      point cP( D[0] +	Du[0]*x + Dv[0]*y,
		D[1] +	Du[1]*x + Dv[1]*y,
		D[2] +	Du[2]*x + Dv[2]*y );

      
      cP *= header.toNDC;
      s = cP.x;
      t = cP.y;
      if ((s < 0) || (s >= 1) || (t < 0) || (t >= 1)) {
	 continue;
      }

      s  	*= header.xres;
      t  	*= header.yres;
      // Note:  cP.z was not / w (tmp[3])
      w   = cP.z - lookup.shadowBias;

      px   = fastmath<float>::floor(s);
      py   = fastmath<float>::floor(t);
      bx   = px >> header.tileShift;
      by   = py >> header.tileShift;
      px   = px & ((1 << header.tileShift) - 1);
      py   = py & ((1 << header.tileShift) - 1);

      cTile  = tiles[by]+bx;

      if (cTile->block->data == NULL)	loadTile(bx,by);

      cPixel  = cTile->lastData[py*header.tileSize+px];

      while(miTRUE) {
	 if (cPixel[0] > w) cPixel	-= 4;
	 else if (cPixel[4] < w)	cPixel += 4;
	 else {
	    const float	alpha = (w - cPixel[0]) / (cPixel[4] - cPixel[0]);

	    result[0] += (1-((1-alpha)*cPixel[1] + alpha*cPixel[5]))*contribution;
	    result[1] += (1-((1-alpha)*cPixel[2] + alpha*cPixel[6]))*contribution;
	    result[2] += (1-((1-alpha)*cPixel[3] + alpha*cPixel[7]))*contribution;

	    cTile->lastData[py*header.tileSize+px] = cPixel;

	    break;
	 }
      }

      //	0	-	z
      //	1	-	r
      //	2	-	g
      //	3	-	b

      //	4	-	z
      //	5	-	r
      //	6	-	g
      //	7	-	b
   }

   result[0]	/= totalContribution;
   result[1]	/= totalContribution;
   result[2]	/= totalContribution;
}

///////////////////////////////////////////////////////////////////////
// Class  :	CCubicEnvironment
// Method  :	CCubicEnvironment
// Description 	:	Ctor
CCubicEnvironment::CCubicEnvironment(const char *n,CTexture **s) :
CEnvironment(n)
{
   sides[0] = s[0];
   sides[1] = s[1];
   sides[2] = s[2];
   sides[3] = s[3];
   sides[4] = s[4];
   sides[5] = s[5];
}

///////////////////////////////////////////////////////////////////////
// Class  :	CCubicEnvironment
// Method  :	~CCubicEnvironment
// Description 	:	Dtor
CCubicEnvironment::~CCubicEnvironment()
{
   delete sides[0];
   delete sides[1];
   delete sides[2];
   delete sides[3];
   delete sides[4];
   delete sides[5];
}

///////////////////////////////////////////////////////////////////////
// Class  :	CCubicEnvironment
// Method  :	Lookup
// Description 	:	Environment lookup
void 	CCubicEnvironment::lookup( const miState* const state, float *result,
				   const float *D,
				   const float *Du,const float *Dv,
				   const CTextureLookup& lookup)
{
   EOrder order;
   float c;
   int 	axis;
   int 	uaxis,vaxis;
   vector cDu,cDv;
   CTexture	*side;
   vector u,v;
   float t;

   addvv(cDu,D,Du);
   addvv(cDv,D,Dv);

   // Find the side of the cube that we're looking at
   if (math<float>::fabs(D[1]) > math<float>::fabs(D[0])) {
      if (math<float>::fabs(D[2]) > math<float>::fabs(D[1])) {
	 order = ZYX;
      } else {
	 if (math<float>::fabs(D[2]) > math<float>::fabs(D[0]))
	    order = YZX;
	 else
	    order = YXZ;
      }
   } else if (math<float>::fabs(D[2]) > math<float>::fabs(D[1])) {
      if (math<float>::fabs(D[2]) > math<float>::fabs(D[0]))
	 order = ZXY;
      else
	 order = XZY;
   } else {
      order = XYZ;
   }

   switch(order) {
      case XYZ:
      case XZY:
	 if (D[0] > 0) {	side = sides[PX];	c = 1;	}
	 else  {	side = sides[NX];	c = -1;	}
	 axis = 0;
	 uaxis = 1;
	 vaxis = 2;
	 break;
      case YXZ:
      case YZX:
	 if (D[1] > 0) {	side = sides[PY];	c = 1;	}
	 else  {	side = sides[NY];	c = -1;	}
	 axis = 1;
	 uaxis = 0;
	 vaxis = 2;
	 break;
      case ZXY:
      case ZYX:
	 if (D[2] > 0) {	side = sides[PZ];	c = 1;	}
	 else  {	side = sides[NZ];	c = -1;	}
	 axis = 2;
	 uaxis = 0;
	 vaxis = 1;
	 break;
   }


   t = c / D[axis];
   u[0] = D[uaxis]*t;
   v[0] = D[vaxis]*t;

   t = c / cDu[axis];
   u[1] = cDu[uaxis]*t - u[0];
   v[1] = cDu[vaxis]*t - v[0];

   t = c / cDv[axis];
   u[2] = cDv[uaxis]*t - u[0];
   v[2] = cDv[vaxis]*t - v[0];

   side->lookup4(state,result,(float*)&u,(float*)&v,lookup);
}


///////////////////////////////////////////////////////////////////////
// Class  :	CSphericalEnvironment
// Method  :	CSphericalEnvironment
// Description 	:	Ctor
// Return Value 	:	-
// Comments  :
// Date last edited :	2/28/2002
CSphericalEnvironment::CSphericalEnvironment(const char *n,CTexture *s) :
CEnvironment(n)
{
   side = s;
}

///////////////////////////////////////////////////////////////////////
// Class  :	CSphericalEnvironment
// Method  :	~CSphericalEnvironment
// Description 	:	Dtor
// Return Value 	:	-
// Comments  :
// Date last edited :	2/28/2002
CSphericalEnvironment::~CSphericalEnvironment()
{
   if (side != NULL) delete side;
}

///////////////////////////////////////////////////////////////////////
// Class  :	CSphericalEnvironment
// Method  :	Lookup
// Description 	:	Environment lookup
// Return Value 	:	-
// Comments  :
// Date last edited :	2/28/2002
void 	CSphericalEnvironment::lookup( const miState* const state,
				       float *result,const float *D,
				       const float *Du,const float *Dv,
				       const CTextureLookup& lookup)
{
   vector cDu,cDv;
   vector u,v;
   float m;

   addvv(cDu,D,Du);
   addvv(cDv,D,Dv);

   m   = (float) (2*sqrt(D[0]*D[0] + D[1]*D[1] + (D[2]+1)*(D[2]+1)));
   u[0]  = D[0] / m + (float) 0.5;
   v[0]  = D[1] / m + (float) 0.5;

   m   = (float) (2*sqrt(cDu[0]*cDu[0] + cDu[1]*cDu[1] + (cDu[2]+1)*(cDu[2]+1)));
   u[1]  = cDu[0] / m + (float) 0.5 - u[0];
   v[1]  = cDu[1] / m + (float) 0.5 - v[0];

   m   = (float) (2*sqrt(cDv[0]*cDv[0] + cDv[1]*cDv[1] + (cDv[2]+1)*(cDv[2]+1)));
   u[2]  = cDv[0] / m + (float) 0.5 - u[0];
   v[2]  = cDv[1] / m + (float) 0.5 - v[0];

   side->lookup4(state,result,(float*)&u,(float*)&v,lookup);
}


///////////////////////////////////////////////////////////////////////
// Function  :	readMadeTexture
// Description 	:	Read the pyramid layers
// Return Value 	:	miTRUE on success
// Comments  :
// Date last edited :	7/7/2001
template <class T>
static CTexture	*readMadeTexture(const char *name,const char *aname,TIFF *in,
				 int dstart,int width,int height,char *smode,
				 char *tmode,int pyramidSize,T enforcer)
{
   CMadeTexture 	*cTexture;
   int   i,j;
   uint32  	fileWidth,fileHeight;
   uint32  	tileWidth,tileHeight;
   uint16  	numSamples;
   int   cwidth,cheight;
   TTextureMode 	sMode,tMode;
   int   tileSize,tileSizeShift;

   fileWidth  = 0;
   fileHeight  = 0;
   numSamples  = 0;

   TIFFSetDirectory(in,dstart);
   TIFFGetFieldDefaulted(in,TIFFTAG_IMAGEWIDTH,      &fileWidth);
   TIFFGetFieldDefaulted(in,TIFFTAG_IMAGELENGTH,     &fileHeight);
   TIFFGetFieldDefaulted(in,TIFFTAG_SAMPLESPERPIXEL, &numSamples);
   TIFFGetFieldDefaulted(in,TIFFTAG_TILEWIDTH,       &tileWidth);
   TIFFGetFieldDefaulted(in,TIFFTAG_TILELENGTH,      &tileHeight);

   mrASSERT(tileWidth == tileHeight);

   tileSize = tileWidth;

   if (strcmp(smode,RI_PERIODIC) == 0) {
      sMode = kTEXTURE_PERIODIC;
   } else if (strcmp(smode,RI_CLAMP) == 0) {
      sMode = kTEXTURE_CLAMP;
   } else if (strcmp(smode,RI_BLACK) == 0) {
      sMode = kTEXTURE_BLACK;
   } else {
//        error(CODE_BADTOKEN,"Unknown texture wrap mode (\"%s\").",smode);
      mi_error("Unknown texture wrap mode in s (\"%s\").",smode);
      sMode = kTEXTURE_PERIODIC;
   }

   if (strcmp(tmode,RI_PERIODIC) == 0) {
      tMode = kTEXTURE_PERIODIC;
   } else if (strcmp(tmode,RI_CLAMP) == 0) {
      tMode = kTEXTURE_CLAMP;
   } else if (strcmp(tmode,RI_BLACK) == 0) {
      tMode = kTEXTURE_BLACK;
   } else {
//        error(CODE_BADTOKEN,"Unknown texture wrap mode (\"%s\").",tmode);
      mi_error("Unknown texture wrap mode in t (\"%s\").",smode);
      tMode = kTEXTURE_PERIODIC;
   }

   cTexture = new CMadeTexture(aname,width,height,sMode,tMode);

   for (i=1,j=0;i != tileSize;i = i << 1,j++);
   tileSizeShift = j;

   cTexture->numLayers = pyramidSize;
   cTexture->layers = new CTextureLayer*[pyramidSize];

   cwidth  = width;
   cheight  = height;
   for (i=0;i<pyramidSize;i++)
   {
      TIFFGetFieldDefaulted(in,TIFFTAG_IMAGEWIDTH,  &fileWidth);
      TIFFGetFieldDefaulted(in,TIFFTAG_IMAGELENGTH, &fileHeight);
      cTexture->layers[i] = new CTiledTexture<T>(name,dstart+i,cwidth,cheight,
						 numSamples,fileWidth,
						 fileHeight,tileSize,
						 tileSizeShift);
      
      if (i != (pyramidSize-1))
	 TIFFSetDirectory(in,dstart+i+1);

      cwidth = cwidth >> 1;
      cheight = cheight >> 1;
   }

   return cTexture;
}

///////////////////////////////////////////////////////////////////////
// Function  :	readTexture
// Description 	:	read a regular texture
// Return Value 	:	The texture
// Comments  :
// Date last edited :	8/10/2001
template <class T>
static CTexture	*readTexture(const char *name,const char *aname,TIFF *in,
			     int dstart,T enforcer)
{
   uint32  width,height;
   uint16  numSamples;
   CRegularTexture *cTexture;

   width  = 0;
   height  = 0;
   TIFFGetFieldDefaulted(in,TIFFTAG_IMAGEWIDTH,      &width);
   TIFFGetFieldDefaulted(in,TIFFTAG_IMAGELENGTH,     &height);
   TIFFGetFieldDefaulted(in,TIFFTAG_SAMPLESPERPIXEL, &numSamples);

   cTexture  = new CRegularTexture(aname,width,height,
				   kTEXTURE_PERIODIC,kTEXTURE_PERIODIC);

   cTexture->width = width;
   cTexture->height = height;
   cTexture->layer = new CBasicTexture<T>(name,dstart,width,height,
					  numSamples,width,height);

   return cTexture;
}

///////////////////////////////////////////////////////////////////////
// Function  :	texLoad
// Description 	:	Load a texture from disk
// Return Value 	:	Pointer to the new texture
// Comments  :
// Date last edited :	7/7/2001
static	CTexture	*texLoad(const char *name,const char *aname,TIFF *in,
				 int &dstart,char *textureSpec)
{
   int  pyramidSize;
   CTexture *cTexture = NULL;
   char 	smode[32],tmode[32];
   int  width,height;
   uint16 	bitspersample;

   TIFFSetDirectory(in,dstart);
   TIFFGetFieldDefaulted(in,TIFFTAG_BITSPERSAMPLE, &bitspersample);
	
   cTexture = NULL;

   strcpy(smode,RI_PERIODIC);
   strcpy(tmode,RI_PERIODIC);

   if (textureSpec == NULL)
   { // Not a made texture
      pyramidSize = 1;
   }
   else
   {
      if (sscanf(textureSpec,
		 "#texture (%dx%d): smode: %s tmode: %s levels: %d ",
		 &width,&height,smode,tmode,&pyramidSize) == 5)
      {
	 if (bitspersample == 8)
	 {
	    cTexture = readMadeTexture<unsigned char>(name,aname,in,dstart,
						      width, height,smode,
						      tmode,pyramidSize,1);
	 } else
	 {
	    cTexture = readMadeTexture<float>(name,aname,in,dstart,
					      width,height,smode,tmode,
					      pyramidSize,1);
	 }
      }
      else
      {
	 pyramidSize = 1;
      }
   }

   if (cTexture == NULL) {
      if (bitspersample == 8) {
	 cTexture = readTexture<unsigned char>(name,aname,in,dstart,1);
      } else {
	 cTexture = readTexture<float>(name,aname,in,dstart,1);
      }
   }

   dstart += pyramidSize;

   return cTexture;
}


////////////////////////////////////////////////////////////////////////
// TextureBase methods


///////////////////////////////////////////////////////////////////////
// Function  :	textureInit
// Description 	:	This function is called before any texturemapping stuff to init
// Return Value :	miTRUE on success
void textureInit(int maxMemory)
{
   if ( usedTextureMemory == 0 )
   {
      refNumber  = 0; 	// Last texture fererence number
      usedBlocks  = NULL;
      freeBlocks  = NULL;
      numUsedBlocks  = 0;
      usedTextureMemory = 0;
      maxTextureMemory = maxMemory;
   }
}

///////////////////////////////////////////////////////////////////////
// Function  :	textureShutdown
// Description 	:	Delete everything about textures
// Return Value :	miTRUE on success
void textureShutdown()
{
   CTextureBlock	*cBlock,*nBlock;

   while(usedBlocks != NULL)
      textureDeleteBlock(usedBlocks);

   for (cBlock=freeBlocks;cBlock!=NULL;) {
      nBlock = cBlock->next;
      delete cBlock;
      cBlock = nBlock;
   }
}

struct TSearchpath
{
};


miBoolean locateFile( char* const fn, const char* const name,
		      TSearchpath *path )
{
   strcpy( fn, name );
   return miTRUE;
}

///////////////////////////////////////////////////////////////////////
// Function  :	textureLoad
// Description 	:	Load a texture from disk
// Return Value :	Pointer to the new texture
CTexture* textureLoad(const char *name,TSearchpath *path) {
   TIFF 	*in;
   CTexture *cTexture = NULL;
   char 	fn[OS_MAX_PATH_LENGTH];
   int  directory = 0;

   if (locateFile(fn,name,path) == miFALSE)
      return NULL;

   // Set the error handler so we don't crash
   TIFFSetErrorHandler(tiffErrorHandler);
   TIFFSetWarningHandler(tiffWarningHandler);
   
   // Open the texture
   in  = TIFFOpen(fn,"r");
   if (in == NULL)
   {
      mi_error("Could not open TIFF file \"%s\".", fn);
      return NULL;
   }
   
   char	*textureSpec = NULL;
   char	tmp[1024];
   
   if (TIFFGetField(in,TIFFTAG_IMAGEDESCRIPTION, &textureSpec) == 1)
   {
      strcpy(tmp,textureSpec);
      textureSpec = tmp;
      
      if (strncmp(textureSpec,"#texture",8) == 0)
	 cTexture = texLoad(fn,name,in,directory,textureSpec);
      else
	 cTexture = texLoad(fn,name,in,directory,NULL);
   }
   else
   {
      cTexture = texLoad(fn,name,in,directory,NULL);
   }
   
   TIFFClose(in);

   return cTexture;
}


///////////////////////////////////////////////////////////////////////
// Function  :	environmentLoad
// Description 	:	Load an environment from disk
// Return Value 	:	Pointer to the new environment
CEnvironment* environmentLoad(const char *name,
			      TSearchpath *path,
			      float *toWorld)
{
   TIFF 	*in;
   char 	fileName[OS_MAX_PATH_LENGTH];
   char 	*ext = NULL;
   CEnvironment	*cTexture = NULL;
   miMatrix 	trans;

   if (locateFile(fileName,name,path) == miFALSE)
      return NULL;

   // Check if the file is a transparency shadow map
//     FILE 	*tmpin;
//     tmpin = ropen(fileName,"rb",fileTransparencyShadow,miTRUE);
//     if (tmpin != NULL)
//        return new CDeepShadow(name, fileName, toWorld, tmpin);

   // Set the error handler so we don't crash
   TIFFSetErrorHandler(tiffErrorHandler);
   TIFFSetWarningHandler(tiffWarningHandler);
   
   // Open the texture
   in  = TIFFOpen(fileName,"r");

   if (in != NULL)
   {
      char  tmp[1024];
      char  *textureSpec = NULL;
      miMatrix  envMat;

      if (TIFFGetField(in,TIFFTAG_IMAGEDESCRIPTION, &textureSpec) == 1)
      {
	 strcpy(tmp,textureSpec);
	 textureSpec = tmp;

	 if (sscanf(textureSpec,"#cenvironment"))
	 {
	    // We're loading a cubic environment 
	    int 	directory = 0;
	    char *cSpec;
	    int 	i;
	    CTexture	*sides[6];

	    cSpec = strstr(textureSpec,"#texture");

	    if (cSpec == NULL)
	    {
//  	       error(CODE_BADFILE,"Missing side in %s\n",fileName);
	       mi_error("Missing side in %s",fileName);
	    }
	    else
	    {
	       for (i=0;i<6;i++)
	       {
		  if (cSpec == NULL)
		  {
		     // This shound't have happened
		     int	j;

		     mi_error("Missing side in %s",fileName);
//  		     error(CODE_BADFILE,"Missing side in %s\n",fileName);

		     for (j=0;j<i;j++)	delete sides[j];

		     break;
		  }
		  else
		  {
		     sides[i] = texLoad(fileName,name,in,directory,cSpec);

		     cSpec += 8;

		     cSpec = strstr(cSpec,"#texture");
		  }
	       }

	       if (i == 6) {
		  cTexture = new CCubicEnvironment(name,sides);
	       }
	    }
	 }
	 else if (sscanf(textureSpec,"#senvironment")) {
	    int 	directory = 0;
	    char *cSpec;
	    CTexture	*side;

	    cSpec = strstr(textureSpec,"#texture");

	    if (cSpec == NULL)
	    {
	       mi_error("Missing texture in %s",fileName);
	    }
	    else
	    {
	       side = texLoad(fileName,name,in,directory,cSpec);
	       cTexture = new CSphericalEnvironment(name,side);
	    }
	 }
	 else if ( (sscanf(textureSpec,
			   "WorldToNDC=[ %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ] ",
			   &envMat[0],  &envMat[1],  &envMat[2],  &envMat[3],
			   &envMat[4],  &envMat[5],  &envMat[6],  &envMat[7],
			   &envMat[8],  &envMat[9],  &envMat[10], &envMat[11],
			   &envMat[12], &envMat[13], &envMat[14], &envMat[15]) == 16) || 
		   (sscanf(textureSpec,
			   "#shadow [ %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ]",
			   &envMat[0],  &envMat[1],  &envMat[2],  &envMat[3],
			   &envMat[4],  &envMat[5],  &envMat[6],  &envMat[7],
			   &envMat[8],  &envMat[9],  &envMat[10], &envMat[11],
			   &envMat[12], &envMat[13], &envMat[14], &envMat[15]) == 16) )
	 {
	    CTextureLayer *side;
	    unsigned short bitspersample,numSamples;
	    unsigned int width,height;

	    TIFFGetFieldDefaulted(in,TIFFTAG_BITSPERSAMPLE,   &bitspersample);
	    TIFFGetFieldDefaulted(in,TIFFTAG_IMAGEWIDTH,      &width);
	    TIFFGetFieldDefaulted(in,TIFFTAG_IMAGELENGTH,     &height);
	    TIFFGetFieldDefaulted(in,TIFFTAG_SAMPLESPERPIXEL, &numSamples);

	    // Only one sided environment map
	    if (TIFFIsTiled(in))
	    {
	       unsigned int tileWidth,tileHeight;

	       TIFFGetFieldDefaulted(in,TIFFTAG_TILEWIDTH,  &tileWidth);
	       TIFFGetFieldDefaulted(in,TIFFTAG_TILELENGTH, &tileHeight);

	       if (tileWidth == tileHeight)
	       {
		  int	tileSize = tileWidth;
		  int	tileSizeShift = 0;

		  for (;tileWidth > 1;tileWidth = tileWidth >> 1)
		     tileSizeShift++;

		  if (bitspersample == 8)
		     side = new CTiledTexture<unsigned char>(fileName,0,width,
							     height,
							     numSamples,
							     width,height,
							     tileSize,
							     tileSizeShift);
		  else
		     side = new CTiledTexture<float>(fileName,0,width,height,
						     numSamples,width,height,
						     tileSize,tileSizeShift);
	       }
	       else
	       {
		  if (bitspersample == 8)
		     side = new CBasicTexture<unsigned char>(fileName,0,width,
							     height,
							     numSamples,
							     width,height);
		  else
		     side = new CBasicTexture<float>(fileName,0,width,height,
						     numSamples,width,height);
	       }
	    }
	    else
	    {
	       if (bitspersample == 8)
		  side = new CBasicTexture<unsigned char>(fileName,0,width,
							  height,numSamples,
							  width,height);
	       else
		  side = new CBasicTexture<float>(fileName,0,width,height,
						  numSamples,width,height);
	    }

	    // Compute the transformation matrix to the light space
//  	    mulmm(trans,envMat,toWorld);
	    mi_matrix_prod( trans, envMat, toWorld);

	    cTexture = new CShadow(name,trans,side);
	 }
      }

      TIFFClose(in);
   }

   return cTexture;
}


END_NAMESPACE( mr )


#endif
