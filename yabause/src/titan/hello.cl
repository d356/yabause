struct PixelData
{
   uint pixel;
   uchar priority;
   uchar linescreen;
   uchar shadow_type;
   uchar shadow_enabled;
};

__kernel void hello(
   __global struct PixelData * p, 
   __global struct PixelData * stack, 
   __global unsigned int * output, 
   const int width, const int height)
{
   int x = get_global_id(0);
   int y = get_global_id(1);

   int pixel_stack_pos = 0;

   if (x < 0 || y < 0) return;
  // if ( x >= width || y >= height) return;

   for (int priority = 7; priority > 0; priority--)
   {
      for (int layer = 5; layer >= 0; layer--)
      {
         int p_pos = x + (y * width) + (layer * width * height);

         if (p[p_pos].priority == priority)
         {
            if (pixel_stack_pos < 1)
            {
               stack[x + (y * width) + (pixel_stack_pos * width * height)] = p[p_pos];
               pixel_stack_pos++;
            }
         }
      }
   }


   output[x + (y * width)] = stack[x + (y * width)].pixel;
}