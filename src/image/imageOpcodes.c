
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

#include <stdio.h>
#include <string.h>

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

static Image * __doOpenImage(const char * filename, CSOUND *csound)
{
#ifdef USE_LIBPNG
#define HS (8)
    FILE *fp;
    char *fullname;
    unsigned char header[HS];
    png_structp png_ptr;
    png_infop info_ptr;
    /* png_infop end_ptr; */
    int32_t is_png;
    png_uint_32 width, height, rowbytes;
    int32_t bit_depth;
    int32_t color_type;
    unsigned char * volatile image_data = NULL;
    size_t datasize;
    png_bytepp volatile row_pointers = NULL;
    uint32_t i;

    Image *img;

    fullname = csound->FindInputFile(csound, filename, "SFDIR;SSDIR");
    fp = (fullname != NULL) ? fopen(fullname, "rb") : NULL;
    if (UNLIKELY(fp == NULL)) {
      if (fullname != NULL)
        csound->Free(csound, fullname);
      csound->InitError(csound,
                               Str("imageload: cannot open image %s.\n"), filename);
      return NULL;
    }
    csound->NotifyFileOpened(csound, fullname, CSFTYPE_IMAGE_PNG, 0, 0);
    csound->Free(csound, fullname);

    if (UNLIKELY(HS!=fread(header, 1, HS, fp))) {
      csound->InitError(csound,
                        Str("imageload: file %s is not in PNG format.\n"),
                        filename);
      fclose(fp);
      return NULL;
    }
    is_png = !png_sig_cmp(header, 0, HS);

    if (UNLIKELY(!is_png)) {
      csound->InitError(csound,
                        Str("imageload: file %s is not in PNG format.\n"),
                        filename);
      fclose(fp);
      return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (UNLIKELY(!png_ptr)) {
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      fclose(fp);
      return NULL;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (UNLIKELY(!info_ptr)) {
      png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      fclose(fp);
      return NULL;
    }

    /* end_ptr = png_create_info_struct(png_ptr); */
    /* if (UNLIKELY(!end_ptr)) { */
    /*   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL); */
    /*   csound->InitError(csound, "%s", Str("imageload: out of memory.\n")); */
    /*   fclose(fp); */
    /*   return NULL; */
    /* } */

    if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      if (image_data != NULL)
        csound->Free(csound, image_data);
      if (row_pointers != NULL)
        csound->Free(csound, row_pointers);
      fclose(fp);
      csound->InitError(csound, Str("imageload: error reading image %s.\n"),
                        filename);
      return NULL;
    }

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
    if (bit_depth < 8) {
      if (color_type == PNG_COLOR_TYPE_GRAY ||
          color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
      else
        png_set_packing(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);
    rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    if (UNLIKELY((size_t) rowbytes > ((size_t) -1) / (size_t) height)) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      csound->InitError(csound, "%s", Str("imageload: image too large.\n"));
      fclose(fp);
      return NULL;
    }
    datasize = (size_t) rowbytes * (size_t) height;
    if (UNLIKELY((image_data =
                 (unsigned char *)csound->Malloc(csound,datasize))==NULL)) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      fclose(fp);
      return NULL;
    }

    row_pointers = (png_bytepp) csound->Malloc(csound, height*sizeof(png_bytep));
    if (UNLIKELY(row_pointers == NULL)) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      csound->Free(csound,image_data);
      image_data = NULL;
      csound->InitError(csound, "%s", Str("imageload: out of memory.\n"));
      fclose(fp);
      return NULL;
    }

    for (i = 0; i < height; i++)
      row_pointers[i] = image_data + (size_t) i * (size_t) rowbytes;

    png_read_image(png_ptr, row_pointers);
    csound->Free(csound,row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

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


static int32_t __doSaveImage(Image *image, const char *filename, CSOUND *csound)
{
#ifdef USE_LIBPNG

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytepp volatile row_pointers = NULL;
    uint32_t rowbytes;
    int32_t i;

    FILE *fp;
    char *fullname;

    fullname = csound->FindOutputFile(csound, filename, "SFDIR;SSDIR");
    fp = (fullname != NULL) ? fopen(fullname, "wb") : NULL;
    if (UNLIKELY(fp == NULL)) {
      if (fullname != NULL)
        csound->Free(csound, fullname);
      return
        csound->InitError(csound,
                          Str("imagesave: cannot open image %s for writing.\n"),
                          filename);
    }
    csound->NotifyFileOpened(csound, fullname, CSFTYPE_IMAGE_PNG, 1, 0);
    csound->Free(csound, fullname);

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (UNLIKELY(!png_ptr)){
      fclose(fp);
      return csound->InitError(csound, "%s", Str("imagesave: out of memory.\n"));
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (UNLIKELY(!info_ptr)) {
      png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
      fclose(fp);
      return csound->InitError(csound, "%s", Str("imagesave: out of memory.\n"));
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      if (row_pointers != NULL)
        csound->Free(csound, row_pointers);
      fclose(fp);
      return csound->InitError(csound,
                               Str("imagesave: error writing image %s.\n"),
                               filename);
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, image->w, image->h,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    row_pointers = (png_bytepp)csound->Malloc(csound, image->h*sizeof(png_bytep));
    if (UNLIKELY(row_pointers == NULL)) {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      fclose(fp);
      return csound->InitError(csound, "%s", Str("imagesave: out of memory.\n"));
    }

    rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    for (i = 0; i < image->h; i++)
      row_pointers[i] = image->imageData + i*rowbytes;

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    csound->Free(csound,row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

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

    if (UNLIKELY(w < 1 || h < 1))
      return (Image *) NULL;
    if (UNLIKELY((size_t) w > ((size_t) -1) / 3 / (size_t) h))
      return (Image *) NULL;

    img = csound->Malloc(csound, sizeof(Image));
    if (UNLIKELY(img == NULL))
      return (Image *) NULL;

    img->w = w;
    img->h = h;

    datasize = (size_t) w * (size_t) h * 3;
    img->imageData = csound->Calloc(csound, datasize);
    if (UNLIKELY(img->imageData == NULL)) {
      csound->Free(csound, img);
      return (Image *) NULL;
    }

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

static Image * getImage(CSOUND *csound, cs_float kn)
{
    Images *pimages;
    int32_t n;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    if (UNLIKELY(pimages == NULL))
      return (Image *) NULL;
    n = (int32_t) kn;
    if (UNLIKELY(n < 1 || (size_t) n > pimages->cnt))
      return (Image *) NULL;
    return pimages->images[n-1];
}

static int32_t imagecreate (CSOUND *csound, IMGCREATE * p)
{
    Images *pimages;
    Image *img;
    Image **newImages;

    pimages = getImages(csound);

    img = createImage(csound, *p->kw, *p->kh);
    if (UNLIKELY(img == NULL)) {
      return csound->InitError(csound, "%s", Str("Cannot allocate memory.\n"));
    }

    newImages = (Image **) csound->ReAlloc(csound, pimages->images,
                                          sizeof(Image *) * (pimages->cnt + 1));
    if (UNLIKELY(newImages == NULL)) {
      csound->Free(csound, img->imageData);
      csound->Free(csound, img);
      return csound->InitError(csound, "%s", Str("Cannot allocate memory.\n"));
    }

    pimages->images = newImages;
    pimages->images[pimages->cnt] = img;
    pimages->cnt++;
    *(p->kn) = (cs_float) pimages->cnt;
    return OK;
}

static int32_t imageload (CSOUND *csound, IMGLOAD * p)
{
    Images *pimages;
    Image *img;
    Image **newImages;

    pimages = getImages(csound);

    img = __doOpenImage((const char*) (p->ifilnam->data), csound);
    if (UNLIKELY(img == NULL))
      return NOTOK;

    newImages = (Image **) csound->ReAlloc(csound, pimages->images,
                                          sizeof(Image *) * (pimages->cnt + 1));
    if (UNLIKELY(newImages == NULL)) {
      csound->Free(csound, img->imageData);
      csound->Free(csound, img);
      return csound->InitError(csound, "%s", Str("Cannot allocate memory.\n"));
    }

    pimages->images = newImages;
    pimages->images[pimages->cnt] = img;
    pimages->cnt++;
    *(p->kn) = (cs_float) pimages->cnt;
    return OK;
}


static int32_t imagesize (CSOUND *csound, IMGSIZE * p)
{
    Image *img;

    img = getImage(csound, *p->kn);
    if (UNLIKELY(img == NULL))
      return csound->InitError(csound, "%s",
                               Str("imagesize: invalid image handle.\n"));

    *(p->kw) = (cs_float) img->w;
    *(p->kh) = (cs_float) img->h;
    return OK;
}


static int32_t imagegetpixel (CSOUND *csound, IMGGETPIXEL * p)
{
    Image *img;
    int32_t w, h, x, y;

    img = getImage(csound, *p->kn);
    if (UNLIKELY(img == NULL))
      return csound->InitError(csound, "%s",
                               Str("imagegetpixel: invalid image handle.\n"));

    w = img->w;
    h = img->h;

    x = *p->kx*w;
    y = *p->ky*h;

    if (x >= 0 && x < w && y >= 0 && y < h ) {
      size_t pixel = ((size_t) w * (size_t) y + (size_t) x) * 3;
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
    Image *img;

    cs_float *r = p->kr;
    cs_float *g = p->kg;
    cs_float *b = p->kb;

    cs_float *tx = p->kx;
    cs_float *ty = p->ky;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t w, h, x, y;
    size_t pixel;

    img = getImage(csound, *p->kn);
    if (UNLIKELY(img == NULL)) {
      memset(r, '\0', nsmps*sizeof(cs_float));
      memset(g, '\0', nsmps*sizeof(cs_float));
      memset(b, '\0', nsmps*sizeof(cs_float));
      return OK;
    }
    w = img->w;
    h = img->h;


    if (UNLIKELY(offset)) {
      memset(r, '\0', offset*sizeof(cs_float));
      memset(g, '\0', offset*sizeof(cs_float));
      memset(b, '\0', offset*sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(cs_float));
      memset(&g[nsmps], '\0', early*sizeof(cs_float));
      memset(&b[nsmps], '\0', early*sizeof(cs_float));
    }
    for (i = offset; i < nsmps; i++) {

      x = tx[i]*w;
      y = ty[i]*h;

      if ( x >= 0 && x < w && y >= 0 && y < h ) {
        pixel = ((size_t) w * (size_t) y + (size_t) x) * 3;
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

static int32_t imagesetpixel_a_init (CSOUND *csound, IMGSETPIXEL * p)
{
    if (UNLIKELY(getImage(csound, *p->kn) == NULL))
      return csound->InitError(csound, "%s",
                               Str("imagesetpixel: invalid image handle.\n"));
    return OK;
}

static int32_t imagesetpixel_a (CSOUND *csound, IMGSETPIXEL * p)
{
    Image *img;

    cs_float *r = p->kr;
    cs_float *g = p->kg;
    cs_float *b = p->kb;

    cs_float *tx = p->kx;
    cs_float *ty = p->ky;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t h,w,x, y;
    size_t pixel;

    img = getImage(csound, *p->kn);
    if (UNLIKELY(img == NULL))
      return OK;

    w = img->w;
    h = img->h;

    if (UNLIKELY(early)) nsmps -= early;
    for (i = offset; i < nsmps; i++) {

      x = tx[i]*w;
      y = ty[i]*h;

      if (x >= 0 && x < w && y >= 0 && y < h ) {
        pixel = ((size_t) w * (size_t) y + (size_t) x) * 3;
        img->imageData[pixel] = (unsigned char)(r[i]*255) % 256;
        img->imageData[pixel+1] = (unsigned char)(g[i]*255) % 256;
        img->imageData[pixel+2] = (unsigned char)(b[i]*255) % 256;
      }

    }

    return OK;
}

static int32_t imagesetpixel (CSOUND *csound, IMGSETPIXEL * p)
{
    Image *img;
    int32_t w, h, x, y;
    size_t pixel;

    img = getImage(csound, *p->kn);
    if (UNLIKELY(img == NULL))
      return csound->InitError(csound, "%s",
                               Str("imagesetpixel: invalid image handle.\n"));

    w = img->w;
    h = img->h;

    x = *p->kx*w;
    y = *p->ky*h;

    if (x >= 0 && x < w && y >= 0 && y < h ) {
      pixel = ((size_t) w * (size_t) y + (size_t) x) * 3;
      img->imageData[pixel] = (unsigned char)((*p->kr)*255) % 256;
      img->imageData[pixel+1] = (unsigned char)((*p->kg)*255) % 256;
      img->imageData[pixel+2] = (unsigned char)((*p->kb)*255) % 256;
    }
    return OK;
}

static int32_t imagesave (CSOUND *csound, IMGSAVE * p)
{
    Image *img;

    img = getImage(csound, *p->kn);
    if (UNLIKELY(img == NULL))
      return csound->InitError(csound, "%s",
                               Str("imagesave: invalid image handle.\n"));

    return __doSaveImage(img, (const char*) (p->ifilnam->data), csound);
}

static int32_t imagefree (CSOUND *csound, IMGFREE * p)
{
    Images *pimages;
    Image *img;
    int32_t n;

    pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                     "imageOpcodes.images");
    n = (int32_t) (*p->kn);
    if (UNLIKELY(pimages == NULL || n < 1 || (size_t) n > pimages->cnt))
      return csound->InitError(csound, "%s",
                               Str("imagefree: invalid image handle.\n"));

    img = pimages->images[n-1];
    if (img != NULL) {
      csound->Free(csound,img->imageData);
      csound->Free(csound,img);
      pimages->images[n-1] = (Image *) NULL;
    }

    return OK;
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound)
{
    Images *pimages = (Images *) csound->QueryGlobalVariable(csound,
                                                             "imageOpcodes.images");
    if (pimages != NULL && pimages->images != NULL) {
      size_t i;
      for (i = 0; i < pimages->cnt; ++i) {
        Image *img = pimages->images[i];
        if (img != NULL) {
          csound->Free(csound, img->imageData);
          csound->Free(csound, img);
        }
      }
      csound->Free(csound, pimages->images);
      pimages->images = (Image **) NULL;
      pimages->cnt = (size_t) 0;
    }
    return OK;
}

static OENTRY localops[] = {
  { "imageload",      sizeof(IMGLOAD),  0, "i", "S",
    (SUBR)imageload, NULL, NULL, NULL, 0 },
  { "imagecreate",    sizeof(IMGCREATE),0, "i", "ii",
    (SUBR)imagecreate, NULL, NULL, NULL, 0 },
  { "imagesize",      sizeof(IMGSIZE),  0, "ii", "i",
    (SUBR)imagesize, NULL, NULL, NULL, 0 },
  { "imagegetpixel",  sizeof(IMGGETPIXEL), 0, "kkk", "ikk",
    (SUBR)imagegetpixel, (SUBR)imagegetpixel, NULL, NULL, 0 },
  { "imagegetpixel",  sizeof(IMGGETPIXEL), 0, "aaa", "iaa",
    (SUBR)imagegetpixel, (SUBR)imagegetpixel_a, NULL, NULL, 0 },
  { "imagesetpixel",  sizeof(IMGSETPIXEL), 0, "", "ikkkkk",
    (SUBR)imagesetpixel, (SUBR)imagesetpixel, NULL, NULL, 0 },
  { "imagesetpixel",  sizeof(IMGSETPIXEL), 0, "", "iaaaaa",
    (SUBR)imagesetpixel_a_init, (SUBR)imagesetpixel_a, NULL, NULL, 0 },
  { "imagesave",      sizeof(IMGSAVE),  0, "", "iS",
    (SUBR)imagesave, NULL, NULL, NULL, 0 },
  { "imagefree",      sizeof(IMGFREE),  0, "", "i",
    (SUBR)imagefree, NULL, NULL, NULL, 0 },
};

LINKAGE
