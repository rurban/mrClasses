
#include <string>

#include "math.h"
#include "stdlib.h"
#include "shader.h"
#include "geoshader.h"


#define EXTERN_C extern "C"

////////////////// Output shader
struct gg_buffers_t
{
     miTag   filename;
};

EXTERN_C DLLEXPORT int gg_buffers_version(void) {return(1);}

const char* EXT = ".iff";
#define FMT miIMG_FORMAT_IFF
#define MAX_USER_BUFFERS 8

EXTERN_C DLLEXPORT miBoolean 
gg_buffers(
	   miColor* const result,
	   miState* const state,
	   struct gg_buffers_t* p
	   )
{

#if 0
   // Extract  filename from color image output name...
   
   //  If you look at camera->output, you'll find a list of miFunction tags
   //  chained with miFunction::next_function. Look for functions where
   //  miFunction::type == miFUNCTION_OUTFILE, and get a pointer

   //      miDecl_fileparm *p = (miDecl_fileparm *)func->parameters;

   //  You'll find miDecl_fileparm in geoshader.h. It contains a file name.

   // Ok, that is how it is supposed to be, but the code below crashes,
   // as next_function returns crap.

   char* n = NULL;
   miTag current = state->camera->output;
   do {
      miFunction* f = (miFunction*) mi_db_access( current );
      if ( f->type == miFUNCTION_OUTFILE )
      {
	 miDecl_fileparm *p = (miDecl_fileparm *)f->parameters;
	 n = p->filename;
	 mi_db_unpin( current );
	 break;
      }
      mi_db_unpin( current );

      current = f->next_function;
   } while ( current != miNULLTAG );

   if ( n == NULL )
   {
      mi_error("Could not determine output file name");
      n = "dummy";
   }
   std::string file( n );
   
#else
   miTag filenameTag = *mi_eval_tag( &p->filename );
   const char* n = (char*) mi_db_access( filenameTag );
   std::string file( n ); 
   mi_db_unpin( filenameTag );
#endif
   
   int xres = state->camera->x_resolution;
   int yres = state->camera->y_resolution;
   
   // get subwindow
   int xl = state->camera->window.xl;
   int yl = state->camera->window.yl;
   int xh = state->camera->window.xh;
   int yh = state->camera->window.yh;
   if ( xh > xres ) xh = xres;
   if ( yh > yres ) yh = yres;
   
   int bxres = xh - xl;
   int byres = yh - yl;
   if ( bxres < xres || byres < yres )
   {
      mi_info("Just a preview, I assume.  Not saving buffers.");
      return miTRUE;
   }

   char tmp[10];
   itoa(state->camera->frame,tmp,10);
   std::string frame( tmp );
   if ( state->camera->frame_field )
   {
      itoa(state->camera->frame_field,tmp,10);
      frame += ".";
      frame += tmp;
   }
   
   miOptions* o = state->options;
   
   miImg_image* buf;
   buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_Z].p );
   if ( buf )
   {
     std::string name( file );
     name += ".Z.";
     name += frame;
     name += EXT;
     mi_info("Saving out depth channel image as \"%s\"", name.c_str());

     miImg_file fd;
     fd.width = bxres;
     fd.height = byres;
     fd.bits = 16;
     fd.comp = 1;
     fd.filter = miFALSE;
     fd.topdown = miFALSE;
     fd.gamma = 1;
     fd.aspect = state->camera->aspect/xres*yres;
     fd.type = miIMG_TYPE_Z;
     fd.format = FMT;
     fd.error = miIMG_ERR_NONE;
     fd.os_error = 0;
     
     if ( mi_img_create( &fd, mi_mem_strdup( name.c_str() ),
			 fd.type, FMT, bxres, byres ) )
     {
	if (! mi_img_image_write( &fd, buf ) )
	   mi_error("Could not write depth channel image.");
	mi_img_close( &fd );
     }
     else
     {
	mi_error("Could not open depth channel image.");
     }
  }
  else
  {
     mi_info("Ignoring depth channel");
  }
  
  buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_N].p );
  if ( buf )
  {
     std::string name( file );
     name += ".N.";
     name += frame;
     name += EXT;
     mi_info("Saving out normal channel image as \"%s\"", name.c_str());

     miImg_file fd;
     fd.width = bxres;
     fd.height = byres;
     fd.bits = 16;
     fd.comp = 1;
     fd.filter = miFALSE;
     fd.topdown = miFALSE;
     fd.gamma = 1;
     fd.aspect = state->camera->aspect/xres*yres;
     fd.type = miIMG_TYPE_N;
     fd.format = FMT;
     fd.error = miIMG_ERR_NONE;
     fd.os_error = 0;
     
     if ( mi_img_create( &fd, mi_mem_strdup( name.c_str() ),
			 fd.type, FMT, bxres, byres ) )
     {
	if (! mi_img_image_write( &fd, buf ) )
	   mi_error("Could not write normal channel image.");
	mi_img_close( &fd );
     }
     else
     {
	mi_error("Could not open normal channel image.");
     }
  }
  else
  {
     mi_info("Ignoring normal channel");
  }



  buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_M].p );
  if ( buf )
  {
     std::string name( file );
     name += ".M.";
     name += frame;
     name += EXT;
     mi_info("Saving out motion channel image as \"%s\"", name.c_str());

     miImg_file fd;
     fd.width = bxres;
     fd.height = byres;
     fd.bits = 16;
     fd.comp = 1;
     fd.filter = miFALSE;
     fd.topdown = miFALSE;
     fd.gamma = 1;
     fd.aspect = state->camera->aspect/xres*yres;
     fd.type = miIMG_TYPE_M;
     fd.format = FMT;
     fd.error = miIMG_ERR_NONE;
     fd.os_error = 0;
     
     if ( mi_img_create( &fd, mi_mem_strdup( name.c_str() ),
			 fd.type, FMT, bxres, byres ) )
     {
	if (! mi_img_image_write( &fd, buf ) )
	   mi_error("Could not write motion channel image.");
	mi_img_close( &fd );
     }
     else
     {
	mi_error("Could not open motion channel image.");
     }
  }
  else
  {
     mi_info("Ignoring motion channel");
  }





  buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_TAG].p );
  if ( buf )
  {
     std::string name( file );
     name += ".TAG.";
     name += frame;
     name += EXT;
     mi_info("Saving out tag channel image as \"%s\"", name.c_str());

     miImg_file fd;
     fd.width = bxres;
     fd.height = byres;
     fd.bits = 16;
     fd.comp = 1;
     fd.filter = miFALSE;
     fd.topdown = miFALSE;
     fd.gamma = 1;
     fd.aspect = state->camera->aspect/xres*yres;
     fd.type = miIMG_TYPE_TAG;
     fd.format = FMT;
     fd.error = miIMG_ERR_NONE;
     fd.os_error = 0;
     
     if ( mi_img_create( &fd, mi_mem_strdup( name.c_str() ),
			 fd.type, FMT, bxres, byres ) )
     {
	if (! mi_img_image_write( &fd, buf ) )
	   mi_error("Could not write tag channel image.");
	mi_img_close( &fd );
     }
     else
     {
	mi_error("Could not open tag channel image.");
     }
  }
  else
  {
     mi_info("Ignoring tag channel");
  }






  buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_COVERAGE].p );
  if ( buf )
  {
     std::string name( file );
     name += ".COV.";
     name += frame;
     name += EXT;
     mi_info("Saving out coverage channel image as \"%s\"", name.c_str());

     miImg_file fd;
     fd.width = bxres;
     fd.height = byres;
     fd.bits = 16;
     fd.comp = 1;
     fd.filter = miFALSE;
     fd.topdown = miFALSE;
     fd.gamma = 1;
     fd.aspect = state->camera->aspect/xres*yres;
     fd.type = miIMG_TYPE_COVERAGE;
     fd.format = FMT;
     fd.error = miIMG_ERR_NONE;
     fd.os_error = 0;
     
     if ( mi_img_create( &fd, mi_mem_strdup( name.c_str() ),
			 fd.type, FMT, bxres, byres ) )
     {
	if (! mi_img_image_write( &fd, buf ) )
	   mi_error("Could not write coverage channel image.");
	mi_img_close( &fd );
     }
     else
     {
	mi_error("Could not open coverage channel image.");
     }
  }
  else
  {
     mi_info("Ignoring coverage channel");
  }


  for (int i = 0; i < MAX_USER_BUFFERS; ++i )
  {
     buf = static_cast< miImg_image* >( o->image[miRC_IMAGE_USER+i].p );
     if ( buf )
     {
	std::string name( file );
	char c[3];  itoa(i,c,10);
	name += ".USER";
	name += c;
	name += ".";
	name += frame;
	name += EXT;
	mi_info("Saving out user buffer #%d as \"%s\"", i, name.c_str());
	
	miImg_file fd;
	fd.width = bxres;
	fd.height = byres;
	fd.bits = 16;
	fd.comp = 1;
	fd.filter = miFALSE;
	fd.topdown = miFALSE;
	fd.gamma = 1;
	fd.aspect = state->camera->aspect/xres*yres;
	fd.type = o->image_types[miRC_IMAGE_USER+i];
	fd.format = FMT;
	fd.error = miIMG_ERR_NONE;
	fd.os_error = 0;
	
	if ( mi_img_create( &fd, mi_mem_strdup( name.c_str() ),
			    fd.type, FMT, bxres, byres ) )
	{
	   if (! mi_img_image_write( &fd, buf ) )
	      mi_error("Could not write user buffer #%d.",i);
	   mi_img_close( &fd );
	}
	else
	{
	   mi_error("Could not open image for user buffer #%d.",i);
	}
     }
     else
     {
	mi_info("Ignoring user buffer #%d channel", i);
     }
  }

  
  return(miTRUE);
}
