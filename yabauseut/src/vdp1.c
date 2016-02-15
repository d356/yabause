/*  Copyright 2013 Theo Berkau

    This file is part of YabauseUT

    YabauseUT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    YabauseUT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with YabauseUT; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iapetus.h>
#include "tests.h"
#include "vdp1.h"

//////////////////////////////////////////////////////////////////////////////

void vdp1_test()
{
   int choice;

   menu_item_struct vdp1_menu[] = {
   { "Draw commands" , &vdp1_draw_test, },
   { "Clip commands" , &vdp1_clip_test, },
   { "Misc" , &vdp1_misc_test, },
   { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(vdp1_menu, &test_disp_font, 0, 0, "VDP1 Tests", MTYPE_CENTER, -1);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_draw_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_clip_test()
{
   int gouraud_table_address = 0x40000;
   u32 clipping_mode = 3;//outside

   u16* p = (u16 *)(0x25C00000 + gouraud_table_address);

   auto_test_sub_test_start("Clipping test");

   VDP1_REG_FBCR = 0;

   vdp_start_draw_list();

   sprite_struct quad;

   quad.x = 0;
   quad.y = 0;

   vdp_local_coordinate(&quad);

   //system clipping
   quad.x = 319 - 8;
   quad.y = 223 - 8;

   vdp_system_clipping(&quad);

   //user clipping
   quad.x = 8;
   quad.y = 8;
   quad.x2 = 319 - 16;
   quad.y2 = 223 - 16;

   vdp_user_clipping(&quad);

   //fullscreen polygon
   quad.x = 319;
   quad.y = 0;
   quad.x2 = 319;
   quad.y2 = 223;
   quad.x3 = 0;
   quad.y3 = 223;
   quad.x4 = 0;
   quad.y4 = 0;

   quad.bank = RGB16(0x10, 0x10, 0x10);//gray

   quad.gouraud_addr = gouraud_table_address;

   quad.attr = (clipping_mode << 9) | 4;//use gouraud shading

   //red, green, blue, and white
   p[0] = RGB16(31, 0, 0);
   p[1] = RGB16(0, 31, 0);
   p[2] = RGB16(0, 0, 31);
   p[3] = RGB16(31, 31, 31);

   vdp_draw_polygon(&quad);

   vdp_end_draw_list();

   vdp_vsync();

   VDP1_REG_FBCR = 3;

   vdp_vsync();
   vdp_vsync();

#ifdef BUILD_AUTOMATED_TESTING

   auto_test_get_framebuffer();

#else

   for (;;)
   {
      while (!(VDP2_REG_TVSTAT & 8)) 
      {
         ud_check(0);
      }

      while (VDP2_REG_TVSTAT & 8) 
      {
         
      }

      if (per[0].but_push_once & PAD_A)
      {
         clipping_mode = 0; //clipping disabled
      }
      if (per[0].but_push_once & PAD_B)
      {
         clipping_mode = 2; //inside drawing mode
      }
      if (per[0].but_push_once & PAD_C)
      {
         clipping_mode = 3; //outside drawing mode
      }

      if (per[0].but_push_once & PAD_START)
         break;

      if (per[0].but_push_once & PAD_X)
      {
         ar_menu();
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }

#endif

}

//////////////////////////////////////////////////////////////////////////////

void vdp1_misc_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_framebuffer_write_test()
{
   volatile u16* framebuffer_ptr = (volatile u16*)0x25C80000;
   int i;

   vdp_vsync();

   VDP1_REG_FBCR = 3;

   //we have to wait a little bit so we don't write to both framebuffers
   vdp_wait_hblankout();

   for (i = 0; i < 128; i += 4)
   {
      framebuffer_ptr[i + 0] = 0xdead;
      framebuffer_ptr[i + 1] = 0xbeef;
      framebuffer_ptr[i + 2] = 0xfeed;
      framebuffer_ptr[i + 3] = 0xcafe;
   }

#ifdef BUILD_AUTOMATED_TESTING

   auto_test_get_framebuffer();

#else

   for (;;)
   {
      while (!(VDP2_REG_TVSTAT & 8))
      {
         ud_check(0);
      }

      while (VDP2_REG_TVSTAT & 8) {}

      if (per[0].but_push_once & PAD_START)
         break;

      if (per[0].but_push_once & PAD_X)
      {
         ar_menu();
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
#endif
}

//////////////////////////////////////////////////////////////////////////////

int current_command = 0;

void write_test_vals(volatile u16* p)
{
   int i;
   for (i = 0; i < 15; i++)
   {
      p[i] = i;
   }
}

void test_draw_end()
{
   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   //write_test_vals(p);

   p[0] = 0x8000;

   current_command++;
}

void test_system_clipping()
{
   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   p[0] = 0x9;
   p[10] = 320;
   p[11] = 224;

   current_command++;
}

void test_user_clipping()
{
   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   p[0] = 0x8;

   p[6] = 0;
   p[7] = 0;

   p[10] = 320;
   p[11] = 224;

   current_command++;
}

void test_local_coordinate()
{
   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   p[0] = 0xa;

   p[6] = 0;
   p[7] = 0;

   current_command++;
}
#if 0
void write_test_sprite()
{
   int location = 1024;
   volatile u32 * spr_p = (volatile u32*)(VDP1_RAM + (location * 0x20));
   int i;
   int j;
   int width = 8;
   int height = 8;

   u32 spr[] = 
   {
      0x00010002,
      0x00030004,
      0x00050006,
      0x00070008,
      0x0009000a,
      0x000b000c,
      0x000d000e,
      0x000f0002 
   };

   for (i = 0; i < 8; i++)
   {
      spr_p[i] = spr[i];
   }

   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   p[0] = 0x0;

   p[6] = 0x4;//x = 4
   p[7] = 0xc;//y = 12

   current_command++;
}
#endif

void test_line(u32 x1, u32 y1, u32 x2, u32 y2)
{
   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   p[0] = 0x6;
   p[2] = 0x8000;
   p[3] = 0xffff;

   p[6] = x1;//x
   p[7] = y1;//y

   p[8] = x2;//x
   p[9] = y2;//y

   current_command++;
}

void test_normal_sprite()
{
   volatile u16 * p = (volatile u16*)(VDP1_RAM + (current_command * 0x20));

   p[0] = 0x0;

   p[6] = 0x4;//x = 4
   p[7] = 0xc;//y = 12

   current_command++;
}

void command_timing()
{

   //vdp2_sprite_window_test();

   VDP1_REG_PTMR = 0x02;
   VDP1_REG_FBCR = 0;
   VDP2_REG_SPCTL = (1 << 5) | 7;
   VDP2_REG_PRISA = 2 | (2 << 8);
   VDP2_REG_PRISB = 2 | (2 << 8);
   VDP2_REG_PRISC = 2 | (2 << 8);
   VDP2_REG_PRISD = 2 | (2 << 8);

   VDP2_REG_WPSX0 = 0;
   VDP2_REG_WPSY0 = 0;
   VDP2_REG_WPEX0 = 320;
   VDP2_REG_WPEY0 = 224;

   VDP2_REG_WPSX1 = 0;
   VDP2_REG_WPSY1 = 0;
   VDP2_REG_WPEX1 = 320;
   VDP2_REG_WPEY1 = 224;

   vdp1_clip_test();

   u16* p = (u16 *)(0x25C00000);

   p[1] = 1;
   p[2] = 2;
   p[3] = 3;
   p[4] = 4;
   p[5] = 5;
   p[6] = 6;
   p[7] = 7;
   p[8] = 8;

   int i;

   for (i = 0; i < 1024; i++)
   {
      p[i] = 0;
   }



   test_system_clipping();
   test_user_clipping();
   test_local_coordinate();
   test_line(4, 4,8,8);
   //test_line(4, 4, 128, 128);
   //test_line(4, 4, 8, 8);
   test_draw_end();

   for (;;)
   {
      vdp_vsync();
#if 0
      p[1] = 1;
      p[2] = 2;
      p[3] = 3;
      p[4] = 4;
      p[5] = 5;
      p[6] = 6;
      p[7] = 7;
      p[8] = 8;
#endif

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_framebuffer_tests()
{
   auto_test_section_start("Vdp1 framebuffer tests");

   vdp1_clip_test();

   auto_test_section_end();
}

//////////////////////////////////////////////////////////////////////////////
