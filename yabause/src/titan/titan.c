/*  Copyright 2012 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "titan.h"
#include "../vidshared.h"

#include <stdlib.h>

/* private */
typedef u32 (*TitanBlendFunc)(u32 top, u32 bottom);
typedef int FASTCALL (*TitanTransFunc)(u32 pixel);

struct PixelData
{
   u32 pixel;
   u8 priority;
   u8 linescreen;
   u8 shadow_type;
   u8 shadow_enabled;
};

static struct TitanContext {
   int inited;
   struct PixelData * vdp2framebuffer[6];
   u32 * linescreen[4];
   int vdp2width;
   int vdp2height;
   TitanBlendFunc blend;
   TitanTransFunc trans;
   struct PixelData * backscreen;
} tt_context = {
   0,
   { NULL, NULL, NULL, NULL, NULL, NULL },
   { NULL, NULL, NULL, NULL },
   320,
   224,
   NULL,NULL,NULL
};

#if defined WORDS_BIGENDIAN
#ifdef USE_RGB_555
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel >> 27) & 0x1F) | ((pixel >> 14) & 0x7C0) | (pixel >> 1) & 0xF8); }
#elif USE_RGB_565
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel >> 27) & 0x1F) | ((pixel >> 13) & 0x7E0) | (pixel & 0xF8)); }
#else
static INLINE u32 TitanFixAlpha(u32 pixel) { return ((((pixel & 0x3F) << 2) + 0x03) | (pixel & 0xFFFFFF00)); }
#endif

static INLINE u8 TitanGetAlpha(u32 pixel) { return pixel & 0x3F; }
static INLINE u8 TitanGetRed(u32 pixel) { return (pixel >> 8) & 0xFF; }
static INLINE u8 TitanGetGreen(u32 pixel) { return (pixel >> 16) & 0xFF; }
static INLINE u8 TitanGetBlue(u32 pixel) { return (pixel >> 24) & 0xFF; }
static INLINE u32 TitanCreatePixel(u8 alpha, u8 red, u8 green, u8 blue) { return alpha | (red << 8) | (green << 16) | (blue << 24); }
#else
#ifdef USE_RGB_555
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel << 7) & 0x7C00) | ((pixel >> 6) & 0x3C0) | ((pixel >> 19) & 0x1F)); }
#elif USE_RGB_565
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel << 8) & 0xF800) | ((pixel >> 5) & 0x7C0) | ((pixel >> 19) & 0x1F)); }
#else
static INLINE u32 TitanFixAlpha(u32 pixel) { return ((((pixel & 0x3F000000) << 2) + 0x03000000) | (pixel & 0x00FFFFFF)); }
#endif

static INLINE u8 TitanGetAlpha(u32 pixel) { return (pixel >> 24) & 0x3F; }
static INLINE u8 TitanGetRed(u32 pixel) { return (pixel >> 16) & 0xFF; }
static INLINE u8 TitanGetGreen(u32 pixel) { return (pixel >> 8) & 0xFF; }
static INLINE u8 TitanGetBlue(u32 pixel) { return pixel & 0xFF; }
static INLINE u32 TitanCreatePixel(u8 alpha, u8 red, u8 green, u8 blue) { return (alpha << 24) | (red << 16) | (green << 8) | blue; }
#endif

static u32 TitanBlendPixelsTop(u32 top, u32 bottom)
{
   u8 alpha, ralpha, tr, tg, tb, br, bg, bb;

   alpha = (TitanGetAlpha(top) << 2) + 3;
   ralpha = 0xFF - alpha;

   tr = (TitanGetRed(top) * alpha) / 0xFF;
   tg = (TitanGetGreen(top) * alpha) / 0xFF;
   tb = (TitanGetBlue(top) * alpha) / 0xFF;

   br = (TitanGetRed(bottom) * ralpha) / 0xFF;
   bg = (TitanGetGreen(bottom) * ralpha) / 0xFF;
   bb = (TitanGetBlue(bottom) * ralpha) / 0xFF;

   return TitanCreatePixel(0x3F, tr + br, tg + bg, tb + bb);
}

static u32 TitanBlendPixelsBottom(u32 top, u32 bottom)
{
   u8 alpha, ralpha, tr, tg, tb, br, bg, bb;

   if ((top & 0x80000000) == 0) return top;

   alpha = (TitanGetAlpha(bottom) << 2) + 3;
   ralpha = 0xFF - alpha;

   tr = (TitanGetRed(top) * alpha) / 0xFF;
   tg = (TitanGetGreen(top) * alpha) / 0xFF;
   tb = (TitanGetBlue(top) * alpha) / 0xFF;

   br = (TitanGetRed(bottom) * ralpha) / 0xFF;
   bg = (TitanGetGreen(bottom) * ralpha) / 0xFF;
   bb = (TitanGetBlue(bottom) * ralpha) / 0xFF;

   return TitanCreatePixel(TitanGetAlpha(top), tr + br, tg + bg, tb + bb);
}

static u32 TitanBlendPixelsAdd(u32 top, u32 bottom)
{
   u32 r, g, b;

   r = TitanGetRed(top) + TitanGetRed(bottom);
   if (r > 0xFF) r = 0xFF;

   g = TitanGetGreen(top) + TitanGetGreen(bottom);
   if (g > 0xFF) g = 0xFF;

   b = TitanGetBlue(top) + TitanGetBlue(bottom);
   if (b > 0xFF) b = 0xFF;

   return TitanCreatePixel(0x3F, r, g, b);
}

static INLINE int FASTCALL TitanTransAlpha(u32 pixel)
{
   return TitanGetAlpha(pixel) < 0x3F;
}

static INLINE int FASTCALL TitanTransBit(u32 pixel)
{
   return pixel & 0x80000000;
}

static u32 TitanDigPixel(int pos, int y)
{
   struct PixelData pixel_stack[2] = { 0 };

   int pixel_stack_pos = 0;

   int priority;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (tt_context.vdp2framebuffer[which_layer][pos].priority == priority)
         {
            pixel_stack[pixel_stack_pos] = tt_context.vdp2framebuffer[which_layer][pos];
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               goto finished;//backscreen is unnecessary in this case
         }
      }
   }

   pixel_stack[pixel_stack_pos] = tt_context.backscreen[pos];

finished:

   if (pixel_stack[0].linescreen)
   {
      pixel_stack[0].pixel = tt_context.blend(pixel_stack[0].pixel, tt_context.linescreen[pixel_stack[0].linescreen][y]);
   }

   if ((pixel_stack[0].shadow_type == TITAN_MSB_SHADOW) && ((pixel_stack[0].pixel & 0xFFFFFF) == 0))
   {
      //transparent sprite shadow
      if (pixel_stack[1].shadow_enabled)
      {
         pixel_stack[0].pixel = TitanBlendPixelsTop(0x20000000, pixel_stack[1].pixel);
      }
      else
      {
         pixel_stack[0].pixel = pixel_stack[1].pixel;
      }
   }
   else if (pixel_stack[0].shadow_type == TITAN_MSB_SHADOW && ((pixel_stack[0].pixel & 0xFFFFFF) != 0))
   {
      if (tt_context.trans(pixel_stack[0].pixel))
      {
         u32 bottom = pixel_stack[1].pixel;
         pixel_stack[0].pixel = tt_context.blend(pixel_stack[0].pixel, bottom);
      }

      //sprite self-shadowing
      pixel_stack[0].pixel = TitanBlendPixelsTop(0x20000000, pixel_stack[0].pixel);
   }
   else if (pixel_stack[0].shadow_type == TITAN_NORMAL_SHADOW)
   {
      if (pixel_stack[1].shadow_enabled)
      {
         pixel_stack[0].pixel = TitanBlendPixelsTop(0x20000000, pixel_stack[1].pixel);
      }
      else
      {
         pixel_stack[0].pixel = pixel_stack[1].pixel;
      }
   }
   else
   {
      if (tt_context.trans(pixel_stack[0].pixel))
      {
         u32 bottom = pixel_stack[1].pixel;
         pixel_stack[0].pixel = tt_context.blend(pixel_stack[0].pixel, bottom);
      }
   }

   return pixel_stack[0].pixel;
}

/* public */
int TitanInit()
{
   int i;

   if (! tt_context.inited)
   {
      for(i = 0;i < 6;i++)
      {
         if ((tt_context.vdp2framebuffer[i] = (struct PixelData *)calloc(sizeof(struct PixelData), 704 * 512)) == NULL)
            return -1;
      }

      /* linescreen 0 is not initialized as it's not used... */
      for(i = 1;i < 4;i++)
      {
         if ((tt_context.linescreen[i] = (u32 *)calloc(sizeof(u32), 512)) == NULL)
            return -1;
      }

      if ((tt_context.backscreen = (struct PixelData  *)calloc(sizeof(struct PixelData), 704 * 512)) == NULL)
         return -1;

      tt_context.inited = 1;
   }

   for(i = 0;i < 6;i++)
      memset(tt_context.vdp2framebuffer[i], 0, sizeof(u32) * 704 * 512);

   for(i = 1;i < 4;i++)
      memset(tt_context.linescreen[i], 0, sizeof(u32) * 512);

   return 0;
}

void TitanErase()
{
   int i = 0;

   for (i = 0; i < 6; i++)
      memset(tt_context.vdp2framebuffer[i], 0, sizeof(struct PixelData) * tt_context.vdp2width * tt_context.vdp2height);
}

int TitanDeInit()
{
   int i;

   for(i = 0;i < 6;i++)
      free(tt_context.vdp2framebuffer[i]);

   for(i = 1;i < 4;i++)
      free(tt_context.linescreen[i]);

   return 0;
}

void TitanSetResolution(int width, int height)
{
   tt_context.vdp2width = width;
   tt_context.vdp2height = height;
}

void TitanGetResolution(int * width, int * height)
{
   *width = tt_context.vdp2width;
   *height = tt_context.vdp2height;
}

void TitanSetBlendingMode(int blend_mode)
{
   if (blend_mode == TITAN_BLEND_BOTTOM)
   {
      tt_context.blend = TitanBlendPixelsBottom;
      tt_context.trans = TitanTransBit;
   }
   else if (blend_mode == TITAN_BLEND_ADD)
   {
      tt_context.blend = TitanBlendPixelsAdd;
      tt_context.trans = TitanTransBit;
   }
   else
   {
      tt_context.blend = TitanBlendPixelsTop;
      tt_context.trans = TitanTransAlpha;
   }
}

void TitanPutBackHLine(s32 y, u32 color)
{
   struct PixelData* buffer = &tt_context.backscreen[(y * tt_context.vdp2width)];
   int i;

   for (i = 0; i < tt_context.vdp2width; i++)
      buffer[i].pixel = color;
}

void TitanPutLineHLine(int linescreen, s32 y, u32 color)
{
   if (linescreen == 0) return;

   {
      u32 * buffer = tt_context.linescreen[linescreen] + y;
      *buffer = color;
   }
}

void TitanPutPixel(int priority, s32 x, s32 y, u32 color, int linescreen, vdp2draw_struct* info)
{
   if (priority == 0) return;

   {
      int pos = (y * tt_context.vdp2width) + x;
      tt_context.vdp2framebuffer[info->titan_which_layer][pos].pixel = color;
      tt_context.vdp2framebuffer[info->titan_which_layer][pos].priority = priority;
      tt_context.vdp2framebuffer[info->titan_which_layer][pos].linescreen = linescreen;
      tt_context.vdp2framebuffer[info->titan_which_layer][pos].shadow_enabled = info->titan_shadow_enabled;
      tt_context.vdp2framebuffer[info->titan_which_layer][pos].shadow_type = info->titan_shadow_type;
   }
}

void TitanPutHLine(int priority, s32 x, s32 y, s32 width, u32 color)
{
   if (priority == 0) return;

   {
      struct PixelData * buffer = &tt_context.vdp2framebuffer[priority][ (y * tt_context.vdp2width) + x];
      int i;

      for (i = 0; i < width; i++)
         buffer[i].pixel = color;
   }
}

int do_main();

struct PixelData vdp2framebuffer[6 * 704 * 512];
//struct PixelData vdp2framebuffer_out[6 * 704 * 512] = { 0 };
struct PixelData pixel_stack_out[2 * 704 * 512] = { 0 };

u32 flat_dig(int pos)
{
   struct PixelData pixel_stack[2] = { 0 };

   int pixel_stack_pos = 0;

   int priority;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         int flat_pos = pos + which_layer * tt_context.vdp2width * tt_context.vdp2height;
         if (vdp2framebuffer[flat_pos].priority == priority)
         {
            pixel_stack[pixel_stack_pos] = vdp2framebuffer[flat_pos];
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               goto finished;//backscreen is unnecessary in this case
         }
      }
   }
finished:

   return pixel_stack[0].pixel;
//   pixel_stack[pixel_stack_pos] = tt_context.backscreen[pos];
}

#define WANT_OPENCL

void TitanRender(pixel_t * dispbuffer)
{
   u32 dot;
   int x, y;

   if (!tt_context.inited || (!tt_context.trans))
   {
      return;
   }

#ifdef WANT_OPENCL

   int layer;
   for (layer = 0; layer < 6; layer++)
   {
      for (y = 0; y < tt_context.vdp2height; y++)
      {
         for (x = 0; x < tt_context.vdp2width; x++)
         {
            int pos = (y * tt_context.vdp2width) + x;
            int flat_pos = x + (y * tt_context.vdp2width) + (layer * tt_context.vdp2width * tt_context.vdp2height);
            vdp2framebuffer[flat_pos] = tt_context.vdp2framebuffer[layer][pos];
         }
      }
   }

   //memset(pixel_stack_out, 0, sizeof(pixel_stack_out));

   opencl_exec(dispbuffer);

   //for (y = 0; y < tt_context.vdp2height; y++)
   //{
   //   for (x = 0; x < tt_context.vdp2width; x++)
   //   {
   //      int i = (y * tt_context.vdp2width) + x;

   //      dispbuffer[i] = 0;

   //      dot = pixel_stack_out[i].pixel;//flat_dig(i);

   //      if (dot)
   //      {
   //         dispbuffer[i] = TitanFixAlpha(dot);
   //      }
   //   }
   //}

   return;

#else
   
#ifdef WANT_VIDSOFT_RENDER_THREADING
#pragma omp parallel for private(x,y,dot)
#endif
   for (y = 0; y < tt_context.vdp2height; y++)
   {
      for (x = 0; x < tt_context.vdp2width; x++)
      {
         int i = (y * tt_context.vdp2width) + x;

         dispbuffer[i] = 0;

         dot = TitanDigPixel(i, y);

         if (dot)
         {
            dispbuffer[i] = TitanFixAlpha(dot);
         }
      }
   }

#endif
}

#ifdef WORDS_BIGENDIAN
void TitanWriteColor(pixel_t * dispbuffer, s32 bufwidth, s32 x, s32 y, u32 color)
{
   int pos = (y * bufwidth) + x;
   pixel_t * buffer = dispbuffer + pos;
   *buffer = ((color >> 24) & 0xFF) | ((color >> 8) & 0xFF00) | ((color & 0xFF00) << 8) | ((color & 0xFF) << 24);
}
#else
void TitanWriteColor(pixel_t * dispbuffer, s32 bufwidth, s32 x, s32 y, u32 color)
{
   int pos = (y * bufwidth) + x;
   pixel_t * buffer = dispbuffer + pos;
   *buffer = color;
}
#endif


#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

struct
{
   cl_device_id device_id;
   cl_context context;
   cl_command_queue command_queue;
   cl_mem memobj;
   cl_mem framebuffer;
   cl_mem pix_stack;
   cl_mem output;
   cl_program program;
   cl_kernel kernel;
   cl_platform_id platform_id;
   cl_uint ret_num_devices;
   cl_uint ret_num_platforms;
   cl_int ret;
   char *source_str;
}opencl_cxt;

void init_opencl()
{
   FILE *fp;
   char fileName[] = "C:/yabause-cl/yabause/yabause/src/titan/hello.cl";
   
   size_t source_size;

   /* Load the source code containing the kernel*/
   fp = fopen(fileName, "r");
   if (!fp) {
      fprintf(stderr, "Failed to load kernel.\n");
      exit(1);
   }
   opencl_cxt.source_str = (char*)malloc(MAX_SOURCE_SIZE);
   source_size = fread(opencl_cxt.source_str, 1, MAX_SOURCE_SIZE, fp);
   fclose(fp);

   /* Get Platform and Device Info */
   opencl_cxt.ret = clGetPlatformIDs(1, &opencl_cxt.platform_id, &opencl_cxt.ret_num_platforms);
   opencl_cxt.ret = clGetDeviceIDs(opencl_cxt.platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &opencl_cxt.device_id, &opencl_cxt.ret_num_devices);

   /* Create OpenCL context */
   opencl_cxt.context = clCreateContext(NULL, 1, &opencl_cxt.device_id, NULL, NULL, &opencl_cxt.ret);

   /* Create Command Queue */
   opencl_cxt.command_queue = clCreateCommandQueue(opencl_cxt.context, opencl_cxt.device_id, 0, &opencl_cxt.ret);

   /* Create Memory Buffer */
   opencl_cxt.memobj = clCreateBuffer(opencl_cxt.context, CL_MEM_READ_WRITE, MEM_SIZE * sizeof(char), NULL, &opencl_cxt.ret);

   opencl_cxt.framebuffer = clCreateBuffer(opencl_cxt.context, CL_MEM_READ_WRITE, sizeof(vdp2framebuffer), NULL, &opencl_cxt.ret);

   opencl_cxt.pix_stack = clCreateBuffer(opencl_cxt.context, CL_MEM_READ_WRITE, sizeof(pixel_stack_out), NULL, &opencl_cxt.ret);

   opencl_cxt.output = clCreateBuffer(opencl_cxt.context, CL_MEM_READ_WRITE, sizeof(unsigned int) * 704 * 512, NULL, &opencl_cxt.ret);

   /* Create Kernel Program from the source */
   opencl_cxt.program = clCreateProgramWithSource(opencl_cxt.context, 1, (const char **)&opencl_cxt.source_str,
      (const size_t *)&source_size, &opencl_cxt.ret);

   /* Build Kernel Program */
   opencl_cxt.ret = clBuildProgram(opencl_cxt.program, 1, &opencl_cxt.device_id, NULL, NULL, NULL);

   size_t log_size;
   clGetProgramBuildInfo(opencl_cxt.program, opencl_cxt.device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

   // Allocate memory for the log
   char *log = (char *)malloc(log_size);

   // Get the log
   clGetProgramBuildInfo(opencl_cxt.program, opencl_cxt.device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

   /* Create OpenCL Kernel */
   opencl_cxt.kernel = clCreateKernel(opencl_cxt.program, "hello", &opencl_cxt.ret);
}

int inited = 0;

int opencl_exec(pixel_t * dispbuffer)
{


   if (!inited)
   {
      init_opencl();
      inited = 1;
   }

   char string[MEM_SIZE];

   int output[256] = { 0 };

#if 1

   opencl_cxt.ret = clEnqueueWriteBuffer(opencl_cxt.command_queue, opencl_cxt.framebuffer, CL_TRUE, 0, sizeof(vdp2framebuffer), vdp2framebuffer, 0, NULL, NULL);
   
   int zero = 0;

 //  opencl_cxt.ret = clEnqueueFillBuffer(opencl_cxt.command_queue, opencl_cxt.pix_stack, &zero, sizeof(zero), NULL, sizeof(opencl_cxt.pix_stack), 0, NULL, NULL);

   opencl_cxt.ret = clSetKernelArg(opencl_cxt.kernel, 0, sizeof(cl_mem), (void *)&opencl_cxt.framebuffer);
   opencl_cxt.ret = clSetKernelArg(opencl_cxt.kernel, 1, sizeof(cl_mem), (void *)&opencl_cxt.pix_stack);
   opencl_cxt.ret = clSetKernelArg(opencl_cxt.kernel, 2, sizeof(cl_mem), (void *)&opencl_cxt.output);
   opencl_cxt.ret = clSetKernelArg(opencl_cxt.kernel, 3, sizeof(int), (void *)&tt_context.vdp2width);
   opencl_cxt.ret = clSetKernelArg(opencl_cxt.kernel, 4, sizeof(int), (void *)&tt_context.vdp2width);

   size_t global_item_size = sizeof(vdp2framebuffer)/64;
   size_t local_item_size = sizeof(struct PixelData);
   size_t offset = (sizeof(vdp2framebuffer) / 64) * 2;

   opencl_cxt.ret = clEnqueueNDRangeKernel(opencl_cxt.command_queue, opencl_cxt.kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);

   //opencl_cxt.ret = clEnqueueReadBuffer(opencl_cxt.command_queue, opencl_cxt.pix_stack, CL_TRUE, 0, sizeof(pixel_stack_out), pixel_stack_out, 0, NULL, NULL);

   opencl_cxt.ret = clEnqueueReadBuffer(opencl_cxt.command_queue, opencl_cxt.output, CL_TRUE, 0, sizeof(u32) * tt_context.vdp2width * tt_context.vdp2height, dispbuffer, 0, NULL, NULL);

#else

   /* Set OpenCL Kernel Parameters */
   opencl_cxt.ret = clSetKernelArg(opencl_cxt.kernel, 0, sizeof(cl_mem), (void *)&opencl_cxt.memobj);

   /* Execute OpenCL Kernel */
   opencl_cxt.ret = clEnqueueTask(opencl_cxt.command_queue, opencl_cxt.kernel, 0, NULL, NULL);

   /* Copy results from the memory buffer */
   opencl_cxt.ret = clEnqueueReadBuffer(opencl_cxt.command_queue, opencl_cxt.memobj, CL_TRUE, 0, MEM_SIZE * sizeof(char), string, 0, NULL, NULL);

   /* Display Result */
   puts(string);

#endif

   return 0;
}

void opencl_close()
{
   /* Finalization */
   opencl_cxt.ret = clFlush(opencl_cxt.command_queue);
   opencl_cxt.ret = clFinish(opencl_cxt.command_queue);
   opencl_cxt.ret = clReleaseKernel(opencl_cxt.kernel);
   opencl_cxt.ret = clReleaseProgram(opencl_cxt.program);
   opencl_cxt.ret = clReleaseMemObject(opencl_cxt.memobj);
   opencl_cxt.ret = clReleaseCommandQueue(opencl_cxt.command_queue);
   opencl_cxt.ret = clReleaseContext(opencl_cxt.context);

   free(opencl_cxt.source_str);
}