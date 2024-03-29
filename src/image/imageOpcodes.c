
/**
 * IMAGE OPCODES
 *
 * imageOpcodes.c
 *
 * Copyright (c) 2007 by Cesare Marilungo. All rights reserved.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * C S O U N D   M A N   P A G E
 *
 * imageload            - opens an image (in PNG format)
 * imagesave            - saves an image (in PNG format)
 * imagecreate          - create an empty image
 * imagesize            - gets the size of an image
 * imagegetpixel        - gets the rgb values of a pixel
 * imagesetpixel        - sets the rgb values of a pixel
 * imagefree            - free memory from a previously loaded or created image.
 *
 * SYNTAX
 *
 * iImageNumber imageload sFilename
 *
 *              imageSave iImageNumber, sFilename
 *
 * iImageNumber imagecreate iWidth, iHeight
 *
 * iWidth, iHeight imagesize iImageNumber
 *
 * kred, kgreen, kblue imagegetpixel iImageNumber, kX, kY
 * ared, agreen, ablue imagegetpixel iImageNumber, aX, aY
 *
 * imagesetpixel iImageNumber, kX, kY, kred, kgreen, kblue
 * imagesetpixel iImageNumber, aX, aY, ared, agreen, ablue
 *
 *              imageFree iImageNumber
 *
 */

#define USE_LIBPNG

#ifdef USE_LIBPNG
#include <png.h>
#else
#include <Imlib.h>
#include <X11/Xlib.h>
#endif

/* #include <SDL/SDL.h> */
/* #include <SDL/SDL_image.h> */

#include "csdl.h"
#include "imageOpcodes.h"

/*
  __doOpenImage and __doSaveImage are the only two functions that deal
  with the actual image loading saving library.

  imageData stores the image content as an array of RGB bytes for each
  row (l->r t->b) so a pixel value in imageData is located at: (w*y+x)*3.
*/

/* #undef CS_KSMPS */
/* #define CS_KSMPS     (csound->GetKsmps(csound)) */

static Image * __doOpenImage(char * filename, CSOUND *csound)
{
#ifdef USE_LIBPNG
#define HS (8)
    FILE *fp;
    void *fd;
    unsigned char header[HS];
    png_structp png_ptr;
    png_infop info_ptr;
    /* png_infop end_ptr; */
    int32_t is_png;
    png_uint_32 width, height, rowbytes;
    int32_t bit_depth;
    int32_t color_type;
    unsigned char *image_data;
    png_bytepp row_pointers;
    uint32_t i;

    Image *img;

    fd = csound->FileOpen2(csound, &fp, CSFILE_STD, filename, "rb",
                           "SFDIR;SSDIR", CSFTYPE_IMAGE_PNG, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->InitError(csound,
                               Str("imageload: cannot open image %s.\n"), filename);
      return NULL;
    }

    if (UNLIKELY(HS!=fread(header, 1, HS, fp))) {
      csound->InitError(csound,
                        Str("imageload: file %s is not in PNG format.\n"),
                        filename);
      return NULL;
    }
    is_png = !png_sig_cmp(header, 0, HS);

    if (UNLIKELY(!is_png)) {
      csound->InitError(csound,
                        Str("imageload: file %s is not in PNG format.\n"),
                        filename);
      csound->FileClose(csound, fd);
      return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (UNLIKELY(!png_ptr)) {
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      csound->FileClose(csound, fd);
      return NULL;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (UNLIKELY(!info_ptr)) {
      png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      csound->FileClose(csound, fd);
      return NULL;
    }

    /* end_ptr = png_create_info_struct(png_ptr); */
    /* if (UNLIKELY(!end_ptr)) { */
    /*   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL); */
    /*   csound->InitError(csound, "%s", Str("imageload: out of memory.\n")); */
    /*   csound->FileClose(csound, fd); */
    /*   return NULL; */
    /* } */

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, HS);

    png_read_info(png_ptr, info_ptr);
    {

      png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                   &color_type, NULL, NULL, NULL);
    }
    if (color_type & PNG_COLOR_MASK_ALPHA)
      png_set_strip_alpha(png_ptr);
    if (bit_depth == 16)
      png_set_strip_16(png_ptr);
    if (bit_depth < 8)
      png_set_packing(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);
    rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    if (UNLIKELY((image_data =
                 (unsigned char *)csound->Malloc(csound,rowbytes*height))==NULL)) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      return NULL;
    }

    row_pointers = (png_bytepp) csound->Malloc(csound, height*sizeof(png_bytep));
    if (UNLIKELY(row_pointers == NULL)) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      csound->Free(csound,image_data);
      image_data = NULL;
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      return NULL;
    }

    for (i = 0; i < height; i++)
      row_pointers[i] = image_data + i*rowbytes;

    png_read_image(png_ptr, row_pointers);
    csound->Free(csound,row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    csound->FileClose(csound, fd);

    img = csound->Malloc(csound, sizeof(Image));
    if (UNLIKELY(!img)) {
      csound->Free(csound,image_data);
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      return NULL;
    }

    img->w = width;
    img->h = height;
    img->imageData = image_data;

    return img;

#else

    Display *disp;
    ImlibData *id;
    ImlibImage *im;
    Image *img;
    size_t datasize;

    disp=XOpenDisplay(NULL);
    id=Imlib_init(disp);
    im=Imlib_load_image(id, filename);

    img = csound->Malloc(csound, sizeof(Image));
    img->w = im->rgb_width;
    img->h = im->rgb_height;
    datasize = img->w*img->h*3 * sizeof(unsigned char);
    img->imageData = csound->Malloc(csound, datasize);
    memcpy(img->imageData, im->rgb_data, datasize);

    return img;
#endif

    /* SDL */
/*  Image *img;
    size_t datasize;
    SDL_Surface *srfc;
    int32_t x,y;
    int32_t bpp;
    int32_t indcount = 0;
    Uint32 *pixel;
    Uint8 r, g, b;

    srfc = IMG_Load(filename);
    if (srfc) {
        SDL_LockSurface(srfc);
        img = csound->Malloc(csound, sizeof(Image));
        img->w = srfc->w;
        img->h = srfc->h;
        bpp = srfc->format->BitsPerPixel;

        datasize = img->w*img->h*3 * sizeof(unsigned char);
        img->imageData = csound->Malloc(csound, datasize);

        for(y = 0; y < img->h; y++) {
            for(x = 0; x < img->w; x++) {
                if (bpp<=8) //need to test on other platforms
                    pixel = srfc->pixels + y * srfc->pitch + x * bpp;
                else
                    pixel = srfc->pixels + y * srfc->pitch + x * bpp / 8;
                SDL_GetRGB(*pixel,srfc->format, &r, &g, &b);
                img->imageData[indcount]= r;
                img->imageData[indcount+1]= g;
                img->imageData[indcount+2]= b;
                indcount += 3;
            }
        }
        SDL_UnlockSurface(srfc);
        SDL_FreeSurface ( srfc );
        return img;
  }

  return NULL;
*/

}


static int32_t __doSaveImage(Image *image, char *filename, CSOUND *csound)
{
#ifdef USE_LIBPNG

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytepp row_pointers;
    uint32_t rowbytes;
    int32_t i;

    FILE *fp;
    void *fd;

    fd = csound->FileOpen2(csound, &fp, CSFILE_STD, filename, "wb",
                           "", CSFTYPE_IMAGE_PNG, 0);
    if (UNLIKELY(fd == NULL)) {
      return
        csound->InitError(csound,
                          Str("imageload: cannot open image %s for writing.\n"),
                          filename);
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (UNLIKELY(!png_ptr)){
      csound->FileClose(csound, fd);
      return csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (UNLIKELY(!info_ptr)) {
      png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
      csound->FileClose(csound, fd);
      return csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, image->w, image->h,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    row_pointers = (png_bytepp)csound->Malloc(csound, image->h*sizeof(png_bytep));
    if (UNLIKELY(row_pointers == NULL)) {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
    }

    rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    for (i = 0; i < image->h; i++)
      row_pointers[i] = image->imageData + i*rowbytes;

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    csound->Free(csound,row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    csound->FileClose(csound, fd);

    return OK;
#else
    Display *disp;
    ImlibData *id;
    ImlibImage *im;

    disp=XOpenDisplay(NULL);
    id=Imlib_init(disp);

    im = Imlib_create_image_from_data(id, image->imageData,
                                      NULL, image->w, image->h);
    Imlib_save_image(id, im, filename, NULL);
    Imlib_kill_image(id, im);

    return OK;
#endif
}


static Image * createImage(CSOUND *csound, int32_t w, int32_t h)
{
    Image *img;
    size_t datasize;

    img = csound->Malloc(csound, sizeof(Image));
    img->w = w;
    img->h = h;

    datasize = img->w*img->h*3 * sizeof(unsigned char);
    img->imageData = csound->Malloc(csound, datasize);

    return img;
}


static Images * getImages(CSOUND *csound)
{
    Images *pimages;
    pimages = (Images*)csound->QueryGlobalVariable(csound,
                                                   "imageOpcodes.images");
    if (pimages==NULL) { /* first call */
      csound->CreateGlobalVariable(csound, "imageOpcodes.images",
                                   sizeof(Images));
      pimages = (Images*)csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
      pimages->images = (Image **) NULL;
      pimages->cnt = (size_t) 0;
    }
    return pimages;
}

static int32_t imagecreate (CSOUND *csound, IMGCREATE * p)
{
    Images *pimages;
    Image *img;

    pimages = getImages(csound);

    pimages->cnt++;
    pimages->images =
      (Image **) csound->ReAlloc(csound, pimages->images,
                                 sizeof(Image *) * pimages->cnt);

    img = createImage(csound, *p->kw, *p->kh);

    if (UNLIKELY(img==NULL)) {
      return csound->InitError(csound, "%s", Str("Cannot allocate memory.\n"));
    }
    else {
      pimages->images[pimages->cnt-1] = img;
      *(p->kn) = (MYFLT) pimages->cnt;
      return OK;
    }
}

static int32_t imageload (CSOUND *csound, IMGLOAD * p)
{
    char filename[256];
    Images *pimages;
    Image *img;

    pimages = getImages(csound);

    pimages->cnt++;
    pimages->images =
      (Image **) csound->ReAlloc(csound, pimages->images,
                                 sizeof(Image *) * pimages->cnt);

    strncpy(filename, (char*) (p->ifilnam->data), 255); filename[255]='\0';

    img = __doOpenImage(filename, csound);

    if (LIKELY(img)) {
      pimages->images[pimages->cnt-1] = img;
      *(p->kn) = (MYFLT) pimages->cnt;
      return OK;
    }
    else {
      return csound->InitError(csound,
                               Str("imageload: cannot open image %s.\n"),
                               filename);
    }
}


static int32_t imagesize (CSOUND *csound, IMGSIZE * p)
{
    Images *pimages;
    Image *img;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t)(*p->kn)-1];

    *(p->kw) = (MYFLT) img->w;
    *(p->kh) = (MYFLT) img->h;
    return OK;
}


static int32_t imagegetpixel (CSOUND *csound, IMGGETPIXEL * p)
{
    Images *pimages;
    Image *img;
    int32_t w, h, x, y;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t)(*p->kn)-1];

    w = img->w;
    h = img->h;

    x = *p->kx*w;
    y = *p->ky*h;

    if (x >= 0 && x < w && y >= 0 && y < h ) {
      int32_t pixel = (w*y+x)*3;
      *p->kr = img->imageData[pixel]/FL(255.0);
      *p->kg = img->imageData[pixel+1]/FL(255.0);
      *p->kb = img->imageData[pixel+2]/FL(255.0);
    }
    else {
      *p->kr = FL(0.0);
      *p->kg = FL(0.0);
      *p->kb = FL(0.0);
    }

    return OK;
}

static int32_t imagegetpixel_a (CSOUND *csound, IMGGETPIXEL * p)
{
    Images *pimages;
    Image *img;

    MYFLT *r = p->kr;
    MYFLT *g = p->kg;
    MYFLT *b = p->kb;

    MYFLT *tx = p->kx;
    MYFLT *ty = p->ky;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t w, h, x, y, pixel;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t)(*p->kn)-1];
    w = img->w;
    h = img->h;


    if (UNLIKELY(offset)) {
      memset(r, '\0', offset*sizeof(MYFLT));
      memset(g, '\0', offset*sizeof(MYFLT));
      memset(b, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
      memset(&g[nsmps], '\0', early*sizeof(MYFLT));
      memset(&b[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = 0; i < nsmps; i++) {

      x = tx[i]*w;
      y = ty[i]*h;

      if ( x >= 0 && x < w && y >= 0 && y < h ) {
        pixel = (w*y+x)*3;
        r[i] = img->imageData[pixel]/FL(255.0);
        g[i] = img->imageData[pixel+1]/FL(255.0);
        b[i] = img->imageData[pixel+2]/FL(255.0);
      }
      else {
        r[i] = FL(0.0);
        g[i] = FL(0.0);
        b[i] = FL(0.0);
      }
    }

    return OK;
}

static int32_t imagesetpixel_a (CSOUND *csound, IMGSETPIXEL * p)
{
    Images *pimages;
    Image *img;

    MYFLT *r = p->kr;
    MYFLT *g = p->kg;
    MYFLT *b = p->kb;

    MYFLT *tx = p->kx;
    MYFLT *ty = p->ky;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t h,w,x, y, pixel;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t)(*p->kn)-1];

    w = img->w;
    h = img->h;

    if (UNLIKELY(early)) nsmps -= early;
    for (i = offset; i < nsmps; i++) {

      x = tx[i]*w;
      y = ty[i]*h;

      if (x >= 0 && x < w && y >= 0 && y < h ) {
        pixel = (w*y+x)*3;
        img->imageData[pixel] = (unsigned char)(r[i]*255) % 256;
        img->imageData[pixel+1] = (unsigned char)(g[i]*255) % 256;
        img->imageData[pixel+2] = (unsigned char)(b[i]*255) % 256;
      }

    }

    return OK;
}

static int32_t imagesetpixel (CSOUND *csound, IMGSETPIXEL * p)
{
    Images *pimages;
    Image *img;
    int32_t w, h, x, y, pixel;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t)(*p->kn)-1];

    w = img->w;
    h = img->h;

    x = *p->kx*w;
    y = *p->ky*h;

    if (x >= 0 && x < w && y >= 0 && y < h ) {
      pixel = (w*y+x)*3;
      img->imageData[pixel] = (unsigned char)((*p->kr)*255) % 256;
      img->imageData[pixel+1] = (unsigned char)((*p->kg)*255) % 256;
      img->imageData[pixel+2] = (unsigned char)((*p->kb)*255) % 256;
    }
    return OK;
}

static int32_t imagesave (CSOUND *csound, IMGSAVE * p)
{
    Images *pimages;
    Image *img;
    char filename[256];

    strncpy(filename, (char*) (p->ifilnam->data), 254); filename[255]='\0';

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t)(*p->kn)-1];

    return __doSaveImage(img, filename, csound);
}

static int32_t imagefree (CSOUND *csound, IMGSAVE * p)
{
    Images *pimages;
    Image *img;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    img = pimages->images[(int32_t
                           )(*p->kn)-1];
    csound->Free(csound,img->imageData);
    csound->Free(csound,img);

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY image_localops[] = {
  { "imageload",  S(IMGLOAD),  0, 1, "i", "S",   (SUBR)imageload, NULL, NULL   },
  { "imagecreate",S(IMGCREATE),0, 1, "i", "ii",  (SUBR)imagecreate, NULL, NULL },
  { "imagesize",  S(IMGSIZE),  0, 1, "ii", "i",  (SUBR)imagesize, NULL, NULL   },
  { "imagegetpixel",  S(IMGGETPIXEL),  0, 3, "kkk", "ixx",
    (SUBR)imagegetpixel, (SUBR)imagegetpixel,                                  },
  { "imagegetpixel",  S(IMGGETPIXEL),  0, 3, "aaa", "ixx",
    (SUBR)imagegetpixel, (SUBR)imagegetpixel_a                           },
  { "imagesetpixel",  S(IMGSETPIXEL),  0, 3, "", "ikkkkk",
    (SUBR)imagesetpixel, (SUBR)imagesetpixel, (SUBR)imagesetpixel_a            },
  { "imagesetpixel",  S(IMGSETPIXEL),  0, 3, "", "iaaaaa",
    (SUBR)imagesetpixel, (SUBR)imagesetpixel_a                          },
  { "imagesave",  S(IMGSAVE),  0, 1, "", "iS",   (SUBR)imagesave, NULL, NULL   },
  { "imagefree",  S(IMGFREE),  0, 1, "", "i",    (SUBR)imagefree, NULL, NULL   },
};


LINKAGE_BUILTIN(image_localops)
