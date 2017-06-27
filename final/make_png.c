#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bitmap.h"    

typedef struct {        // An RGBA pixel
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} pixel_t;
   
typedef struct  {       // A Picture
    pixel_t *pixels;
    size_t width;
    size_t height;
} bitmap_t;
    
/* Given "bitmap", this returns the pixel of bitmap at the point 
   ("x", "y"). */

static pixel_t * pixel_at (bitmap_t * bitmap, int x, int y)
{
    return bitmap->pixels + bitmap->width * y + x;
}
    
/* Write "bitmap" to a PNG file specified by "path"; returns 0 on
   success, non-zero on error. */

static int save_png_to_file (bitmap_t *bitmap, const char *path)
{
    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    /* "status" contains the return value of this function. At first
       it is set to a value which means 'failure'. When the routine
       has finished its work, it is set to a value which means
       'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
       see where it it is documented in the libpng manual.
    */
    int pixel_size = 4;
    int depth = 8;
    
    fp = fopen (path, "wb");
    if (! fp) {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }
    
    /* Set up error handling. */

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }
    
    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  bitmap->width,
                  bitmap->height,
                  depth,
                  PNG_COLOR_TYPE_RGBA,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    
    /* Initialize rows of PNG. */

    row_pointers = png_malloc (png_ptr, bitmap->height * sizeof (png_byte *));
    for (y = 0; y < bitmap->height; y++) {
        png_byte *row = 
            png_malloc (png_ptr, sizeof (uint8_t) * bitmap->width * pixel_size);
        row_pointers[y] = row;
        for (x = 0; x < bitmap->width; x++) {
            pixel_t * pixel = pixel_at (bitmap, x, y);
            *row++ = pixel->red;
            *row++ = pixel->green;
            *row++ = pixel->blue;
            *row++ = pixel->alpha;
        }
    }
    
    /* Write the image data to "fp". */

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* The routine has successfully written the file, so we set
       "status" to a value which indicates success. */

    status = 0;

    for (y = 0; y < bitmap->height; y++) {
        png_free (png_ptr, row_pointers[y]);
    }
    png_free (png_ptr, row_pointers);
    
 png_failure:
 png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
 png_create_write_struct_failed:
    fclose (fp);
 fopen_failed:
    return status;
}

int main (int argc, char const *argv[])
{
    FT_Library      library;
    FT_Face         face;
    FT_GlyphSlot    slot;

    FT_Bitmap*      bitmap;

    FT_Error        error;

    const char*     font_file;      // Arguments
    FT_UInt32       size;
    FT_UInt         glyph_index;
    const char*     character;

    if (argc != 4)
    {
        printf("\nTo render a RGBA PNG(s) of a glyph to be displayed on an \n");
        printf("LCD surface and to generate the 128bit MurmurHash3 hash \n\n");

        printf("Filter used is FT_LCD_FILTER_DEFAULT\n\n");

        printf("Usage   ./<exe> <font_file> <pt_size> <character>\n\n");

        exit(0);
    }

    font_file =     argv[1];
    size =          atoi(argv[2]);  
    character =     argv[3];

    error = FT_Init_FreeType( &library );
    if(error){
        printf("Error while initialising library\n");
    }

    error = FT_New_Face( library, 
                         font_file, 
                         0, 
                         &face );
    if(error){
        printf("Error loading new face\n");
    }

    error = FT_Set_Char_Size( face,
                              size * 64, 
                              0, 
                              100,
                              0 );
    if(error){
        printf("Error setting character size\n");
    }

    glyph_index = FT_Get_Char_Index( face, *character );

    slot = face->glyph;

    error = FT_Load_Glyph( face,
                           130, 
                           0);
    if(error){
        printf("Error loading glyph\n");
    }

    FT_Render_Glyph( slot, 
                     FT_RENDER_MODE_NORMAL);
    if(error){
        printf("Error rendering the glyph\n");
    }

    bitmap = &slot->bitmap;

    if (bitmap->width == 0 || bitmap->rows == 0)
    {
        return 1;
    }
     
    char * file_name = ( char * )calloc(20,sizeof(char));
    /* Write the image to a file 'fruit.png'. */
    sprintf(file_name, "%d_127.png",glyph_index);

    file_name = "out.png"; // remove this 

    bitmap_t fruit;
    int x;
    int y;

    /* Create an image. */

    fruit.width = bitmap->width;
    fruit.height = bitmap->rows;

    fruit.pixels = calloc (fruit.width * fruit.height, sizeof (pixel_t));

    if (! fruit.pixels) {
	return -1;
    }

    unsigned char value;
    for (y = 0; y < fruit.height; y++) {
        for (x = 0; x < fruit.width; x++) {
            pixel_t * pixel = pixel_at (& fruit, x, y);
            value = bitmap->buffer[(y * bitmap->pitch ) + x];
            pixel->red = 255- value;
            pixel->green = 255- value;
            pixel->blue = 255- value;
            if (value == 0)
            {
                pixel->alpha = 0;
            }else{
                pixel->alpha = 255;
            }
        }
    }

    save_png_to_file (& fruit, file_name);

    free (fruit.pixels);
    return 0;
}