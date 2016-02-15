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
#include "../vidsoft.h"
#include "../threads.h"

#include <stdlib.h>

#include "C:/Users/d/Downloads/ispc-v1.8.2-windows/stuff_ispc.h"

#define WANT_VIDSOFT_PRIORITY_THREADING 1

/* private */
typedef u32 (*TitanBlendFunc)(u32 top, u32 bottom);
typedef int FASTCALL (*TitanTransFunc)(u32 pixel);
void TitanRenderLines(pixel_t * dispbuffer, int start_line, int end_line);

int vidsoft_num_priority_threads = 0;

struct PixelData
{
   u32 pixel;
   u8 priority;
   u8 linescreen;
   u8 shadow_type;
   u8 shadow_enabled;
};

struct PixelDataSoa
{
   u32 pixel[6];
   u8 priority[6];
   u8 linescreen[6];
   u8 shadow_type[6];
   u8 shadow_enabled[6];
};

struct PixelDataSoa soa_thing[704 * 256];

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

#ifdef WANT_VIDSOFT_PRIORITY_THREADING

struct
{
   volatile int need_draw[5];
   volatile int draw_finished[5];
   struct
   {
      volatile int start;
      volatile int end;
   }lines[5];

   pixel_t * dispbuffer;
}priority_thread_context;

#endif

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

#ifdef WANT_VIDSOFT_PRIORITY_THREADING

#define DECLARE_PRIORITY_THREAD(FUNC_NAME, THREAD_NUMBER) \
void FUNC_NAME(void* data) \
{ \
   for (;;) \
   { \
      if (priority_thread_context.need_draw[THREAD_NUMBER]) \
      { \
         priority_thread_context.need_draw[THREAD_NUMBER] = 0; \
         TitanRenderLines(priority_thread_context.dispbuffer, priority_thread_context.lines[THREAD_NUMBER].start, priority_thread_context.lines[THREAD_NUMBER].end); \
         priority_thread_context.draw_finished[THREAD_NUMBER] = 1; \
      } \
      YabThreadSleep(); \
   } \
}

DECLARE_PRIORITY_THREAD(VidsoftPriorityThread0, 0);
DECLARE_PRIORITY_THREAD(VidsoftPriorityThread1, 1);
DECLARE_PRIORITY_THREAD(VidsoftPriorityThread2, 2);
DECLARE_PRIORITY_THREAD(VidsoftPriorityThread3, 3);
DECLARE_PRIORITY_THREAD(VidsoftPriorityThread4, 4);
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

void kernel(int pos, struct PixelData *pixel_stack, struct PixelData * vdp2framebuffer[6], struct PixelData* back_screen)
{
   int priority;
   int pixel_stack_pos = 0;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (vdp2framebuffer[which_layer][pos].priority == priority)
         {
            pixel_stack[pixel_stack_pos] = vdp2framebuffer[which_layer][pos];
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               return;
         }
      }
   }

   pixel_stack[pixel_stack_pos] = back_screen[pos];
}

struct PixelData make_pixel(struct PixelDataSoa input, int which)
{
   struct PixelData data = { 0 };

   data.linescreen = input.linescreen[which];
   data.pixel = input.pixel[which];
   data.priority = input.priority[which];
   data.shadow_enabled = input.shadow_enabled[which];
   data.shadow_type = input.shadow_type[which];

   return data;
}

void kernel_soa(int pos, struct PixelData *pixel_stack, struct PixelDataSoa vdp2framebuffer, struct PixelData* back_screen)
{
   int priority;
   int pixel_stack_pos = 0;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (vdp2framebuffer.priority[which_layer] == priority)
         {
            pixel_stack[pixel_stack_pos] = make_pixel(vdp2framebuffer, which_layer);//vdp2framebuffer[which_layer][pos];
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               return;
         }
      }
   }

   pixel_stack[pixel_stack_pos] = back_screen[pos];
}

void set_pixel(u32 *pixel,
   u8 *priority,
   u8 *linescreen,
   u8 *shadow_type,
   u8 *shadow_enabled, struct PixelDataSoa vdp2framebuffer, int which_layer)
{
   *linescreen = vdp2framebuffer.linescreen[which_layer];
   *pixel = vdp2framebuffer.pixel[which_layer];
   *priority = vdp2framebuffer.priority[which_layer];
   *shadow_enabled = vdp2framebuffer.shadow_enabled[which_layer];
   *shadow_type = vdp2framebuffer.shadow_type[which_layer];

   //*linescreen = *linescreen + 1;
   //*pixel = *pixel + 1;
   //*priority = *priority + 1;
   //*shadow_enabled = *shadow_enabled + 1;
   //*shadow_type = *shadow_type + 1;
}

void kernel_soa_arr(int pos, 
u32 *pixel,
u8 *priority,
u8 *linescreen,
u8 *shadow_type,
u8 *shadow_enabled,
struct PixelDataSoa vdp2framebuffer, 
struct PixelData* back_screen)
{
   int priority_;
   int pixel_stack_pos = 0;

   //sort the pixels from highest to lowest priority
   for (priority_ = 7; priority_ > 0; priority_--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (vdp2framebuffer.priority[which_layer] == priority_)
         {
            set_pixel(pixel, priority, linescreen, shadow_type, shadow_enabled, vdp2framebuffer, which_layer);

            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               return;
         }
      }
   }

//   pixel_stack[pixel_stack_pos] = back_screen[pos];
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

      //sprite self-shadowing, only if sprite window is not enabled
      if (!(Vdp2Regs->SPCTL & 0x10))
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

#ifdef WANT_VIDSOFT_PRIORITY_THREADING

      for (i = 0; i < 5; i++)
      {
         priority_thread_context.draw_finished[i] = 1;
         priority_thread_context.need_draw[i] = 0;
      }

      YabThreadStart(YAB_THREAD_VIDSOFT_PRIORITY_0, VidsoftPriorityThread0, NULL);
      YabThreadStart(YAB_THREAD_VIDSOFT_PRIORITY_1, VidsoftPriorityThread1, NULL);
      YabThreadStart(YAB_THREAD_VIDSOFT_PRIORITY_2, VidsoftPriorityThread2, NULL);
      YabThreadStart(YAB_THREAD_VIDSOFT_PRIORITY_3, VidsoftPriorityThread3, NULL);
      YabThreadStart(YAB_THREAD_VIDSOFT_PRIORITY_4, VidsoftPriorityThread4, NULL);
#endif

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

struct PixelData pixels[704 * 256][2] = { 0 };

void make_soa(int y, int start_line, int interlace_line, int end_line, int line_increment)
{
   int x, screen;

   for (y = start_line + interlace_line; y < end_line; y += line_increment)
   {
      for (x = 0; x < tt_context.vdp2width; x++)
      {
         int i = (y * tt_context.vdp2width) + x;

         for (screen = 0; screen < 6; screen++)
         {
            soa_thing[i].linescreen[screen] = tt_context.vdp2framebuffer[screen][i].linescreen;
            soa_thing[i].pixel[screen] = tt_context.vdp2framebuffer[screen][i].pixel;
            soa_thing[i].priority[screen] = tt_context.vdp2framebuffer[screen][i].priority;
            soa_thing[i].shadow_enabled[screen] = tt_context.vdp2framebuffer[screen][i].shadow_enabled;
            soa_thing[i].shadow_type[screen] = tt_context.vdp2framebuffer[screen][i].shadow_type;
         }
      }
   }
}

void TitanRenderLines(pixel_t * dispbuffer, int start_line, int end_line)
{
   int x, y;
   u32 dot;
   int line_increment, interlace_line;

   if (!tt_context.inited || (!tt_context.trans))
   {
      return;
   }

   Vdp2GetInterlaceInfo(&interlace_line, &line_increment);
   
   for (y = start_line + interlace_line; y < end_line; y += line_increment)
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
}

//num + 1 needs to be an even number to avoid issues with interlace modes
void VIDSoftSetNumPriorityThreads(int num)
{
   vidsoft_num_priority_threads = num > 5 ? 5 : num;

   if (num == 2)
      vidsoft_num_priority_threads = 1;

   if (num == 4)
      vidsoft_num_priority_threads = 3;
}

#ifdef WANT_VIDSOFT_PRIORITY_THREADING

void TitanStartPriorityThread(int which)
{
   priority_thread_context.need_draw[which] = 1;
   priority_thread_context.draw_finished[which] = 0;
   YabThreadWake(YAB_THREAD_VIDSOFT_PRIORITY_0 + which);
}

void TitanWaitForPriorityThread(int which)
{
   while (!priority_thread_context.draw_finished[which]){}
}

void TitanRenderThreads(pixel_t * dispbuffer)
{
   int i;
   int total_jobs = vidsoft_num_priority_threads + 1;//main thread runs a job
   int num_lines_per_job = tt_context.vdp2height / total_jobs;
   int remainder = tt_context.vdp2height % total_jobs;
   int starts[6] = { 0 }; 
   int ends[6] = { 0 };

   priority_thread_context.dispbuffer = dispbuffer;

   for (i = 0; i < total_jobs; i++)
   {
      starts[i] = num_lines_per_job * i;
      ends[i] = ((i + 1) * num_lines_per_job);
   }

   for (i = 0; i < vidsoft_num_priority_threads; i++)
   {
      priority_thread_context.lines[i].start = starts[i+1];
      priority_thread_context.lines[i].end = ends[i+1];
   }

   //put any remaining lines on the last thread
   priority_thread_context.lines[vidsoft_num_priority_threads - 1].end += remainder;

   for (i = 0; i < vidsoft_num_priority_threads; i++)
   {
      TitanStartPriorityThread(i);
   }

   TitanRenderLines(dispbuffer, starts[0], ends[0]);

   for (i = 0; i < vidsoft_num_priority_threads; i++)
   {
      TitanWaitForPriorityThread(i);
   }
}

#endif

void TitanRender(pixel_t * dispbuffer)
{
   if (!tt_context.inited || (!tt_context.trans))
   {
      return;
   }

#ifdef WANT_VIDSOFT_PRIORITY_THREADING

   if (vidsoft_num_priority_threads > 0)
   {

      TitanRenderThreads(dispbuffer);
   }
   else
   {
      TitanRenderLines(dispbuffer, 0, tt_context.vdp2height);
   }

#else
   TitanRenderLines(dispbuffer, 0, tt_context.vdp2height);
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
