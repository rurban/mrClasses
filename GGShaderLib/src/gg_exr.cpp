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
/******************************************************************************
 * Created:	02.02.04
 * Module:	gg_exr
 *
 * Exports:
 *      gg_exr(), gg_exr_version()
 *
 * Requires:
 *      mrClasses, ILM's EXR library
 *
 * History:
 *      07.05.03: initial version
 *
 * Description:
 *      This output shader saves all mray channels into one or several
 *      .exr images.
 *
 *
 *****************************************************************************/
//-----------------------------------------------------------------------------
//
//	mental ray output shader that outputs
//	floating-point image files, using ILM's IlmImf library.
//
//      It will automatically save out all image channels found,
//      including user buffers.  For details on .mi file and
//	shader syntax, see the Programming mental ray manual
//      (Output shaders).
//
//      The output shader allows saving all channels as a single file or
//      each channel as a separate file (always using RGBA).
//	In a single file, this shader maps mental ray's output variables to 
//	image channels as follows:
//
//	Renderman output	image channel		image channel
//	variable name		name			type
//	--------------------------------------------------------------
//
//	"r"			"R"			HALF
//
//	"g"			"G"			HALF
//
//	"b"			"B"			HALF
//
//	"a"			"A"			HALF
//
//	"z"			"Z"			FLOAT
//
//      "n"                     "N.0001.x"              FLOAT
//                              "N.0001.y"              FLOAT
//                              "N.0001.z"              FLOAT
//
//      "m"                     "dPdtime.0001.x"        FLOAT
//                              "dPdtime.0001.y"        FLOAT
//                              "dPdtime.0001.z"        FLOAT
//
//      "tag"                   "TAG"                   UINT
//
//      "coverage"              "COVERAGE"              FLOAT
//
//	user			same as output		preferred type
//				variable name           (see below)
//                              (ex.  USER.0000.r ) 		
//
//	By default, the "preferred" channel type is HALF; the
//	preferred type can be changed by adding an "pixeltype"
//	argument to the Display command in the RIB file.
//	For example:
//
//	    # Store point positions in FLOAT format
//          output  "gg_exr" ( "filename" "test.exr", "pixeltype" 1 )
//
//	The default compression method for the image's pixel data
//	is zip.  You can select a different compression method by
//      adding a value for "compression" to the output command.
//      For example:
//
//          output  "gg_exr" ( "filename" "test.exr",
//                             "padding" 4, "compression" 1 )
//
//	See function parseParameters(), below, for a list of valid
//	"pixeltype" and "compression" values.
//
//-----------------------------------------------------------------------------


#define MAX_USER_BUFFERS 8
#define SHADER_VERSION   1


#include <string>
#include <vector>
#include <iostream>
#include <limits>

#include "mrGenerics.h"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfIntAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfLut.h>
#include <ImfArray.h>
#include <ImathFun.h>
#include <Iex.h>
#include <half.h>
#include <halfFunction.h>


using namespace Imath;
using namespace Imf;
using namespace std;
using namespace mr;

struct gg_exr_t
{
     miBoolean singleFile;
     miTag     filename;
     miInteger padding;
     miInteger pixelType;
     miInteger compression;
};




enum CompressionTypes
{
kNO_COMPRESSION,
kRLE_COMPRESSION,
kPIZ_COMPRESSION,
kPIZ12_COMPRESSION,
kZIP_COMPRESSION,
kZIPS_COMPRESSION,
};

enum PixelTypes
{
kHALF_PixelType,
kFLOAT_PixelType,
kUINT_PixelType,
};


namespace {


//! Turn an image type into a char step size (ie. how many chars to skip)
int type2size( miImg_type type )
{
   switch( type )
   {
      case miIMG_TYPE_RGBA:
      case miIMG_TYPE_RGB:
	 return 1;
      case miIMG_TYPE_RGBA_16:
      case miIMG_TYPE_RGB_16:
	 return 2;
      case miIMG_TYPE_A:
      case miIMG_TYPE_S:
	 return 1;
      case miIMG_TYPE_A_16:
      case miIMG_TYPE_S_16:
	 return 2;
      case miIMG_TYPE_VTA:
	 return 2;
      case miIMG_TYPE_VTS:
	 return 2;
      case miIMG_TYPE_Z:
	 return 4;
      case miIMG_TYPE_N:
	 return 4;
      case miIMG_TYPE_M:
	 return 4;
      case miIMG_TYPE_TAG:
	 return 4;
      case miIMG_TYPE_BIT:
	 return 1;
      case miIMG_TYPE_RGB_FP:
	 return 4;
      case miIMG_TYPE_RGBA_FP:
	 return 4;
      case miIMG_TYPE_COVERAGE:
	 return 4;
      case miIMG_TYPE_RGBE:
	 return 4;
      case miIMG_TYPE_A_FP:
	 return 2;
      case miIMG_TYPE_S_FP:
	 return 4;
      default:
	 mi_error("Unknown miIMG_TYPE_ (%d)",type);
	 return 4;
   }
}


struct imgInfo
{
     miImg_image* img;
     miImg_type   type;
     int component;
};

typedef std::map < string, imgInfo >   	ChannelOffsetMap;
typedef std::vector <halfFunction <half> *>	ChannelLuts;

//
// Define halfFunctions for the identity and piz12
//
half	    	    halfID( half x ) { return x; }

halfFunction <half> id( halfID );
halfFunction <half> piz12( round12log );

class Image
{
   public:

     Image (const char filename[],
	    const Header &header,
	    ChannelOffsetMap &mrayInfo,
	    ChannelLuts &channelLuts);
     
   private:
     std::vector <int>	_bufferChannelOffsets;
     Array <char> 	_buffer;
     OutputFile		_file;
     ChannelLuts&   _channelLuts;
};


Image::Image (
	      const char filename[],
	      const Header &header,
	      ChannelOffsetMap &mrayInfo,
	      ChannelLuts &channelLuts
	      )
:
_file (filename, header),
_channelLuts (channelLuts)
{

   FrameBuffer  fb;
    
   int _bufferXMin  = header.dataWindow().min.x;
   int _bufferYMin  = header.dataWindow().min.y;
   int _bufferXMax  = header.dataWindow().max.x + 1;
   int _bufferYMax  = header.dataWindow().max.y + 1;
   int width  = _bufferXMax - _bufferXMin;
   int height = _bufferYMax - _bufferYMin;

   int bufferPixelSize = 0;
    
   for (ChannelList::ConstIterator i = header.channels().begin();
	i != header.channels().end();
	++i)
   {
      int size;
      switch (i.channel().type)
      {
	 case Imf::UINT:
	    size = sizeof( unsigned int );
	    break;
	 case Imf::HALF:
	    size = sizeof (float);    // Note: to avoid alignment
	    break;			// problems when float and half
	    // channels are mixed, halfs
	 case Imf::FLOAT:		// are not packed densely.
	    size = sizeof (float);
	    break;

	 default:
	    mrFAIL("unsupported channel type");
	    break;
      }
	
      _bufferChannelOffsets.push_back( bufferPixelSize );
      bufferPixelSize += size;
   }

    
   _buffer.resizeErase (width * height * bufferPixelSize);

   // Create empty buffers...
    
   int     	 xStride = bufferPixelSize;
   int     	 yStride = width * bufferPixelSize;
   char    	*base = &_buffer[0] -
   _bufferXMin * bufferPixelSize;

   int j = 0;
   for (ChannelList::ConstIterator i = header.channels().begin();
	i != header.channels().end(); ++i, ++j)
   {
      fb.insert (i.name(),
		 Slice (i.channel().type,			// type
			base + _bufferChannelOffsets[j],  	// base
			xStride,		            	// xStride
			yStride,	    	    	    	// yStride
			1,					// xSampling
			1));					// ySampling
   }

   // Add data...
   char    *toBase = &_buffer[0];
   int       toInc = bufferPixelSize;
   j = 0;
    
   for (ChannelList::ConstIterator i = header.channels().begin();
	i != header.channels().end(); ++i, ++j)
   {
      const char* name = i.name();
      const imgInfo& info = mrayInfo[ name ];
      mi_progress("gg_exr:  Saving channel \"%s\" for \"%s\".",
		  name, filename);
      miImg_image* f = info.img;
      int comp    = info.component;
      miImg_type type = info.type;
      int fromInc = type2size( type );
      char* to = toBase + _bufferChannelOffsets[j];
       
      switch (i.channel().type)
      {
	 case Imf::UINT:
	    {
	       switch ( type )
	       {
		  case miIMG_TYPE_RGBA_FP:
		  case miIMG_TYPE_RGB_FP:
		  case miIMG_TYPE_N:
		  case miIMG_TYPE_M:
		  case miIMG_TYPE_Z:
		  case miIMG_TYPE_S_FP:
		  case miIMG_TYPE_COVERAGE:
		     {
			mi_warning("%s buffer was created as a FP buffer.  "
				   "Saving it as uint reduces precision.", name);
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      float val = *(float *) from;
			      val *= numeric_limits<unsigned int>::max();
			      unsigned int tmp = (unsigned int) val;
			      *(unsigned int *) to = tmp;
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
		      
		  case miIMG_TYPE_RGBA_16:
		  case miIMG_TYPE_RGB_16:
		  case miIMG_TYPE_A_16:
		  case miIMG_TYPE_S_16:
		  case miIMG_TYPE_VTA:
		  case miIMG_TYPE_VTS:
		     {
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      unsigned short s = *(unsigned short *) from;
			      unsigned int tmp = s;
			      *(unsigned int *) to = tmp;
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
		  case miIMG_TYPE_RGBA:
		  case miIMG_TYPE_RGB:
		  case miIMG_TYPE_RGBE:
		  case miIMG_TYPE_BIT:
		  case miIMG_TYPE_A:
		  case miIMG_TYPE_S:
		     {
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      unsigned char  s = *(unsigned char *) from;
			      unsigned int tmp = s;
			      *(unsigned int *) to = tmp;
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
	       }
	       break;
	    }
	     
	 case Imf::HALF:
	    {
	       halfFunction <half> &lut = *_channelLuts[j];

	       switch ( type )
	       {
		  case miIMG_TYPE_RGBA_FP:
		  case miIMG_TYPE_RGB_FP:
		  case miIMG_TYPE_N:
		  case miIMG_TYPE_M:
		  case miIMG_TYPE_Z:
		  case miIMG_TYPE_S_FP:
		  case miIMG_TYPE_COVERAGE:
		     {
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      *(half *) to = lut( ( half )( *(float *) from ) );
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
		      
		  case miIMG_TYPE_RGBA_16:
		  case miIMG_TYPE_RGB_16:
		  case miIMG_TYPE_A_16:
		  case miIMG_TYPE_S_16:
		  case miIMG_TYPE_VTA:
		  case miIMG_TYPE_VTS:
		     {
			mi_warning("%s buffer was created as a 16-bit buffer."
				   "  Saving exr is wasteful.", name);
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      unsigned short s = *(unsigned short *) from;
			      float tmp = (float)s / 65535.0f;
			      *(half *) to = lut( ( half )( tmp ) );
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
		  case miIMG_TYPE_RGBA:
		  case miIMG_TYPE_RGB:
		  case miIMG_TYPE_RGBE:
		  case miIMG_TYPE_BIT:
		  case miIMG_TYPE_A:
		  case miIMG_TYPE_S:
		     {
			mi_warning("%s buffer was created as an 8-bit buffer."
				   "  Saving exr is wasteful.", name);
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      unsigned char s = *(unsigned char *) from;
			      float tmp = (float)s / 255.0f;
			      *(half *) to = lut( ( half )( tmp ) );
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
	       }
	       break;
	    }

	 case Imf::FLOAT:
	    {
	       switch ( type )
	       {
		  case miIMG_TYPE_RGBA_FP:
		  case miIMG_TYPE_RGB_FP:
		  case miIMG_TYPE_N:
		  case miIMG_TYPE_M:
		  case miIMG_TYPE_Z:
		  case miIMG_TYPE_S_FP:
		  case miIMG_TYPE_COVERAGE:
		     {
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      *(float *) to = *(float *) from;
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
		      
		  case miIMG_TYPE_RGBA_16:
		  case miIMG_TYPE_RGB_16:
		  case miIMG_TYPE_A_16:
		  case miIMG_TYPE_S_16:
		  case miIMG_TYPE_VTA:
		  case miIMG_TYPE_VTS:
		     {
			mi_warning("%s buffer was created as a 16-bit buffer."
				   "  Saving exr is wasteful.", name);
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      unsigned short s = *(unsigned short *) from;
			      float tmp = (float)s / 65535.0f;
			      *(float *) to = tmp;
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
		  case miIMG_TYPE_RGBA:
		  case miIMG_TYPE_RGB:
		  case miIMG_TYPE_RGBE:
		  case miIMG_TYPE_BIT:
		  case miIMG_TYPE_A:
		  case miIMG_TYPE_S:
		     {
			mi_warning("%s buffer was created as an 8-bit buffer."
				   "  Saving exr is wasteful.", name);
			for ( int y = _bufferYMax-1; y >= _bufferYMin; --y )
			{
			   miUchar* from = miIMG_ACCESS( f, y, comp );
			   for ( int x = _bufferXMin; x < _bufferXMax; ++x )
			   {
			      unsigned char s = *(unsigned char *) from;
			      float tmp = (float)s / 255.0f;
			      *(float *) to = tmp;
			      to += toInc;
			      from += fromInc;
			   }
			}
			break;
		     }
	       }
	       break;
	    }
      }

   }

    
   _file.setFrameBuffer (fb);
   _file.writePixels( height );

   _bufferChannelOffsets.clear();
   _channelLuts.clear();
}



} // namespace




EXTERN_C DLLEXPORT int gg_exr_version() { return(SHADER_VERSION); };




EXTERN_C DLLEXPORT
miBoolean gg_exr(
		 miColor*              result,
		 register miState*     state,
		 struct gg_exr_t*      p
		 )
{
   char* filename = NULL;
   try
   {
      //
      // Get root filename
      //

      miTag filenameTag = *mi_eval_tag( &p->filename );
      filename = mi_mem_strdup( (char*) mi_db_access( filenameTag ) );
      mi_db_unpin( filenameTag );


      //
      // Get frame, with padding if needed and field info.
      //
      char tmp[10];
      char spec[5];
      int padding = *mi_eval_integer( &p->padding );
      sprintf(spec,"%%0%dd", padding );
      sprintf(tmp, spec, state->camera->frame);
      std::string frame( tmp );
      if ( state->camera->frame_field )
      {
	 if ( state->camera->frame_field == 1 )
	    frame += ".1";
	 else
	    frame += ".2";
      }

	
      //
      // Are we saving a single file or not?
      //
      miBoolean singleFile = *mi_eval_boolean( &p->singleFile );
	
      //
      // Build an output file header
      //

      Header	    	     header;
      ChannelOffsetMap     channelOffsets;
      ChannelLuts 	     channelLuts;
      int 	    	     pixelSize = 0;

      halfFunction <half> *rgbLUT = &id;
      halfFunction <half> *otherLUT = &id;

      //
      // Comments
      //
      {
	 char c[256];
	 sprintf(c,"Created with mental ray - exr output shader - v%d",
		 SHADER_VERSION);
	 header.insert("comments", StringAttribute(c) );
      }

	
      //
      // Creation/Capture Date ( Exif format: "YYYY:MM:DD HH:MM:SS" )
      //
      {
	 struct tm *newtime;
	 time_t aclock;
	 time( &aclock );
	 newtime = localtime( &aclock );
	 char c[256];
	 sprintf(c,"%04d:%02d:%02d %02d:%02d:%02d",
		 newtime->tm_year+1900, newtime->tm_mon, newtime->tm_mday,
		 newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	 header.insert("capDate", StringAttribute( c ) );
      }

	
      //
      // Data window
      //

      {
	 Box2i &dw = header.dataWindow();

	 dw.min.x = state->camera->window.xl;
	 dw.min.y = state->camera->window.yl;
	 dw.max.x = state->camera->window.xh;
	 dw.max.y = state->camera->window.yh;
	 if ( dw.max.x > state->camera->x_resolution )
	    dw.max.x = state->camera->x_resolution;
	 if ( dw.max.y > state->camera->y_resolution )
	    dw.max.y = state->camera->y_resolution;
	 dw.max.x -= 1;
	 dw.max.y -= 1;
      }

      //
      // Display window
      //

      {
	 Box2i &dw = header.displayWindow();

	 dw.min.x  = 0;
	 dw.min.y  = 0;
	 dw.max.x  = state->camera->x_resolution - 1;
	 dw.max.y  = state->camera->y_resolution - 1;
      }

	
      //
      // Camera parameters
      //

      {

	 //
	 // The matrices reflect the orientation of the camera at
	 // render time.
	 //
	 miMatrix* m;
	 mi_query( miQ_TRANS_WORLD_TO_CAMERA, state, miNULLTAG,
		   &m );
	 matrix a( *m );
	 M44f Nl( a[0][0], a[0][1], a[0][2], a[0][3],
		  a[1][0], a[1][1], a[1][2], a[1][3],
		  a[2][0], a[2][1], a[2][2], a[2][3],
		  a[3][0], a[3][1], a[3][2], a[3][3] );
	 header.insert ("worldToCamera", M44fAttribute(Nl));
	   
	    
	 //
	 // Projection matrix
	 //
	 matrix P( state );

	 // Concat it to world matrix
	 a *= P;
	 M44f NP( a[0][0], a[0][1], a[0][2], a[0][3],
		  a[1][0], a[1][1], a[1][2], a[1][3],
		  a[2][0], a[2][1], a[2][2], a[2][3],
		  a[3][0], a[3][1], a[3][2], a[3][3] );
	 header.insert ("worldToNDC", M44fAttribute (NP));
	   
	    
	 header.insert ("clipNear", FloatAttribute( state->camera->clip.min ));
	 header.insert ("clipFar", FloatAttribute( state->camera->clip.max ) );

	   
	 //
	 // Derive pixel aspect ratio, screen window width, screen
	 // window center from projection matrix.
	 //
	   
	 Box2f sw (V2f ((-1 - P[3][0] - P[2][0]) / P[0][0],
			(-1 - P[3][1] - P[2][1]) / P[1][1]),
		   V2f (( 1 - P[3][0] - P[2][0]) / P[0][0],
			( 1 - P[3][1] - P[2][1]) / P[1][1]));
	   
	 header.screenWindowWidth() = sw.max.x - sw.min.x;
	 header.screenWindowCenter() = (sw.max + sw.min) / 2;
	   
	 const Box2i &dw = header.displayWindow();
	   
	 header.pixelAspectRatio()   = ( (sw.max.x - sw.min.x) /
					 (sw.max.y - sw.min.y) *
					 (dw.max.y - dw.min.y + 1) /
					 (dw.max.x - dw.min.x + 1) );
      }

      //
      // Line order
      //

      header.lineOrder() = INCREASING_Y;

      //
      // Compression
      //

      {

	 miInteger comp = *mi_eval_integer(&p->compression);
	 switch (comp)
	 {
	    case kNO_COMPRESSION:
	       header.compression() = NO_COMPRESSION; break;
	    case kRLE_COMPRESSION:
	       header.compression() = RLE_COMPRESSION; break;
	    case kZIPS_COMPRESSION:
	       header.compression() = ZIPS_COMPRESSION; break;
	    case kZIP_COMPRESSION:
	       header.compression() = ZIP_COMPRESSION; break;
	    case kPIZ_COMPRESSION:
	       header.compression() = PIZ_COMPRESSION; break;
	    case kPIZ12_COMPRESSION:
	       header.compression() = PIZ_COMPRESSION;
	       rgbLUT = &piz12;
	       break;
	    default:
	       THROW (Iex::ArgExc,
		      "Invalid exrcompression \"" << comp << "\" "
		      "for image file " << filename << ".");
	 }
      }

      //
      // Channel list
      //

      {
	 PixelType pixelType = HALF;

	 miInteger ptype = *mi_eval_integer(&p->pixelType);
	 switch( ptype )
	 {
	    case kFLOAT_PixelType:
	       pixelType = Imf::FLOAT;
	       break;
	    case kHALF_PixelType:
	       pixelType = Imf::HALF;
	       break;
	    case kUINT_PixelType:
	       pixelType = Imf::UINT;
	       break;
	    default:
	       THROW (Iex::ArgExc,
		      "Invalid exrpixeltype \"" << ptype << "\" "
		      "for image file " << filename << ".");
	 }
	   
	 ChannelList &channels = header.channels();

	 miOptions* o = state->options;
	 miImg_image* buf;
	 buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_RGBA].p );
	   
	 imgInfo info;
	    
	 channels.insert ("R", Channel (HALF));
	 info.type = o->image_types[miRC_IMAGE_RGBA];
	 info.img = buf;  info.component = miIMG_R;
	 channelOffsets["R"] = info;
	 channelLuts.push_back( rgbLUT );
	    
	 channels.insert ("G", Channel (HALF));
	 info.img = buf;  info.component = miIMG_G;
	 channelOffsets["G"] = info;
	 channelLuts.push_back( rgbLUT );
	    
	 channels.insert ("B", Channel (HALF));
	 info.img = buf;  info.component = miIMG_B;
	 channelOffsets["B"] = info;
	 channelLuts.push_back( rgbLUT );
	    
	 channels.insert ("A", Channel (HALF));
	 info.img = buf;  info.component = miIMG_A;
	 channelOffsets["A"] = info;
	 channelLuts.push_back( otherLUT );

	
	 if ( !singleFile )
	 {
	    //
	    // Open and save the output file
	    //
	    std::string name( filename );
	    name += "." + frame + ".exr";
	      
	    Image image( name.c_str(), header, channelOffsets, channelLuts);
	 }   

	   
	 buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_Z].p );

	 if ( buf )
	 {
	    info.type = o->image_types[miRC_IMAGE_Z];
	    info.img = buf;  info.component = miIMG_Z;
	    channelLuts.push_back( otherLUT );
	      
	      
	    if ( !singleFile )
	    {
	       header.channels() = ChannelList();
	       channels = header.channels();
	       channels.insert ("R", Channel (Imf::FLOAT));
	       channelOffsets["R"] = info;
		 
	       //
	       // Open and save the output file
	       //
	       std::string name( filename );
	       name += ".Z." + frame + ".exr";
		 
	       Image image( name.c_str(), header, channelOffsets, channelLuts);
	    }
	    else
	    {
	       channels.insert ("Z", Channel (Imf::FLOAT));
	       channelOffsets["Z"] = info;
	    }
	 }
	    
	 buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_N].p );
	 if ( buf )
	 {
	    if ( !singleFile )
	    {
	       header.channels() = ChannelList();
	       channels = header.channels();
	       
	       info.type = o->image_types[miRC_IMAGE_N];
	       info.img = buf;  info.component = miIMG_NX;
	       channels.insert ("R", Channel (Imf::FLOAT));
	       channelOffsets["R"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       channels.insert ("G", Channel (Imf::FLOAT));
	       info.component = miIMG_NY;
	       channelOffsets["G"] = info;
	       channelLuts.push_back( otherLUT );
	       
	       channels.insert ("B", Channel (Imf::FLOAT));
	       info.component = miIMG_NZ;
	       channelOffsets["B"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       //
	       // Open and save the output file
	       //
	       std::string name( filename );
	       name += ".N." + frame + ".exr";
		 
	       Image image( name.c_str(), header,
			    channelOffsets, channelLuts );
	    }
	    else
	    {
	       info.type = o->image_types[miRC_IMAGE_N];
	       info.img = buf;  info.component = miIMG_NX;
	       channels.insert ("N.0001.x", Channel (Imf::FLOAT));
	       channelOffsets["N.0001.x"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       channels.insert ("N.0001.y", Channel (Imf::FLOAT));
	       info.component = miIMG_NY;
	       channelOffsets["N.0001.y"] = info;
	       channelLuts.push_back( otherLUT );
	       
	       channels.insert ("N.0001.z", Channel (Imf::FLOAT));
	       info.component = miIMG_NZ;
	       channelOffsets["N.0001.z"] = info;
	       channelLuts.push_back( otherLUT );
	    }
	      
	 }
	    
	 buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_M].p );
	 if ( buf )
	 {
	    if ( !singleFile )
	    {
	       header.channels() = ChannelList();
	       channels = header.channels();
	       
	       info.type = o->image_types[miRC_IMAGE_N];
	       info.img = buf;  info.component = miIMG_NX;
	       channels.insert ("R", Channel (Imf::FLOAT));
	       channelOffsets["R"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       channels.insert ("G", Channel (Imf::FLOAT));
	       info.component = miIMG_NY;
	       channelOffsets["G"] = info;
	       channelLuts.push_back( otherLUT );
	       
	       channels.insert ("B", Channel (Imf::FLOAT));
	       info.component = miIMG_NZ;
	       channelOffsets["B"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       //
	       // Open and save the output file
	       //
	       std::string name( filename );
	       name += ".dPdtime." + frame + ".exr";
		 
	       Image image( name.c_str(), header,
			    channelOffsets, channelLuts );
	    }
	    else
	    {
	       channels.insert ("dPdtime.0001.x", Channel (Imf::FLOAT));
	       info.type = o->image_types[miRC_IMAGE_M];
	       info.img = buf;  info.component = miIMG_NX;
	       channelOffsets["dPdtime.0001.x"] = info;
	       channelLuts.push_back( otherLUT );
	       
	       channels.insert ("dPdtime.0001.y", Channel (Imf::FLOAT));
	       info.component = miIMG_NY;
	       channelOffsets["dPdtime.0001.y"] = info;
	       channelLuts.push_back( otherLUT );
	       
	       channels.insert ("dPdtime.0001.z", Channel (Imf::FLOAT));
	       info.component = miIMG_NZ;
	       channelOffsets["dPdtime.0001.z"] = info;
	       channelLuts.push_back( otherLUT );
	    }
	 }
	    
	 buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_TAG].p );
	 if ( buf )
	 {
	    if ( !singleFile )
	    {
	       header.channels() = ChannelList();
	       channels = header.channels();
	       
	       channels.insert ("R", Channel (Imf::UINT));
	       info.type = o->image_types[miRC_IMAGE_TAG];
	       info.img = buf;  info.component = 0;
	       channelOffsets["R"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       //
	       // Open and save the output file
	       //
	       std::string name( filename );
	       name += ".TAG." + frame + ".exr";
		 
	       Image image( name.c_str(), header,
			    channelOffsets, channelLuts );
	    }
	    else
	    {
	       channels.insert ("TAG", Channel (Imf::UINT));
	       info.type = o->image_types[miRC_IMAGE_TAG];
	       info.img = buf;  info.component = 0;
	       channelOffsets["TAG"] = info;
	       channelLuts.push_back( otherLUT );
	    }
	 }

	   
	 buf = static_cast< miImg_image* >(
					   o->image[miRC_IMAGE_COVERAGE].p
					   );
	 if ( buf )
	 {
	    if ( !singleFile )
	    {
	       header.channels() = ChannelList();
	       channels = header.channels();
	       
	       channels.insert ("R", Channel (Imf::FLOAT));
	       info.type = o->image_types[miRC_IMAGE_COVERAGE];
	       info.img = buf;  info.component = 0;
	       channelOffsets["R"] = info;
	       channelLuts.push_back( otherLUT );
		 
	       //
	       // Open and save the output file
	       //
	       std::string name( filename );
	       name += ".COV." + frame + ".exr";
		 
	       Image image( name.c_str(), header,
			    channelOffsets, channelLuts );
	    }
	    else
	    {
	       channels.insert ("COVERAGE", Channel (Imf::FLOAT));
	       info.type = o->image_types[miRC_IMAGE_COVERAGE];
	       info.img = buf;  info.component = 0;
	       channelOffsets["COVERAGE"] = info;
	       channelLuts.push_back( otherLUT );
	    }
	 }
	    
	 for (int i = 0; i < MAX_USER_BUFFERS; ++i )
	 {
	    buf = static_cast< miImg_image* >(
					      o->image[miRC_IMAGE_USER+i].p
					      );
	    if ( buf )
	    {
	       if ( singleFile )
	       {
		  std::string root( "USER." );
		  char c[5];
		  sprintf(c,"%04d",i);
		  root += c;
		  root += ".";
		  std::string name( root );

		  switch( info.type )
		  {
		     case miIMG_TYPE_RGBA:
		     case miIMG_TYPE_RGBA_16:
		     case miIMG_TYPE_RGBA_FP:
		     case miIMG_TYPE_RGBE:
			{
			   name = root + "r";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   
			   name = root + "g";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_G;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );

			   name = root + "b";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_B;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   
			   name = root + "a";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_A;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   break;
			}
			
		     case miIMG_TYPE_RGB:
		     case miIMG_TYPE_RGB_16:
		     case miIMG_TYPE_RGB_FP:
			{
			   name = root + "r";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.type = o->image_types[miRC_IMAGE_USER+i];
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   
			   name = root + "g";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_G;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );

			   name = root + "b";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_B;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   break;
			}
		     case miIMG_TYPE_N:
		     case miIMG_TYPE_M:
			{
			   name += "x";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.type = o->image_types[miRC_IMAGE_USER+i];
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   
			   name = root + "y";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_G;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );

			   name = root + "z";
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_B;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   break;
			}
		     case miIMG_TYPE_COVERAGE:
		     case miIMG_TYPE_BIT:
		     case miIMG_TYPE_TAG:
		     case miIMG_TYPE_VTA:
		     case miIMG_TYPE_VTS:
		     case miIMG_TYPE_Z:
		     case miIMG_TYPE_A:
		     case miIMG_TYPE_S:
		     case miIMG_TYPE_A_16:
		     case miIMG_TYPE_S_16:
		     case miIMG_TYPE_A_FP:
		     case miIMG_TYPE_S_FP:
			{
			   name = root;
			   channels.insert( name.c_str(), Channel(pixelType) );
			   info.type = o->image_types[miRC_IMAGE_USER+i];
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets[name] = info;
			   channelLuts.push_back( otherLUT );
			   break; 
			}
		  }
	       }
	       else
	       {
		  header.channels() = ChannelList();
		  channels = header.channels();

		  switch( info.type )
		  {
		     case miIMG_TYPE_RGBA:
		     case miIMG_TYPE_RGBA_16:
		     case miIMG_TYPE_RGBA_FP:
		     case miIMG_TYPE_RGBE:
			{
			   channels.insert( "R", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets["R"] = info;
			   channelLuts.push_back( otherLUT );
			   
			   channels.insert( "G", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_G;
			   channelOffsets["G"] = info;
			   channelLuts.push_back( otherLUT );

			   channels.insert( "B", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_B;
			   channelOffsets["B"] = info;
			   channelLuts.push_back( otherLUT );
			   
			   channels.insert( "A", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_A;
			   channelOffsets["A"] = info;
			   channelLuts.push_back( otherLUT );
			   break;
			}
			
		     case miIMG_TYPE_RGB:
		     case miIMG_TYPE_RGB_16:
		     case miIMG_TYPE_RGB_FP:
			{
			   channels.insert( "R", Channel(pixelType) );
			   info.type = o->image_types[miRC_IMAGE_USER+i];
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets["R"] = info;
			   channelLuts.push_back( otherLUT );
			   
			   channels.insert( "G", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_G;
			   channelOffsets["G"] = info;
			   channelLuts.push_back( otherLUT );

			   channels.insert( "B", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_B;
			   channelOffsets["B"] = info;
			   channelLuts.push_back( otherLUT );
			   break;
			}
		     case miIMG_TYPE_N:
		     case miIMG_TYPE_M:
			{
			   channels.insert( "R", Channel(pixelType) );
			   info.type = o->image_types[miRC_IMAGE_USER+i];
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets["R"] = info;
			   channelLuts.push_back( otherLUT );
			   
			   channels.insert( "G", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_G;
			   channelOffsets["G"] = info;
			   channelLuts.push_back( otherLUT );

			   channels.insert( "B", Channel(pixelType) );
			   info.img = buf;  info.component = miIMG_B;
			   channelOffsets["B"] = info;
			   channelLuts.push_back( otherLUT );
			   break;
			}
		     case miIMG_TYPE_COVERAGE:
		     case miIMG_TYPE_BIT:
		     case miIMG_TYPE_TAG:
		     case miIMG_TYPE_VTA:
		     case miIMG_TYPE_VTS:
		     case miIMG_TYPE_Z:
		     case miIMG_TYPE_A:
		     case miIMG_TYPE_S:
		     case miIMG_TYPE_A_16:
		     case miIMG_TYPE_S_16:
		     case miIMG_TYPE_A_FP:
		     case miIMG_TYPE_S_FP:
			{
			   channels.insert( "R", Channel(pixelType) );
			   info.type = o->image_types[miRC_IMAGE_USER+i];
			   info.img = buf;  info.component = miIMG_R;
			   channelOffsets["R"] = info;
			   channelLuts.push_back( otherLUT );
			   break; 
			}
		  }

		    
		  char c[5];
		  sprintf(c,"%04d",i);
		  std::string name( filename );
		  name += ".USER.";
		  name += c;
		  name += "." + frame + ".exr";
		  Image image( name.c_str(), header,
			       channelOffsets, channelLuts );
	       }
	    }
	 }
      }

      if ( singleFile )
      {
	 //
	 // Open and save the output file
	 //
	 std::string name( filename );
	 name += "." + frame + ".exr";
	
	 Image image( name.c_str(), header, channelOffsets, channelLuts );
      }
   }
   catch (const exception &e)
   {
      mi_error("OpenEXR: %s\n", e.what());
      mi_mem_release(filename);
      return miFALSE;
   }
   
   mi_mem_release(filename);
   return(miTRUE);
}
