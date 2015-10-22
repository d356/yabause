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

int sprite_window_inside[6] = { 0 };
int sprite_window_enabled[6] = { 0 };
int transparent_process_window_logic[6] = { 0 };
int transparent_process_window_enable[6][2] = { 0 };
int transparent_process_window_area[6][2] = { 0 };

clipping_struct window[2] = { 0 };
clipping_struct line_window[2] = { 0 };

int CheckWindow(int which_layer, int which_window, int x, int y, int pos)
{
   if (transparent_process_window_area[which_layer][which_window])
   {
      //outside
      if (x >= window[which_window].xstart && x <= window[which_window].xend &&
         y >= window[which_window].ystart && y <= window[which_window].yend)
      {
         return 0;
         //tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
      }
   }
   else
   {
      //inside
      if (x < window[which_window].xstart || x > window[which_window].xend ||
         y < window[which_window].ystart || y > window[which_window].yend)
      {
         return 0;
         //tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
      }
   }

   return 1;
}

int CheckSpriteWindow(int which_layer, int pos, struct PixelData sprite_pixel)
{
   if (sprite_window_inside[which_layer])
   {
      if (sprite_pixel.shadow_type != TITAN_MSB_SHADOW)
      {
         //tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
         return 1;
      }
   }
   else
   {
      if (sprite_pixel.shadow_type == TITAN_MSB_SHADOW)
      {
         //tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
         return 1;
      }
   }
   return 0;
}

void DoWindowProcess(int x, int y, int pos)
{
   struct PixelData sprite_pixel = tt_context.vdp2framebuffer[TITAN_SPRITE][pos];

   int which_layer;

   for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
   {
      int w0_result = CheckWindow(which_layer, 0, x, y, pos);

      int w1_result = CheckWindow(which_layer, 1, x, y, pos);

      int sprite_result = CheckSpriteWindow(which_layer, pos, sprite_pixel);

      w0_result = transparent_process_window_enable[which_layer][0] && w0_result;
      w1_result = transparent_process_window_enable[which_layer][1] && w1_result;
      sprite_result = sprite_window_enabled[which_layer] && sprite_result && (Vdp2Regs->SPCTL & 0x10);

      if (transparent_process_window_logic[which_layer])
      {
         //if (!(transparent_process_window_enable[which_layer][0] && transparent_process_window_enable[which_layer][1] && sprite_window_enabled[which_layer]))
         //{
         //   tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
         //}
         
         if (w0_result && w1_result && sprite_result)
         {
            tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
         }
      }
      else
      {
         if (w0_result || w1_result || sprite_result)
         {
            tt_context.vdp2framebuffer[which_layer][pos].priority = 0;
         }
      }
   }
}

static u32 TitanDigPixel(int pos, int x, int y)
{
   struct PixelData pixel_stack[2] = { 0 };

   int pixel_stack_pos = 0;

   int priority;

   int sprite_is_clipped = 0;

   DoWindowProcess(x, y, pos);

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

      if (!(Vdp2Regs->SPCTL & 0x10))
      {
         //sprite self-shadowing
         pixel_stack[0].pixel = TitanBlendPixelsTop(0x20000000, pixel_stack[0].pixel);
      }
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

void LineWindow(clipping_struct *clip, Vdp2* regs, u8* ram, u32 * linewndaddr)
{
   clip->xstart = T1ReadWord(ram, *linewndaddr);
   *linewndaddr += 2;
   clip->xend = T1ReadWord(ram, *linewndaddr);
   *linewndaddr += 2;

   /* Ok... that looks insane... but there's at least two games (3D Baseball and
   Panzer Dragoon Saga) that set the line window end to 0xFFFF and expect the line
   window to be invalid for those lines... */
   if (clip->xend == 0xFFFF)
   {
      clip->xstart = 0;
      clip->xend = 0;
      return;
   }

   clip->xstart &= 0x3FF;
   clip->xend &= 0x3FF;

   switch ((regs->TVMD >> 1) & 0x3)
   {
   case 0: // Normal
      clip->xstart = (clip->xstart >> 1) & 0x1FF;
      clip->xend = (clip->xend >> 1) & 0x1FF;
      break;
   case 1: // Hi-Res
      clip->xstart = clip->xstart & 0x3FF;
      clip->xend = clip->xend & 0x3FF;
      break;
   case 2: // Exclusive Normal
      clip->xstart = clip->xstart & 0x1FF;
      clip->xend = clip->xend & 0x1FF;
      break;
   case 3: // Exclusive Hi-Res
      clip->xstart = (clip->xstart & 0x3FF) >> 1;
      clip->xend = (clip->xend & 0x3FF) >> 1;
      break;
   }
}

void GetLineWindowBase(Vdp2 * regs, u32* addr)
{
  // if (regs->LWTA0.all & 0x80000000)
   {
   //   islinewindow[0] |= 0x1;
      addr[0] = (regs->LWTA0.all & 0x7FFFE) << 1;
   }
//   if (regs->LWTA1.all & 0x80000000)
   {
   //   islinewindow[0] |= 0x2;
      addr[1] = (regs->LWTA1.all & 0x7FFFE) << 1;
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


   u32 line_window_base_addr[2] = { 0 };
   u32 line_window_addr[2] = { 0 };

   GetLineWindowBase(Vdp2Regs, line_window_base_addr);
   
   for (y = start_line + interlace_line; y < end_line; y += line_increment)
   {

      line_window_addr[0] = line_window_base_addr[0] + (y * 4);
      line_window_addr[1] = line_window_base_addr[1] + (y * 4);

      LineWindow(window, Vdp2Regs, Vdp2Ram, &line_window_addr[0]);

      for (x = 0; x < tt_context.vdp2width; x++)
      {
         int i = (y * tt_context.vdp2width) + x;

         dispbuffer[i] = 0;

         dot = TitanDigPixel(i, x, y);

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

   ReadWindowCoordinates(0, &window[0], Vdp2Regs);
   ReadWindowCoordinates(1, &window[1], Vdp2Regs);

   transparent_process_window_logic[TITAN_NBG0] = (Vdp2Regs->WCTLA >> 7) & 1;
   transparent_process_window_logic[TITAN_NBG1] = (Vdp2Regs->WCTLA >> 15) & 1;
   transparent_process_window_logic[TITAN_NBG2] = (Vdp2Regs->WCTLB >> 7) & 1;
   transparent_process_window_logic[TITAN_NBG3] = (Vdp2Regs->WCTLB >> 15) & 1;
   transparent_process_window_logic[TITAN_RBG0] = (Vdp2Regs->WCTLC >> 7) & 1;
   transparent_process_window_logic[TITAN_SPRITE] = (Vdp2Regs->WCTLC >> 15) & 1;

   transparent_process_window_enable[TITAN_NBG0][0] = (Vdp2Regs->WCTLA >> 1) & 1;
   transparent_process_window_enable[TITAN_NBG1][0] = (Vdp2Regs->WCTLA >> 9) & 1;
   transparent_process_window_enable[TITAN_NBG2][0] = (Vdp2Regs->WCTLB >> 1) & 1;
   transparent_process_window_enable[TITAN_NBG3][0] = (Vdp2Regs->WCTLB >> 9) & 1;
   transparent_process_window_enable[TITAN_RBG0][0] = (Vdp2Regs->WCTLC >> 1) & 1;
   transparent_process_window_enable[TITAN_SPRITE][0] = (Vdp2Regs->WCTLC >> 9) & 1;

   transparent_process_window_enable[TITAN_NBG0][1] = (Vdp2Regs->WCTLA >> 3) & 1;
   transparent_process_window_enable[TITAN_NBG1][1] = (Vdp2Regs->WCTLA >> 11) & 1;
   transparent_process_window_enable[TITAN_NBG2][1] = (Vdp2Regs->WCTLB >> 3) & 1;
   transparent_process_window_enable[TITAN_NBG3][1] = (Vdp2Regs->WCTLB >> 11) & 1;
   transparent_process_window_enable[TITAN_RBG0][1] = (Vdp2Regs->WCTLC >> 3) & 1;
   transparent_process_window_enable[TITAN_SPRITE][1] = (Vdp2Regs->WCTLC >> 11) & 1;

   sprite_window_enabled[TITAN_NBG0] = (Vdp2Regs->WCTLA >> 5) & 1;
   sprite_window_enabled[TITAN_NBG1] = (Vdp2Regs->WCTLA >> 13) & 1;
   sprite_window_enabled[TITAN_NBG2] = (Vdp2Regs->WCTLB >> 5) & 1;
   sprite_window_enabled[TITAN_NBG3] = (Vdp2Regs->WCTLB >> 13) & 1;
   sprite_window_enabled[TITAN_SPRITE] = (Vdp2Regs->WCTLC >> 13) & 1;
   sprite_window_enabled[TITAN_RBG0] = (Vdp2Regs->WCTLC >> 5) & 1;

   transparent_process_window_area[TITAN_NBG0][0] = (Vdp2Regs->WCTLA >> 0) & 1;
   transparent_process_window_area[TITAN_NBG1][0] = (Vdp2Regs->WCTLA >> 8) & 1;
   transparent_process_window_area[TITAN_NBG2][0] = (Vdp2Regs->WCTLB >> 0) & 1;
   transparent_process_window_area[TITAN_NBG3][0] = (Vdp2Regs->WCTLB >> 8) & 1;
   transparent_process_window_area[TITAN_RBG0][0] = (Vdp2Regs->WCTLC >> 0) & 1;
   transparent_process_window_area[TITAN_SPRITE][0] = (Vdp2Regs->WCTLC >> 8) & 1;

   transparent_process_window_area[TITAN_NBG0][1] = (Vdp2Regs->WCTLA >> 2) & 1;
   transparent_process_window_area[TITAN_NBG1][1] = (Vdp2Regs->WCTLA >> 10) & 1;
   transparent_process_window_area[TITAN_NBG2][1] = (Vdp2Regs->WCTLB >> 2) & 1;
   transparent_process_window_area[TITAN_NBG3][1] = (Vdp2Regs->WCTLB >> 10) & 1;
   transparent_process_window_area[TITAN_RBG0][1] = (Vdp2Regs->WCTLC >> 2) & 1;
   transparent_process_window_area[TITAN_SPRITE][1] = (Vdp2Regs->WCTLC >> 10) & 1;

   sprite_window_inside[TITAN_NBG0] = (Vdp2Regs->WCTLA >> 4) & 1;
   sprite_window_inside[TITAN_NBG1] = (Vdp2Regs->WCTLA >> 12) & 1;
   sprite_window_inside[TITAN_NBG2] = (Vdp2Regs->WCTLB >> 4) & 1;
   sprite_window_inside[TITAN_NBG3] = (Vdp2Regs->WCTLB >> 12) & 1;
   sprite_window_inside[TITAN_SPRITE] = (Vdp2Regs->WCTLC >> 12) & 1;
   sprite_window_inside[TITAN_RBG0] = (Vdp2Regs->WCTLC >> 4) & 1;

#ifdef WANT_VIDSOFT_PRIORITY_THREADING

   if (0)//vidsoft_num_priority_threads > 0)
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
