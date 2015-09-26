struct PixelData
{
   uint pixel[6];
   uchar priority[6];
   uchar linescreen[6];
   uchar shadow_type[6];
   uchar shadow_enabled[6];
};

struct StackPixel
{
   uint pixel[2];
   uchar priority[2];
   uchar linescreen[2];
   uchar shadow_type[2];
   uchar shadow_enabled[2];
};

struct OnePixel
{
   uint pixel;
   uchar priority;
   uchar linescreen;
   uchar shadow_type;
   uchar shadow_enabled;
};

#define TITAN_BLEND_TOP     0
#define TITAN_BLEND_BOTTOM  1
#define TITAN_BLEND_ADD     2

#define TITAN_NORMAL_SHADOW 1
#define TITAN_MSB_SHADOW 2

uchar TitanGetAlpha(uint pixel) 
{ 
   return (pixel >> 24) & 0x3F;
}

uchar TitanTransAlpha(uint pixel)
{
   return TitanGetAlpha(pixel) < 0x3F;
}

uchar TitanGetRed(uint pixel)
{ 
   return (pixel >> 16) & 0xFF; 
}

uchar TitanGetGreen(uint pixel)
{ 
   return (pixel >> 8) & 0xFF; 
}

uchar TitanGetBlue(uint pixel)
{ 
   return pixel & 0xFF; 
}

uint TitanCreatePixel(uchar alpha, uchar red, uchar green, uchar blue) 
{
   return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

uint TitanBlendPixelsTop(uint top, uint bottom)
{
   uchar alpha, ralpha, tr, tg, tb, br, bg, bb;

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

uint TitanBlendPixelsBottom(uint top, uint bottom)
{
   uchar alpha, ralpha, tr, tg, tb, br, bg, bb;

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

uint TitanBlendPixelsAdd(uint top, uint bottom)
{
   uint r, g, b;

   r = TitanGetRed(top) + TitanGetRed(bottom);
   if (r > 0xFF) r = 0xFF;

   g = TitanGetGreen(top) + TitanGetGreen(bottom);
   if (g > 0xFF) g = 0xFF;

   b = TitanGetBlue(top) + TitanGetBlue(bottom);
   if (b > 0xFF) b = 0xFF;

   return TitanCreatePixel(0x3F, r, g, b);
}

uchar TitanTransBit(uint pixel)
{
   return (pixel & 0x80000000) != 0;
}

uchar IsTrans(uint pixel, int blend_mode)
{
   if (blend_mode == TITAN_BLEND_TOP)
   {
      return TitanTransAlpha(pixel); 
   }
   else
   {
      return TitanTransBit(pixel);
   }
}

uint Blend(uint top, uint bottom, int blend_mode)
{
   if (blend_mode == TITAN_BLEND_BOTTOM)
   {
      return TitanBlendPixelsBottom(top, bottom);
   }
   else if (blend_mode == TITAN_BLEND_ADD)
   {
      return TitanBlendPixelsAdd(top, bottom);
   }
   else
   {
      return TitanBlendPixelsTop(top, bottom);
   }
}

__kernel void hello(
   __global struct PixelData * p, 
   __global struct StackPixel * stack, 
   __global unsigned int * output, 
   const int width, const int height,
   const int blend_mode)
{
   int x = get_global_id(0);
   int y = get_global_id(1);

   int pixel_stack_pos = 0;

   if (x < 0 || y < 0) return;
  // if ( x >= width || y >= height) return;
   
   int p_pos = x + (y * width);

   for (int priority = 7; priority > 0; priority--)
   {
      for (int layer = 5; layer >= 0; layer--)
      {
         if (p[p_pos].priority[layer] == priority)
         {
            stack[p_pos].pixel[pixel_stack_pos] = p[p_pos].pixel[layer];
            //stack[p_pos].priority[pixel_stack_pos] = p[p_pos].priority[layer];
            //stack[p_pos].linescreen[pixel_stack_pos] = p[p_pos].linescreen[layer];
            //stack[p_pos].shadow_type[pixel_stack_pos] = p[p_pos].shadow_type[layer];
            //stack[p_pos].shadow_enabled[pixel_stack_pos] = p[p_pos].shadow_enabled[layer];
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               goto finished;
         }
      }
   }

finished:

   if (pixel_stack_pos < 1)
   {
      output[p_pos] = 0;
   }
   else
   {
      if (IsTrans(stack[p_pos].pixel[0], blend_mode))
      {
         output[p_pos] = Blend(stack[p_pos].pixel[0], stack[p_pos].pixel[1], blend_mode);
      }
      else
      {
         output[p_pos] = stack[p_pos].pixel[0];
      }

      //if (TitanTransAlpha(stack[p_pos].pixel[0]))
      //{
      //   output[p_pos] = ;
      //}
      //else
      //{
      //   output[p_pos] = stack[p_pos].pixel[0];
      //}
   }
}