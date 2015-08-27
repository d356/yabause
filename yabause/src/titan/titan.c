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
#include "../vdp2.h"

#include <stdlib.h>

/* private */
typedef u32 (*TitanBlendFunc)(u32 top, u32 bottom);
typedef int FASTCALL (*TitanTransFunc)(u32 pixel);

struct OnePixel {
   u8 priority;
   u8 is_msb_shadow;
   u8 shadow_enabled_on_layer;
   u8 line_screen_inserts;
   u8 color_calc_enable;
   u8 color_calc_ratio;
   u32 pixel;
};

struct PixelsAtPos
{
   struct OnePixel pixels[6];
};

static struct TitanContext {
   int inited;
   struct PixelsAtPos bg_layers[704 * 480];
   u32 backscreen[480];
   u32 linescreen[480];
   int vdp2width;
   int vdp2height;
   TitanBlendFunc blend;
   TitanTransFunc trans;
   int color_calc_enable[8];
   int color_calc_ratio[8];
   int extended_color_calc;
} tt_context;

#if defined WORDS_BIGENDIAN
#ifdef USE_RGB_555
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel >> 16) & 0xF800) | ((pixel >> 13) & 0x7C0) | ((pixel >> 10) & 0x3E)); }
#elif USE_RGB_565
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel >> 16) & 0xF800) | ((pixel >> 13) & 0x7E0) | ((pixel >> 11) & 0x1F)); }
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
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel >> 3) & 0x1F) | ((pixel >> 6) & 0x3E0) | ((pixel >> 9) & 0x7C00)); }
#elif USE_RGB_565
static INLINE u32 TitanFixAlpha(u32 pixel) { return (((pixel >> 3) & 0x1F) | ((pixel >> 5) & 0x7E0) | ((pixel >> 8) & 0xF800)); }
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

//when selecting by top
//when the top layer is 0 is fully opaque
//when the top layer is 31 is fully transparent

//when selecting by bottom
//when the bottom layer is 31 the top layer is fully transparent. you only see the bottom layer
//when the bottom layer is 0 the top layer is fully opaque. you only see the top layer
static u32 TitanBlendPixelsExtendedBottom(u32 top, u32 bottom, u32 top_alpha, u32 bottom_alpha)
{
   u8 tr, tg, tb, br, bg, bb;

   top_alpha <<= 3;
   bottom_alpha <<= 3;

   top_alpha = 0xff - bottom_alpha;

   tr = (TitanGetRed(top) * top_alpha) / 0xFF;
   tg = (TitanGetGreen(top) * top_alpha) / 0xFF;
   tb = (TitanGetBlue(top) * top_alpha) / 0xFF;

   br = (TitanGetRed(bottom) * bottom_alpha) / 0xFF;
   bg = (TitanGetGreen(bottom) * bottom_alpha) / 0xFF;
   bb = (TitanGetBlue(bottom) * bottom_alpha) / 0xFF;

   return TitanCreatePixel(0x3f, tr + br, tg + bg, tb + bb);
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

static u32 TitanBlendPixelsAddExtended(u32 a_in, u32 b_in, int a_shift, int b_shift)
{
   int ra = TitanGetRed(a_in);
   int ga = TitanGetGreen(a_in);
   int ba = TitanGetBlue(a_in);

   int rb = TitanGetRed(b_in);
   int gb = TitanGetGreen(b_in);
   int bb = TitanGetBlue(b_in);

   ra >>= a_shift;
   ga >>= a_shift;
   ba >>= a_shift;

   rb >>= b_shift;
   gb >>= b_shift;
   bb >>= b_shift;

   int r = ra + rb;
   int g = ga + gb;
   int b = ba + bb;

   int clamp = 0xff;
   if (r > clamp) 
      r = clamp;
   if (g > clamp)
      g = clamp;
   if (b > clamp) 
      b = clamp;

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

int TitanDoPriority(int pos, struct OnePixel *pos_stack)
{
   struct PixelsAtPos bg_layers_at_pos = tt_context.bg_layers[pos];
   int stack_pos = 1;

   //sort the pixels from highest to lowest priority
   for (int priority = 7; priority > 0; priority--)
   {
      for (int which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (bg_layers_at_pos.pixels[which_layer].priority == priority)
         {
   //         bg_layers_at_pos.pixels[which_layer].color_calc_enable = tt_context.color_calc_enable[which_layer];
    //        bg_layers_at_pos.pixels[which_layer].color_calc_ratio = tt_context.color_calc_ratio[which_layer];
            pos_stack[stack_pos] = bg_layers_at_pos.pixels[which_layer];
            stack_pos++;
         }
      }
   }

   return stack_pos;
}

u32 TitanDoExtendedColorCalc(int  stack_pos, struct OnePixel *pos_stack)
{
   struct OnePixel first = pos_stack[stack_pos];
   struct OnePixel second = pos_stack[stack_pos + 1];
   struct OnePixel third = pos_stack[stack_pos + 2];

   u32 outpixel = 0;
#if 0
   if (second.line_screen_inserts)
   {
      if (second.color_calc_enable)
      {
         return TitanBlendPixelsAddExtended(first.pixel, second.pixel, 0, 0);
      }
      else
      {
         u32 second_blend = TitanBlendPixelsAddExtended(second.pixel, third.pixel, 1, 5);

       //  outpixel = TitanBlendPixelsAddExtended(first.pixel, second_blend, 2, 0);

       //  return outpixel;
         if ((Vdp2Regs->CCCTL >> 8) & 1)
         {

         }
         else
         {

            u32 first_ratio = pos_stack[stack_pos].color_calc_ratio;
            u32 second_ratio = pos_stack[stack_pos + 1].color_calc_ratio;

            return TitanBlendPixelsExtendedBottom(first.pixel, second_blend, first_ratio, second_ratio);
          //  return tt_context.blend(first.pixel, second_blend);
            //additive
          //  return TitanBlendPixelsAddExtended(first.pixel, second_blend, 2, 0);
         }
      }
   }
#endif
   if (!first.color_calc_enable)
   {
      outpixel = first.pixel;
   }
   else if (first.color_calc_enable && !second.color_calc_enable)
   {
      outpixel = tt_context.blend(first.pixel, second.pixel);
   }
   else
   {
      int second_blend = 0;

      if (((Vdp2Regs->RAMCTL >> 12) & 3) == 0)
      {
         second_blend = TitanBlendPixelsAddExtended(second.pixel, third.pixel, 1, 5);
      }

      if (((Vdp2Regs->CCCTL) >> 9) & 1)
      {
         //select per second screen
         u32 first_ratio = pos_stack[stack_pos].color_calc_ratio;
         u32 second_ratio = pos_stack[stack_pos + 1].color_calc_ratio;

         outpixel = TitanBlendPixelsExtendedBottom(first.pixel, second_blend, first_ratio, second_ratio);
      }
      else
      {
         //select per top
         outpixel = tt_context.blend(first.pixel, second_blend);
      }
   }

   return outpixel;
}

static u32 TitanDigPixel(int x, int y, int debug)
{
   struct OnePixel pos_stack[8] = { 0 };

   int stack_pos = TitanDoPriority((y*tt_context.vdp2width) + x, &pos_stack);

   if (y == 8 * 13)
   {
      if (x == 8 * 0)
      {
         int i = 0;
      }
   }

   //vdp2 viewer
   if (debug)
   {
      return pos_stack[1].pixel;
   }

   //bottom pixel is always the back screen
   pos_stack[stack_pos++].pixel = tt_context.backscreen[y];

   //put the line screen in front if it is enabled
   if (pos_stack[1].line_screen_inserts)
   {
      pos_stack[0].pixel = tt_context.linescreen[y];
      pos_stack[0].color_calc_enable = (Vdp2Regs->CCCTL >> 5) & 1;
      pos_stack[0].color_calc_ratio = Vdp2Regs->CCRLB & 0x1f;
      stack_pos = 0;
   }
   else
   {
      stack_pos = 1;
   }

   u32 outpixel = pos_stack[stack_pos].pixel;

   if (outpixel == 0)
      return 0;

   if (tt_context.extended_color_calc)
   {
         outpixel = TitanDoExtendedColorCalc(stack_pos, pos_stack);
   }
   else
   {
      //blend the line screen
      if (pos_stack[1].line_screen_inserts)
      {
         outpixel = tt_context.blend(pos_stack[1].pixel, pos_stack[0].pixel);
      }
      else
      {
         if (tt_context.trans(outpixel))
         {
            u32 below = pos_stack[stack_pos + 1].pixel;
            outpixel = tt_context.blend(outpixel, below);
         }
      }
   }

   //normal shadow
   if (pos_stack[stack_pos].pixel == TITAN_NORMAL_SHADOW_VALUE)
   {
      if (pos_stack[stack_pos + 1].shadow_enabled_on_layer)
      {
         outpixel = TitanBlendPixelsTop(0x20000000, pos_stack[stack_pos + 1].pixel);
      }
      else
      {
         outpixel = pos_stack[stack_pos + 1].pixel;
      }
   }

   //handle msb shadow
   if (pos_stack[stack_pos].is_msb_shadow)
   {
      if ((pos_stack[stack_pos].pixel & 0x7ff) == 0)
      {
         //layer below the shadow has to have shadow enabled
         if (pos_stack[stack_pos + 1].shadow_enabled_on_layer)
         {
            //transparent shadow
            outpixel = TitanBlendPixelsTop(0x20000000, pos_stack[stack_pos + 1].pixel);
         }
         else
         {
            outpixel = pos_stack[stack_pos + 1].pixel;
         }
      }
      else
         //sprite shadow
         outpixel = TitanBlendPixelsTop(0x20000000, outpixel);
   }

   return outpixel;
}

/* public */
int TitanInit()
{
   memset(&tt_context, 0, sizeof(tt_context));
   tt_context.vdp2width = 320;
   tt_context.vdp2height = 224;
   tt_context.blend = NULL;
   tt_context.trans = NULL;
   tt_context.inited = 1;
   return 0;
}

void TitanErase()
{
   memset(tt_context.bg_layers, 0, sizeof(tt_context.bg_layers));
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

   tt_context.extended_color_calc = (Vdp2Regs->CCCTL >> 10) & 1;
}

void TitanPutBackHLine(s32 y, u32 color)
{
   tt_context.backscreen[y] = color;
}

void TitanPutLineHLine(int linescreen, s32 y, u32 color)
{
   if (linescreen == 0) return;

   tt_context.linescreen[y] = color;
}

void TitanPutPixel(int priority, s32 x, s32 y, u32 color, 
   int linescreen, int which_layer, int shadow, int color_calc_enable,
   int color_calc_ratio)
{
   int pos = (y * tt_context.vdp2width) + x;
   tt_context.bg_layers[pos].pixels[which_layer].priority = priority;
   tt_context.bg_layers[pos].pixels[which_layer].pixel = color;
   tt_context.bg_layers[pos].pixels[which_layer].shadow_enabled_on_layer = shadow;
   tt_context.bg_layers[pos].pixels[which_layer].line_screen_inserts = linescreen;
   tt_context.bg_layers[pos].pixels[which_layer].color_calc_enable = color_calc_enable;
   tt_context.bg_layers[pos].pixels[which_layer].color_calc_ratio = color_calc_ratio;
}

void TitanPutShadow(int priority, s32 x, s32 y, int type)
{
   if (priority == 0) return;

   int pos = (y * tt_context.vdp2width) + x;

   if (type == TITAN_NORMAL_SHADOW)//normal shadow
   {
      tt_context.bg_layers[pos].pixels[TITAN_SPRITE].priority = priority;
      tt_context.bg_layers[pos].pixels[TITAN_SPRITE].pixel = TITAN_NORMAL_SHADOW_VALUE;
   }
   else
   {
      tt_context.bg_layers[pos].pixels[TITAN_SPRITE].is_msb_shadow = 1;
   }
}

#include <omp.h>

void TitanRender(pixel_t * dispbuffer, int debug)
{
   u32 dot = 0;
   int y = 0;

//#pragma omp parallel for private(y)
   for (y = 0; y < tt_context.vdp2height; y++)
   {
      int x;

//#pragma omp parallel for
      for (x = 0; x < tt_context.vdp2width; x++)
      {
         dot = TitanDigPixel(x, y, debug);

         if (dot)
         {
            dispbuffer[(y* tt_context.vdp2width) + x] = TitanFixAlpha(dot);
         }
      }
   }
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
