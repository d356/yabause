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
#include "../yui.h"

#include <stdlib.h>

/* private */
typedef u32 (*TitanBlendFunc)(u32 top, u32 bottom);
typedef int FASTCALL (*TitanTransFunc)(u32 pixel);

struct OnePixel
{
   u32 pixel;
   u8 priority;
   u8 linescreen;
   u8 shadow_type;
   u8 shadow_enabled;
};

struct StackPixel
{
   u32 pixel[2];
   u8 priority[2];
   u8 linescreen[2];
   u8 shadow_type[2];
   u8 shadow_enabled[2];
};

struct PixelData
{
   u32 pixel[6];
   u8 priority[6];
   u8 linescreen[6];
   u8 shadow_type[6];
   u8 shadow_enabled[6];
};

struct OnePixel PixelDataToOnePixel(int layer, struct PixelData * p)
{
   struct OnePixel pp = { 0 };
   pp.pixel = p->pixel[layer];
   pp.priority = p->priority[layer];
   pp.linescreen = p->linescreen[layer];
   pp.shadow_type = p->shadow_type[layer];
   pp.shadow_enabled = p->shadow_enabled[layer];

   return pp;
}

static struct TitanContext {
   int inited;
   struct PixelData vdp2framebuffer[704*512];
   u32 * linescreen[4];
   int vdp2width;
   int vdp2height;
   TitanBlendFunc blend;
   TitanTransFunc trans;
   struct OnePixel * backscreen;
} tt_context = {
   0,
   { NULL },
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

void DoStack(int pixel_stack_pos, int pos, int which_layer, struct StackPixel* pixel_stack)
{
   pixel_stack->pixel[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
   pixel_stack->priority[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
   pixel_stack->linescreen[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
   pixel_stack->shadow_type[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
   pixel_stack->shadow_enabled[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
}

static u32 TitanDigPixel(int pos, int y)
{
   struct StackPixel pixel_stack = { 0 };

   int pixel_stack_pos = 0;

   int priority;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (tt_context.vdp2framebuffer[pos].priority[which_layer] == priority)
         {
#if 1
            pixel_stack.pixel[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
            pixel_stack.priority[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
            pixel_stack.linescreen[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
            pixel_stack.shadow_type[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
            pixel_stack.shadow_enabled[pixel_stack_pos] = tt_context.vdp2framebuffer[pos].pixel[which_layer];
#endif
          //  DoStack(pixel_stack_pos, pos, which_layer, &pixel_stack);
            //pixel_stack[pixel_stack_pos] = PixelDataToOnePixel(which_layer, &tt_context.vdp2framebuffer[pos]);
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               goto finished;//backscreen is unnecessary in this case
         }
      }
   }

   pixel_stack.pixel[pixel_stack_pos] = tt_context.backscreen[pos].pixel;
   pixel_stack.priority[pixel_stack_pos] = tt_context.backscreen[pos].pixel;
   pixel_stack.linescreen[pixel_stack_pos] = tt_context.backscreen[pos].pixel;
   pixel_stack.shadow_type[pixel_stack_pos] = tt_context.backscreen[pos].pixel;
   pixel_stack.shadow_enabled[pixel_stack_pos] = tt_context.backscreen[pos].pixel;

  // pixel_stack[pixel_stack_pos] = tt_context.backscreen[pos];

finished:

   if (pixel_stack.linescreen[0])
   {
    //  pixel_stack.pixel[0] = tt_context.blend(pixel_stack.pixel[0], tt_context.linescreen[pixel_stack.linescreen[0]][y]);
   }

   if ((pixel_stack.shadow_type[0] == TITAN_MSB_SHADOW) && ((pixel_stack.pixel[0] & 0xFFFFFF) == 0))
   {
      //transparent sprite shadow
      if (pixel_stack.shadow_enabled[1])
      {
         pixel_stack.pixel[0] = TitanBlendPixelsTop(0x20000000, pixel_stack.pixel[1]);
      }
      else
      {
         pixel_stack.pixel[0] = pixel_stack.pixel[1];
      }
   }
   else if (pixel_stack.shadow_type[0] == TITAN_MSB_SHADOW && ((pixel_stack.pixel[0] & 0xFFFFFF) != 0))
   {
      if (tt_context.trans(pixel_stack.pixel[0]))
      {
         u32 bottom = pixel_stack.pixel[1];
         pixel_stack.pixel[0] = tt_context.blend(pixel_stack.pixel[0], bottom);
      }

      //sprite self-shadowing
      pixel_stack.pixel[0] = TitanBlendPixelsTop(0x20000000, pixel_stack.pixel[0]);
   }
   else if (pixel_stack.shadow_type[0] == TITAN_NORMAL_SHADOW)
   {
      if (pixel_stack.shadow_enabled[1])
      {
         pixel_stack.pixel[0] = TitanBlendPixelsTop(0x20000000, pixel_stack.pixel[1]);
      }
      else
      {
         pixel_stack.pixel[0] = pixel_stack.pixel[1];
      }
   }
   else
   {
      if (tt_context.trans(pixel_stack.pixel[0]))
      {
         u32 bottom = pixel_stack.pixel[1];
         pixel_stack.pixel[0] = tt_context.blend(pixel_stack.pixel[0], bottom);
      }
   }

   return pixel_stack.pixel[0];
}

/* public */
int TitanInit()
{
   int i;

   if (! tt_context.inited)
   {

 //        if ((tt_context.vdp2framebuffer = (struct PixelData *)calloc(sizeof(struct PixelData), 704 * 512)) == NULL)
 //           return -1;

      /* linescreen 0 is not initialized as it's not used... */
      for(i = 1;i < 4;i++)
      {
         if ((tt_context.linescreen[i] = (u32 *)calloc(sizeof(u32), 512)) == NULL)
            return -1;
      }

      if ((tt_context.backscreen = (struct OnePixel  *)calloc(sizeof(struct OnePixel), 704 * 512)) == NULL)
         return -1;

      tt_context.inited = 1;
   }

      memset(tt_context.vdp2framebuffer, 0, sizeof(struct PixelData) * 704 * 512);

   for(i = 1;i < 4;i++)
      memset(tt_context.linescreen[i], 0, sizeof(u32) * 512);

   return 0;
}

void TitanErase()
{
   int i = 0;

   memset(tt_context.vdp2framebuffer, 0, sizeof(struct PixelData) * tt_context.vdp2width * tt_context.vdp2height);
}

int TitanDeInit()
{
   int i;

   free(tt_context.vdp2framebuffer);

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

int titan_blend_mode = 0;

void TitanSetBlendingMode(int blend_mode)
{
   titan_blend_mode = blend_mode;

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
   struct OnePixel* buffer = &tt_context.backscreen[(y * tt_context.vdp2width)];
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
      tt_context.vdp2framebuffer[pos].pixel[info->titan_which_layer] = color;
      tt_context.vdp2framebuffer[pos].priority[info->titan_which_layer] = priority;
      tt_context.vdp2framebuffer[pos].linescreen[info->titan_which_layer] = linescreen;
      tt_context.vdp2framebuffer[pos].shadow_enabled[info->titan_which_layer] = info->titan_shadow_enabled;
      tt_context.vdp2framebuffer[pos].shadow_type[info->titan_which_layer] = info->titan_shadow_type;
   }
}
#if 0
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
#endif
int do_main();

struct StackPixel pixel_stack_out[704 * 512] = { 0 };
#if 0
u32 flat_dig(int pos)
{
   struct OnePixel pixel_stack[2] = { 0 };

   int pixel_stack_pos = 0;

   int priority;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         int flat_pos = pos + which_layer * tt_context.vdp2width * tt_context.vdp2height;
         if (vdp2framebuffer[flat_pos].priority[which_layer] == priority)
         {
            pixel_stack[pixel_stack_pos] = PixelDatavdp2framebuffer[flat_pos];
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
#endif
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
#if 0
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
#endif
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
   cl_mem framebuffer;
   cl_mem pix_stack;
   cl_mem output;
   cl_program program;
   cl_kernel kernel;
   cl_platform_id platform_id;
   cl_uint ret_num_devices;
   cl_uint ret_num_platforms;
   char *source_str;
}opencl_cxt;

const char* opencl_get_error(int error_code)
{
   switch (error_code)
   {
      case 0: return "CL_SUCCESS";
      case -1: return "CL_DEVICE_NOT_FOUND";
      case -2: return "CL_DEVICE_NOT_AVAILABLE";
      case -3: return "CL_COMPILER_NOT_AVAILABLE";
      case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
      case -5: return "CL_OUT_OF_RESOURCES";
      case -6: return "CL_OUT_OF_HOST_MEMORY";
      case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
      case -8: return "CL_MEM_COPY_OVERLAP";
      case -9: return "CL_IMAGE_FORMAT_MISMATCH";
      case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
      case -11: return "CL_BUILD_PROGRAM_FAILURE";
      case -12: return "CL_MAP_FAILURE";
      case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
      case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
      case -15: return "CL_COMPILE_PROGRAM_FAILURE";
      case -16: return "CL_LINKER_NOT_AVAILABLE";
      case -17: return "CL_LINK_PROGRAM_FAILURE";
      case -18: return "CL_DEVICE_PARTITION_FAILED";
      case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

      case -30: return "CL_INVALID_VALUE";
      case -31: return "CL_INVALID_DEVICE_TYPE";
      case -32: return "CL_INVALID_PLATFORM";
      case -33: return "CL_INVALID_DEVICE";
      case -34: return "CL_INVALID_CONTEXT";
      case -35: return "CL_INVALID_QUEUE_PROPERTIES";
      case -36: return "CL_INVALID_COMMAND_QUEUE";
      case -37: return "CL_INVALID_HOST_PTR";
      case -38: return "CL_INVALID_MEM_OBJECT";
      case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
      case -40: return "CL_INVALID_IMAGE_SIZE";
      case -41: return "CL_INVALID_SAMPLER";
      case -42: return "CL_INVALID_BINARY";
      case -43: return "CL_INVALID_BUILD_OPTIONS";
      case -44: return "CL_INVALID_PROGRAM";
      case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
      case -46: return "CL_INVALID_KERNEL_NAME";
      case -47: return "CL_INVALID_KERNEL_DEFINITION";
      case -48: return "CL_INVALID_KERNEL";
      case -49: return "CL_INVALID_ARG_INDEX";
      case -50: return "CL_INVALID_ARG_VALUE";
      case -51: return "CL_INVALID_ARG_SIZE";
      case -52: return "CL_INVALID_KERNEL_ARGS";
      case -53: return "CL_INVALID_WORK_DIMENSION";
      case -54: return "CL_INVALID_WORK_GROUP_SIZE";
      case -55: return "CL_INVALID_WORK_ITEM_SIZE";
      case -56: return "CL_INVALID_GLOBAL_OFFSET";
      case -57: return "CL_INVALID_EVENT_WAIT_LIST";
      case -58: return "CL_INVALID_EVENT";
      case -59: return "CL_INVALID_OPERATION";
      case -60: return "CL_INVALID_GL_OBJECT";
      case -61: return "CL_INVALID_BUFFER_SIZE";
      case -62: return "CL_INVALID_MIP_LEVEL";
      case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
      case -64: return "CL_INVALID_PROPERTY";
      case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
      case -66: return "CL_INVALID_COMPILER_OPTIONS";
      case -67: return "CL_INVALID_LINKER_OPTIONS";
      case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";
      default: return "Unknown OpenCL error";
   }

   return "Unknown OpenCL error";
}

void opencl_check_error(int ret, const char* call)
{
   if (ret != CL_SUCCESS)
   {
      char error_string[2048] = { 0 };
      const char* error = opencl_get_error(ret);
      sprintf(error_string, "OpenCL error calling %s: %s", call, error);
      YuiErrorMsg(error_string);
   }
}

void init_opencl()
{
   FILE *fp;
   char fileName[] = "C:/yabause-cl/yabause/yabause/src/titan/hello.cl";
   cl_int return_val = 0;
   
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
   return_val = clGetPlatformIDs(1, &opencl_cxt.platform_id, &opencl_cxt.ret_num_platforms);
   opencl_check_error(return_val, "clGetPlatformIDs");

   return_val = clGetDeviceIDs(opencl_cxt.platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &opencl_cxt.device_id, &opencl_cxt.ret_num_devices);
   opencl_check_error(return_val, "clGetDeviceIDs");

   /* Create OpenCL context */
   opencl_cxt.context = clCreateContext(NULL, 1, &opencl_cxt.device_id, NULL, NULL, &return_val);
   opencl_check_error(return_val, "clCreateContext");

   /* Create Command Queue */
   opencl_cxt.command_queue = clCreateCommandQueue(opencl_cxt.context, opencl_cxt.device_id, 0, &return_val);
   opencl_check_error(return_val, "clCreateCommandQueue");

   opencl_cxt.framebuffer = clCreateBuffer(opencl_cxt.context, CL_MEM_READ_ONLY, sizeof(tt_context.vdp2framebuffer), NULL, &return_val);
   opencl_check_error(return_val, "clCreateBuffer");

   opencl_cxt.pix_stack = clCreateBuffer(opencl_cxt.context, CL_MEM_READ_WRITE, sizeof(pixel_stack_out), NULL, &return_val);
   opencl_check_error(return_val, "clCreateBuffer");

   opencl_cxt.output = clCreateBuffer(opencl_cxt.context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 704 * 512, NULL, &return_val);
   opencl_check_error(return_val, "clCreateBuffer");

   /* Create Kernel Program from the source */
   opencl_cxt.program = clCreateProgramWithSource(opencl_cxt.context, 1, (const char **)&opencl_cxt.source_str,
      (const size_t *)&source_size, &return_val);
   opencl_check_error(return_val, "clCreateProgramWithSource");

   /* Build Kernel Program */
   return_val = clBuildProgram(opencl_cxt.program, 1, &opencl_cxt.device_id, NULL, NULL, NULL);
   opencl_check_error(return_val, "clBuildProgram");

   if (return_val < 0)
   {
      size_t log_size;
      return_val = clGetProgramBuildInfo(opencl_cxt.program, opencl_cxt.device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
      opencl_check_error(return_val, "clGetProgramBuildInfo");

      // Allocate memory for the log
      char *log = (char *)malloc(log_size);

      // Get the log
      return_val = clGetProgramBuildInfo(opencl_cxt.program, opencl_cxt.device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
      opencl_check_error(return_val, "clGetProgramBuildInfo");

      YuiErrorMsg(log);

      free(log);
   }

   /* Create OpenCL Kernel */
   opencl_cxt.kernel = clCreateKernel(opencl_cxt.program, "hello", &return_val);
   opencl_check_error(return_val, "clCreateKernel");
}

int inited = 0;

int opencl_exec(pixel_t * dispbuffer)
{
   cl_int return_val = 0;

   if (!inited)
   {
      init_opencl();
      inited = 1;
   }

   return_val = clEnqueueWriteBuffer(opencl_cxt.command_queue, opencl_cxt.framebuffer, CL_TRUE, 0, 
      sizeof(struct PixelData) * tt_context.vdp2width * tt_context.vdp2height, tt_context.vdp2framebuffer, 0, NULL, NULL);
   opencl_check_error(return_val, "clEnqueueWriteBuffer");

   return_val = clSetKernelArg(opencl_cxt.kernel, 0, sizeof(cl_mem), (void *)&opencl_cxt.framebuffer);
   opencl_check_error(return_val, "clSetKernelArg");

   return_val = clSetKernelArg(opencl_cxt.kernel, 1, sizeof(cl_mem), (void *)&opencl_cxt.pix_stack);
   opencl_check_error(return_val, "clSetKernelArg");

   return_val = clSetKernelArg(opencl_cxt.kernel, 2, sizeof(cl_mem), (void *)&opencl_cxt.output);
   opencl_check_error(return_val, "clSetKernelArg");

   return_val = clSetKernelArg(opencl_cxt.kernel, 3, sizeof(int), (void *)&tt_context.vdp2width);
   opencl_check_error(return_val, "clSetKernelArg");

   return_val = clSetKernelArg(opencl_cxt.kernel, 4, sizeof(int), (void *)&tt_context.vdp2width);
   opencl_check_error(return_val, "clSetKernelArg");

   return_val = clSetKernelArg(opencl_cxt.kernel, 5, sizeof(int), (void *)&titan_blend_mode);
   opencl_check_error(return_val, "clSetKernelArg");

   size_t global_item_size = sizeof(tt_context.vdp2framebuffer) / 64;
   size_t local_item_size = sizeof(struct PixelData);

   return_val = clEnqueueNDRangeKernel(opencl_cxt.command_queue, opencl_cxt.kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
   opencl_check_error(return_val, "clEnqueueNDRangeKernel");

   return_val = clEnqueueReadBuffer(opencl_cxt.command_queue, opencl_cxt.output, CL_TRUE, 0, sizeof(u32) * tt_context.vdp2width * tt_context.vdp2height, dispbuffer, 0, NULL, NULL);
   opencl_check_error(return_val, "clEnqueueReadBuffer");

   return 0;
}

void opencl_close()
{
   cl_int return_val = 0;

   /* Finalization */
   return_val = clFlush(opencl_cxt.command_queue);
   opencl_check_error(return_val, "clFlush");

   return_val = clFinish(opencl_cxt.command_queue);
   opencl_check_error(return_val, "clFinish");

   return_val = clReleaseKernel(opencl_cxt.kernel);
   opencl_check_error(return_val, "clReleaseKernel");

   return_val = clReleaseProgram(opencl_cxt.program);
   opencl_check_error(return_val, "clReleaseProgram");

   //return_val = clReleaseMemObject(opencl_cxt.memobj);
   return_val = clReleaseCommandQueue(opencl_cxt.command_queue);
   opencl_check_error(return_val, "clReleaseCommandQueue");

   return_val = clReleaseContext(opencl_cxt.context);
   opencl_check_error(return_val, "clReleaseContext");

   free(opencl_cxt.source_str);
}