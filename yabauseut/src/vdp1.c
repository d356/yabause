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
#include <stdio.h>
#include "vdp1.h"
#include "sh2\sh2int.h"
#include "scu.h"

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
#if 0
struct {
   volatile int draw_end;
   volatile int vblank_in;
   volatile int vblank_out;

   volatile int cur_line;
   volatile int write_to_screen;

   volatile int timer0_do_write;
   volatile int do_draw;
   volatile int x, dir;

   volatile int draw_incomplete;
}vd1_manual;

void vdp1_manual_reset_state()
{
   vd1_manual.draw_end = 0;
   vd1_manual.vblank_in = 0;
   vd1_manual.vblank_out = 0;

   vd1_manual.cur_line = 0;
   vd1_manual.write_to_screen = 0;

   vd1_manual.timer0_do_write = 0;
   vd1_manual.do_draw = 0;

   vd1_manual.x = 0;
   vd1_manual.dir = 1;

   vd1_manual.draw_incomplete = 0;
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_manual_write_str(char * str, int palette)
{
   vd1_manual.cur_line++;

   if (vd1_manual.cur_line > 31)
      return;

   write_str_as_pattern_name_data(0, vd1_manual.cur_line, str, palette, 0x000000, 0x40000);
}

//////////////////////////////////////////////////////////////////////////////

void draw_next_frame(int set)
{
   vd1_manual.do_draw = set;
}

//////////////////////////////////////////////////////////////////////////////

int get_draw_next_frame()
{
   return vd1_manual.do_draw;
}

//////////////////////////////////////////////////////////////////////////////

void timer_set(int val)
{
   vd1_manual.timer0_do_write = val;
}

//////////////////////////////////////////////////////////////////////////////

int timer_get()
{
   return vd1_manual.timer0_do_write;
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_print_status()
{
   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };

   int ptmr = (VDP1_REG_MODR >> 8) & 1;
   int fcm = (VDP1_REG_MODR >> 4) & 1;
   int vbe = (VDP1_REG_MODR >> 3) & 1;
   int cef = (VDP1_REG_EDSR >> 1) & 1;
   int bef = VDP1_REG_EDSR & 1;

   sprintf(str, "PTMR=%d FCM=%d VBE=%d CEF=%d BEF=%d           ", ptmr, fcm, vbe, cef, bef);
   vdp1_manual_write_str(str, 3);
}

//////////////////////////////////////////////////////////////////////////////
#if 0
void set_vbe(int vbe)
{
   VDP1_REG_TVMR = vbe << 3;

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };
   sprintf(str, "Setting VBE=%d                                 ", vbe);
   vdp1_manual_write_str(str, 4);

   vdp1_print_status();
}

//////////////////////////////////////////////////////////////////////////////

void set_fbcr(int fcm, int fct)
{
   VDP1_REG_FBCR = (fcm << 1) | fct;

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };
   sprintf(str, "Setting FCM=%d FCT=%d                          ", fcm, fct);
   vdp1_manual_write_str(str, 4);

   vdp1_print_status();
}

//////////////////////////////////////////////////////////////////////////////

void set_ptmr(int ptmr)
{
   VDP1_REG_PTMR = ptmr;

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };
   sprintf(str, "Setting PTMR=%d                                ", ptmr);
   vdp1_manual_write_str(str, 4);

   vdp1_print_status();
}
#endif
//////////////////////////////////////////////////////////////////////////////

void vdp1_manual_draw_list(int x, int y)
{
   vdp_start_draw_list();

   sprite_struct localcoord;

   localcoord.attr = 0;
   localcoord.x = 0;
   localcoord.y = 0;

   vdp_local_coordinate(&localcoord);

   sprite_struct quad = { 0 };

   int size = 7;

   int top_right_x = x;
   int top_right_y = y;
   quad.x = top_right_x + size;
   quad.y = top_right_y;
   quad.x2 = top_right_x + size;
   quad.y2 = top_right_y + size;
   quad.x3 = top_right_x;
   quad.y3 = top_right_y + size;
   quad.x4 = top_right_x;
   quad.y4 = top_right_y;
   quad.bank = RGB16(0x10, 0x10, 0x10);
   vdp_draw_polygon(&quad);

   vdp_end_draw_list();
}

//////////////////////////////////////////////////////////////////////////////

void vblank_in_handler()
{
   vd1_manual.vblank_in++;

   //if (vd1_manual.draw_incomplete)
   //{
   //   timer_set(1);
   //   vd1_manual.draw_incomplete = 0;
   //}
   //else
   //{
      if (vd1_manual.vblank_out % 3 == 0)
      {
         timer_set(1);
      }
      else
         timer_set(0);
   //}

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };

   sprintf(str, "VBlank In Interrupt #%d                     ", vd1_manual.vblank_in);
   vdp1_manual_write_str(str, 5);
   //vdp1_print_status();
   vdp1_manual_write_str("                                                 ", 5);
}

//////////////////////////////////////////////////////////////////////////////

void vblank_out_handler()
{
   // Issue Intback command(we only want peripheral data)
   smpc_wait_till_ready();
   SMPC_REG_IREG(0) = 0x00; // no intback status
   SMPC_REG_IREG(1) = 0x0A; // 15-byte mode, peripheral data returned, time optimized
   SMPC_REG_IREG(2) = 0xF0; // ???
   smpc_issue_command(SMPC_CMD_INTBACK);

   vd1_manual.vblank_out++;

   if (get_draw_next_frame())
   {
      VDP1_REG_TVMR = 0 << 3;
      VDP1_REG_PTMR = 1;
      draw_next_frame(0);
   }

   if (vd1_manual.x > 319 - 8)
   {
      vd1_manual.dir = -vd1_manual.dir;
   }

   if (vd1_manual.x < 0)
   {
      vd1_manual.dir = -vd1_manual.dir;
   }

   vd1_manual.x += vd1_manual.dir;

   vdp1_manual_draw_list(vd1_manual.x, 0);

   if (per[0].but_push_once & PAD_A)
   {
      vdp1_manual_reset_state();
      //         test_finished = 1;
      vdp1_manual_write_str("END                                                            ", 4);
      vdp1_manual_write_str("                                                               ", 4);
      vdp1_manual_write_str("                                                               ", 4);
   }

   if (per[0].but_push_once & PAD_START)
   {
      void(*ar)() = (void(*)())0x02000100;

      ar();
   }

   if (!vd1_manual.write_to_screen)
      return;

   char intstr[64] = { 0 };

   sprintf(intstr, "VBlank Out Interrupt #%d                     ", vd1_manual.vblank_out);
   vdp1_manual_write_str(intstr, 5);
   //vdp1_print_status();
   vdp1_manual_write_str("                                                 ", 5);
}

//////////////////////////////////////////////////////////////////////////////

void timer0_handler()
{
   if (!timer_get())
      return;

   //if (!((VDP1_REG_EDSR >> 1) & 1))
   //{
   //   vd1_manual.draw_incomplete = 1;
   //}
   //else
   {
      VDP1_REG_TVMR = 1 << 3;
      VDP1_REG_FBCR = 3;
      VDP1_REG_PTMR = 2;
      draw_next_frame(1);
      timer_set(0);
   }

   if (!vd1_manual.write_to_screen)
      return;

   char intstr[64] = { 0 };
   sprintf(intstr, "timer 0                    ");
   vdp1_manual_write_str(intstr, 5);
   
}

//////////////////////////////////////////////////////////////////////////////

void draw_end_handler()
{
   vd1_manual.draw_end++;

   if (!vd1_manual.write_to_screen)
      return;

   char intstr[64] = { 0 };

   sprintf(intstr, "Draw End Interrupt #%d                     ", vd1_manual.draw_end);
   vdp1_manual_write_str(intstr, 5);
   //vdp1_print_status();
   vdp1_manual_write_str("                                                 ", 5);
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_manual_test()
{
   vd1_manual.dir = 1;
   vd1_manual.write_to_screen = 0;
   vdp2_basic_tile_scroll_setup(0x40000);

   //VDP1_REG_EWDR = 0;
   //VDP1_REG_EWLR = 0;
   //VDP1_REG_EWRR = (320 << 9) | 224;

   interrupt_set_level_mask(0xF);

   bios_change_scu_interrupt_mask(0xFFFFFFFF, (MASK_VBLANKIN | MASK_VBLANKOUT | MASK_TIMER0 | MASK_DRAWEND));

   bios_set_scu_interrupt(0x40, vblank_in_handler);
   bios_set_scu_interrupt(0x41, vblank_out_handler);
   bios_set_scu_interrupt(0x43, timer0_handler);

   //SCUREG_T1MD = 0x001;
   //SCUREG_T0C = 223;

   bios_set_scu_interrupt(0x4D, draw_end_handler);

   bios_change_scu_interrupt_mask(~(MASK_VBLANKIN | MASK_VBLANKOUT | MASK_TIMER0 | MASK_DRAWEND), 0);

   interrupt_set_level_mask(0x1);

   int scroll = 0;

   vd1_manual.write_to_screen = 0;

   vdp1_manual_reset_state();

   draw_next_frame(1);

   for (;;)
   {
#if 0
      vdp_wait_vblankin();

      timer_set(1);

      vdp_wait_vblankout();

      set_ptmr1();

      vdp_wait_vblankin();
      vdp_wait_vblankout();

      vdp_wait_vblankin();
      vdp_wait_vblankout();
#endif
#if 0
      VDP2_REG_SCYIN0 = scroll;

      if (per[0].but_push & PAD_UP)
      {
         scroll--;

         if (scroll < 0)
            scroll = 0;
      }
      if (per[0].but_push & PAD_DOWN)
      {
         scroll++;
      }
#endif
   }
}
#else


struct {
   volatile int draw_end;
   volatile int vblank_in;
   volatile int vblank_out;

   volatile int cur_line;
   volatile int write_to_screen;

   volatile int x, dir;

   volatile int frames_delayed;

   volatile int state;

   volatile int hblank_timer;
}vd1_manual;

#define DRAW_IDLE 0
#define DRAW_START_PLOTTING 1
#define DRAW_DELAY_FRAMES 2
#define DRAW_CHECK_IF_FINISHED 3
#define DRAW_WRITE_2 4

void vdp1_manual_reset_state()
{
   vd1_manual.draw_end = 0;
   vd1_manual.vblank_in = 0;
   vd1_manual.vblank_out = 0;

   vd1_manual.cur_line = 0;
   vd1_manual.write_to_screen = 0;

   vd1_manual.x = 0;
   vd1_manual.dir = 1;

   vd1_manual.frames_delayed = 0;

   vd1_manual.state = DRAW_IDLE;

   //vd1_manual.hblank_timer = 0;
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_manual_write_str(char * str, int palette)
{
   vd1_manual.cur_line++;

   if (vd1_manual.cur_line > 31)
      return;

   write_str_as_pattern_name_data(0, vd1_manual.cur_line, str, palette, 0x000000, 0x40000);
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_print_status()
{
   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };

   int ptmr = (VDP1_REG_MODR >> 8) & 1;
   int fcm = (VDP1_REG_MODR >> 4) & 1;
   int vbe = (VDP1_REG_MODR >> 3) & 1;
   int cef = (VDP1_REG_EDSR >> 1) & 1;
   int bef = VDP1_REG_EDSR & 1;

   sprintf(str, "PTMR=%d FCM=%d VBE=%d CEF=%d BEF=%d           ", ptmr, fcm, vbe, cef, bef);
   vdp1_manual_write_str(str, 3);
}

//////////////////////////////////////////////////////////////////////////////
#if 0
void set_vbe(int vbe)
{
   VDP1_REG_TVMR = vbe << 3;

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };
   sprintf(str, "Setting VBE=%d                                 ", vbe);
   vdp1_manual_write_str(str, 4);

   vdp1_print_status();
}

//////////////////////////////////////////////////////////////////////////////

void set_fbcr(int fcm, int fct)
{
   VDP1_REG_FBCR = (fcm << 1) | fct;

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };
   sprintf(str, "Setting FCM=%d FCT=%d                          ", fcm, fct);
   vdp1_manual_write_str(str, 4);

   vdp1_print_status();
}

//////////////////////////////////////////////////////////////////////////////

void set_ptmr(int ptmr)
{
   VDP1_REG_PTMR = ptmr;

   if (!vd1_manual.write_to_screen)
      return;

   char str[64] = { 0 };
   sprintf(str, "Setting PTMR=%d                                ", ptmr);
   vdp1_manual_write_str(str, 4);

   vdp1_print_status();
}
#endif
//////////////////////////////////////////////////////////////////////////////

void vdp1_manual_draw_list(int x, int y)
{
   vdp_start_draw_list();

   sprite_struct localcoord;

   localcoord.attr = 0;
   localcoord.x = 0;
   localcoord.y = 0;

   vdp_local_coordinate(&localcoord);

   sprite_struct quad = { 0 };

   int size = 7;

   int top_right_x = x;
   int top_right_y = y;
   quad.x = top_right_x + size;
   quad.y = top_right_y;
   quad.x2 = top_right_x + size;
   quad.y2 = top_right_y + size;
   quad.x3 = top_right_x;
   quad.y3 = top_right_y + size;
   quad.x4 = top_right_x;
   quad.y4 = top_right_y;
   quad.bank = RGB16(0x10, 0x10, 0x10);
   vdp_draw_polygon(&quad);

   vdp_end_draw_list();
}

//////////////////////////////////////////////////////////////////////////////

void vblank_in_handler()
{
   if (vd1_manual.write_to_screen)
   {
      char str[64] = { 0 };

      sprintf(str, "VBlank In Interrupt #%d                     ", vd1_manual.vblank_in);
      vdp1_manual_write_str(str, 5);
      vdp1_print_status();
      //vdp1_manual_write_str("                                                 ", 5);
   }

   if (vd1_manual.state != DRAW_IDLE)
   {
      if (vd1_manual.state == DRAW_CHECK_IF_FINISHED)
      {
         vdp1_manual_write_str("EDSR Read", 5);


         //erase and change

         if (((VDP1_REG_EDSR >> 1) & 1) == 0)
         {
            vdp1_manual_write_str("TVMR=8", 5);
            vdp1_manual_write_str("FBCR=3", 5);
            vdp1_manual_write_str("PTMR=2", 5);
            VDP1_REG_TVMR = 1 << 3;
            VDP1_REG_FBCR = 3;
            VDP1_REG_PTMR = 2;
            vd1_manual.state = DRAW_START_PLOTTING;
            //   vdp1_manual_write_str("223 DRAW_START_PLOTTING set", 5);
         }
         // else
               {
                  //   vdp1_manual_write_str("223 cef not 0", 5);
               }
      }
   }


   vd1_manual.vblank_in++;
#if 0

#endif

}

//////////////////////////////////////////////////////////////////////////////

void vblank_out_handler()
{
   // Issue Intback command(we only want peripheral data)
   smpc_wait_till_ready();
   SMPC_REG_IREG(0) = 0x00; // no intback status
   SMPC_REG_IREG(1) = 0x0A; // 15-byte mode, peripheral data returned, time optimized
   SMPC_REG_IREG(2) = 0xF0; // ???
   smpc_issue_command(SMPC_CMD_INTBACK);

   static int scroll = 0;

   vd1_manual.hblank_timer = 0;

   if (vd1_manual.write_to_screen == 1)
   {
      char intstr[64] = { 0 };

      sprintf(intstr, "VBlank Out Interrupt #%d                     ", vd1_manual.vblank_out);
      vdp1_manual_write_str(intstr, 5);
      vdp1_print_status();
   }

   vd1_manual.vblank_out++;

//   vd1_manual.frames_delayed--;

   if (vd1_manual.state != DRAW_IDLE)
   {
      if (vd1_manual.state == DRAW_START_PLOTTING)
      {
       //  vdp1_manual_write_str("vblank_out_handler DRAW_START_PLOTTING", 5);
         vdp1_manual_write_str("TVMR=0", 5);
         vdp1_manual_write_str("PTMR=1", 5);
         VDP1_REG_TVMR = 0 << 3;
         VDP1_REG_PTMR = 1;
         vd1_manual.state = DRAW_DELAY_FRAMES;
         vd1_manual.frames_delayed = 0;
       //  vdp1_manual_write_str("vblank_out_handler->DRAW_DELAY_FRAMES", 5);
      }
      else if (vd1_manual.state == DRAW_DELAY_FRAMES)
      {

         if (vd1_manual.frames_delayed > 0)
         {
            vd1_manual.state = DRAW_CHECK_IF_FINISHED;
            //vdp1_manual_write_str("vblank_in_handler DRAW_CHECK_IF_FINISHED set", 5);
         }

         vd1_manual.frames_delayed++;
         //  vdp1_manual_write_str("vblank_out_handler frames_delayed++", 5);


      }
   }

   if (vd1_manual.x > 319 - 8)
   {
      vd1_manual.dir = -vd1_manual.dir;
   }

   if (vd1_manual.x < 0)
   {
      vd1_manual.dir = -vd1_manual.dir;
   }

   vd1_manual.x += vd1_manual.dir;

   //vdp1_manual_draw_list(vd1_manual.x, 0);

   if (per[0].but_push_once & PAD_A)
   {
      vdp1_manual_reset_state();
      //         test_finished = 1;
      vdp1_manual_write_str("END                                                            ", 4);
      vdp1_manual_write_str("                                                               ", 4);
      vdp1_manual_write_str("                                                               ", 4);
   }

   if (per[0].but_push_once & PAD_START)
   {
      void(*ar)() = (void(*)())0x02000100;

      ar();
   }

   VDP2_REG_SCYIN0 = scroll;

   if (per[0].but_push & PAD_UP)
   {
      scroll--;

      if (scroll < 0)
         scroll = 0;
   }
   if (per[0].but_push & PAD_DOWN)
   {
      scroll++;
   }

   if (!vd1_manual.write_to_screen)
      return;
}

void hblank_in_handler()
{
   vd1_manual.hblank_timer++;

   if (vd1_manual.hblank_timer == 225)
   {
      vd1_manual.hblank_timer = 0;

      //vdp1_manual_write_str("hit!!!", 5);
   }
}

void prepare_draw()
{
//   vd1_manual.check_if_draw_finished = 0;
//   vd1_manual.need_draw = 1;
//   vd1_manual.draw_incomplete = 0;
//   vd1_manual.frames_delayed = 0;
}

//////////////////////////////////////////////////////////////////////////////

void timer0_handler()
{
#if 0
   return;

   if (vd1_manual.write_to_screen)
   {
      char intstr[64] = { 0 };
      sprintf(intstr, "timer 0                    ");
      vdp1_manual_write_str(intstr, 5);
   }
   if (vd1_manual.state != DRAW_IDLE)
   {
      if (vd1_manual.state == DRAW_CHECK_IF_FINISHED)
      {
         //vdp1_manual_write_str("timer0_handler DRAW_CHECK_IF_FINISHED", 5);

         if (((VDP1_REG_EDSR >> 1) & 1) == 0)
         {
            vdp1_manual_write_str("TVMR=8", 5);
            vdp1_manual_write_str("FBCR=3", 5);
            vdp1_manual_write_str("PTMR=2", 5);
            VDP1_REG_TVMR = 1 << 3;
            VDP1_REG_FBCR = 3;
            VDP1_REG_PTMR = 2;
            vd1_manual.state = DRAW_START_PLOTTING;
           // vdp1_manual_write_str("timer0_handler DRAW_START_PLOTTING set", 5);
         }
         else
         {
            //vdp1_manual_write_str("cef not 0", 5);
         }
      }
   }
#endif
}

//////////////////////////////////////////////////////////////////////////////

void draw_end_handler()
{
   vd1_manual.draw_end++;

   if (!vd1_manual.write_to_screen)
      return;

   char intstr[64] = { 0 };

   sprintf(intstr, "Draw End Interrupt #%d                     ", vd1_manual.draw_end);
   vdp1_manual_write_str(intstr, 5);
   //vdp1_print_status();
}

//////////////////////////////////////////////////////////////////////////////

void vdp1_manual_test()
{
   vd1_manual.dir = 1;
   vd1_manual.write_to_screen = 0;
   vdp2_basic_tile_scroll_setup(0x40000);
#if 0
   //reset it
   VDP1_REG_PTMR = 0;
   VDP1_REG_TVMR = 0;
   VDP1_REG_FBCR = 3;
   vdp_vsync();
   vdp_vsync();
   vdp_vsync();
#endif
   //VDP1_REG_EWDR = 0;
   //VDP1_REG_EWLR = 0;
   //VDP1_REG_EWRR = (320 << 9) | 224;

   interrupt_set_level_mask(0xF);

   bios_change_scu_interrupt_mask(0xFFFFFFFF, (MASK_VBLANKIN | MASK_VBLANKOUT | MASK_HBLANKIN | MASK_TIMER0 | MASK_DRAWEND));

   bios_set_scu_interrupt(0x40, vblank_in_handler);
   bios_set_scu_interrupt(0x41, vblank_out_handler);
   bios_set_scu_interrupt(0x42, hblank_in_handler);
   bios_set_scu_interrupt(0x43, timer0_handler);

   bios_set_scu_interrupt(0x4D, draw_end_handler);

   bios_change_scu_interrupt_mask(~(MASK_VBLANKIN | MASK_VBLANKOUT | MASK_HBLANKIN | MASK_TIMER0 | MASK_DRAWEND), 0);

   interrupt_set_level_mask(0x1);

   int scroll = 0;

   vd1_manual.write_to_screen = 0;

   vdp1_manual_reset_state();

//   draw_next_frame(1);

   //prepare_draw();

   //draw_start();

//   vd1_manual.begin_drawing = 1;
//   vd1_manual.started = 1;
//   vd1_manual.check_if_draw_finished = 0;
//   vd1_manual.frames_delayed = 0;

   vd1_manual.state = DRAW_START_PLOTTING;
   vd1_manual.write_to_screen = 1;

   for (;;)
   {
      //vdp1_manual_draw_list(vd1_manual.x, 0);

#if 0
      vdp_wait_vblankin();

      timer_set(1);

      vdp_wait_vblankout();

      set_ptmr1();

      vdp_wait_vblankin();
      vdp_wait_vblankout();

      vdp_wait_vblankin();
      vdp_wait_vblankout();
#endif
#if 1

#endif
   }
}
#endif
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

void vdp1_framebuffer_tests()
{
   auto_test_section_start("Vdp1 framebuffer tests");

   vdp1_clip_test();

   auto_test_section_end();
}

//////////////////////////////////////////////////////////////////////////////

volatile int vdp1_manual_lines_since_vblank_in = 0;
volatile int vdp1_manual_lines_since_vblank_out = 0;
volatile int need_draw = 0;
volatile int wait_counter = 0;
volatile int frames_to_wait = 3;

volatile int stored_edsr = 0;
volatile int need_update = 0;

struct
{
   int since_vblank_in;
   int since_vblank_out;
   int edsr;
   int frame;
}manual_results[64] = { 0 };

volatile int manual_results_pos = 0;
volatile int manual_frame_count = 0;

void vdp1_manual_vblank_in_handler()
{
   vdp1_manual_lines_since_vblank_in = 0;

   if (need_draw == 1)
   {
      //change
      VDP1_REG_TVMR = 1 << 3;
      VDP1_REG_FBCR = 3;
      need_draw = 0;
   }
   else
   {
      wait_counter++;

      if (wait_counter > frames_to_wait)
      {
         wait_counter = 0;
         need_draw = 1;
      }
   }
}

void vdp1_manual_vblank_out_handler()
{
   vdp1_manual_lines_since_vblank_out = 0;

   manual_frame_count++;

   if (need_draw == 0)
      VDP1_REG_TVMR = 0 << 3;

   smpc_wait_till_ready();
   SMPC_REG_IREG(0) = 0x00; // no intback status
   SMPC_REG_IREG(1) = 0x0A; // 15-byte mode, peripheral data returned, time optimized
   SMPC_REG_IREG(2) = 0xF0; // ???
   smpc_issue_command(SMPC_CMD_INTBACK);

   need_update = 1;
}

void vdp1_manual_hblank_in_handler()
{
   vdp1_manual_lines_since_vblank_in++;
   vdp1_manual_lines_since_vblank_out++;

}

void vdp1_manual_draw_end_handler()
{

}

void vdp1_manual_2()
{
   VDP1_REG_TVMR = 1 << 3;
   VDP1_REG_FBCR = 3;
   VDP1_REG_PTMR = 2;

   interrupt_set_level_mask(0xF);
   bios_change_scu_interrupt_mask(0xFFFFFFFF, (MASK_VBLANKIN | MASK_VBLANKOUT | MASK_HBLANKIN | MASK_DRAWEND));
   bios_set_scu_interrupt(0x40, vdp1_manual_vblank_in_handler);
   bios_set_scu_interrupt(0x41, vdp1_manual_vblank_out_handler);
   bios_set_scu_interrupt(0x42, vdp1_manual_hblank_in_handler);
   bios_set_scu_interrupt(0x4D, vdp1_manual_draw_end_handler);
   bios_change_scu_interrupt_mask(~(MASK_VBLANKIN | MASK_VBLANKOUT | MASK_HBLANKIN | MASK_DRAWEND), 0);

   interrupt_set_level_mask(0x1);

   int dir = 4;
   int x = 8;
   int y = 8;

   for (;;)
   {
      if (need_update)
      {
         need_update = 0;

         x += dir;

         if (x > (320 - 8))
            dir = -4;
         else if (x < (8))
            dir = 4;

         vdp1_manual_draw_list(x, y);
      }

      if (VDP1_REG_EDSR != stored_edsr)
      {
         manual_results[manual_results_pos].since_vblank_out = vdp1_manual_lines_since_vblank_out;
         manual_results[manual_results_pos].since_vblank_in = vdp1_manual_lines_since_vblank_in;
         manual_results[manual_results_pos].edsr = VDP1_REG_EDSR;
         manual_results[manual_results_pos].frame = manual_frame_count;
         stored_edsr = VDP1_REG_EDSR;
         manual_results_pos++;
      }

      if (manual_results_pos > 62)
         break;
   }

   int i;

   for (i = 0; i < 32; i++)
   {
      vdp_printf(&test_disp_font, 0, 8 * i, 15,
         "%03d | %03d | %04d | %04d",
         manual_results[i].edsr,
         manual_results[i].since_vblank_out,
         manual_results[i].since_vblank_in,
         manual_results[i].frame);
   }

   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}



void analyze_test1()
{
   volatile u16 *p = (volatile u16 *)(VDP2_RAM);

   for (;;)
   {
      p[0] = 0xc;
      p[1] = 0xd;
      p[2] = 0xe;
      p[3] = 0xf;

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}

volatile int finished = 0;
volatile int interrupt_error = 0;

int num_interrupts = 0;

void dma_intr(void)
{
   if (finished == 1)
      interrupt_error = 1;

   finished = 1;

   num_interrupts += 1;
}

const u32 vdp2_vram = 0x25E00000 + 0x00010008;
const u32 scsp_ram = 0x25a00000 + 0x00010008;
const u32 vdp1_ram_addr = 0x25C00000 + 0x00010008;
const u32 scsp_regs = 0x25a10000;

int dma_length = 0x8000;

#define SH2_FTCSR (*(volatile u8 *)0xFFFFFE11)
#define SH2_FRCH (*(volatile u8 *)0xFFFFFE12)
#define SH2_FRCL (*(volatile u8 *)0xFFFFFE13)
#define SH2_TCR (*(volatile u8 *)0xFFFFFE16)

void frc_clear()
{
   SH2_FRCH = 0;
   SH2_FRCL = 0;
}

int frc_get()
{
   u8 frch = SH2_FRCH;
   u8 frcl = SH2_FRCL;
   return frch << 8 | frcl;
}

int dma_print_pos = 0;

void dma_interrupt_setup()
{
   bios_set_scu_interrupt(0x4B, dma_intr);
   bios_change_scu_interrupt_mask(~MASK_DMA0, 0);
   interrupt_set_level_mask(0x4);
}

void do_dma(u32 src_addr, u32 dst_addr, u32 read_add, u32 write_add, u32 length, u32 print_result, u32 factor)
{
   finished = 0;

   SCUREG_D0EN = 0;
   SCUREG_D0R = src_addr;
   SCUREG_D0W = dst_addr;
   SCUREG_D0C = length;
   SCUREG_D0AD = (read_add << 8) | write_add;
   SCUREG_D0MD = factor;

   frc_clear();

   SCUREG_D0EN = 0x101;

   while (!finished) {}

   u32 endtime = frc_get();

   if (print_result)
      vdp_printf(&test_disp_font, 0 * 8, dma_print_pos * 8, 0xF, "frc: %d (~%d cycles)", endtime, endtime * 8);

   dma_print_pos++;
}

struct DmaStruct
{
   u32 source_addr;
   u32 dest_addr;
   u32 length;
   u32 read_add;
   u32 write_add;
   u32 factor;
   u32 enable;
   u32 level;
};

void do_dma_multiple(
   u32 *src_addrs, 
   u32 *dst_addrs, 
   u32 read_add,
   u32 write_add, 
   u32 length, 
   u32 print_result, 
   u32 factor,
   u32 num_dmas)
{
   finished = 0;

   int i;

   SCUREG_D0EN = 0;

   for (i = 0; i < num_dmas; i++)
   {
      SCUREG_D0R = src_addrs[i];
      SCUREG_D0W = dst_addrs[i];
      SCUREG_D0C = length;
      SCUREG_D0AD = (read_add << 8) | write_add;
      SCUREG_D0MD = factor;
      SCUREG_D0EN = 0x101;
   }

   frc_clear();

   while (!finished) {}

   u32 endtime = frc_get();

   if (print_result)
      vdp_printf(&test_disp_font, 0 * 8, dma_print_pos * 8, 0xF, "frc: %d (~%d cycles)", endtime, endtime * 8);

   dma_print_pos++;
}


void do_dma_multiple_struct(
   struct DmaStruct * dma_struct,
   u32 print_result,
   u32 num_dmas)
{
   finished = 0;

   int i;

   SCUREG_D0EN = 0;

   for (i = 0; i < num_dmas; i++)
   {
      u32 level = dma_struct[i].level;

      if (level == 0)
      {
         SCUREG_D0R = dma_struct[i].source_addr;
         SCUREG_D0W = dma_struct[i].dest_addr;
         SCUREG_D0C = dma_struct[i].length;
         SCUREG_D0AD = (dma_struct[i].read_add << 8) | dma_struct[i].write_add;
         SCUREG_D0MD = dma_struct[i].factor;
         SCUREG_D0EN = dma_struct[i].enable;
      }
      else if (level == 1)
      {
         SCUREG_D1R = dma_struct[i].source_addr;
         SCUREG_D1W = dma_struct[i].dest_addr;
         SCUREG_D1C = dma_struct[i].length;
         SCUREG_D1AD = (dma_struct[i].read_add << 8) | dma_struct[i].write_add;
         SCUREG_D1MD = dma_struct[i].factor;
         SCUREG_D1EN = dma_struct[i].enable;
      }
      else if (level == 2)
      {
         SCUREG_D2R = dma_struct[i].source_addr;
         SCUREG_D2W = dma_struct[i].dest_addr;
         SCUREG_D2C = dma_struct[i].length;
         SCUREG_D2AD = (dma_struct[i].read_add << 8) | dma_struct[i].write_add;
         SCUREG_D2MD = dma_struct[i].factor;
         SCUREG_D2EN = dma_struct[i].enable;
      }
   }

   frc_clear();

   while (!finished) {}

   u32 endtime = frc_get();

   if (print_result)
      vdp_printf(&test_disp_font, 0 * 8, dma_print_pos * 8, 0xF, "frc: %d (~%d cycles)", endtime, endtime * 8);

   dma_print_pos++;
}

void do_double_dma(
   u32 src_addr, u32 dst_addr, u32 read_add, u32 write_add, u32 length, 
   u32 src_addr2, u32 dst_addr2, u32 read_add2, u32 write_add2, u32 length2,
   u32 result, u32 do_double
   )
{
   bios_set_scu_interrupt(0x4B, dma_intr);
   bios_change_scu_interrupt_mask(~MASK_DMA0, 0);

   finished = 0;

   SCUREG_D0R = src_addr;
   SCUREG_D0W = dst_addr;
   SCUREG_D0C = length;
   SCUREG_D0AD = (read_add << 8) | write_add;
   SCUREG_D0MD = 0x00000007;
   SCUREG_D0EN = 0x101;

   if (do_double)
   {
      SCUREG_D0R = src_addr2;
      SCUREG_D0W = dst_addr2;
      SCUREG_D0C = length2;
      SCUREG_D0AD = (read_add2 << 8) | write_add2;
      SCUREG_D0MD = 0x00000007;
      SCUREG_D0EN = 0x101;
   }

   interrupt_set_level_mask(0x4);

   u32 starttime = timer_counter();

   while (!finished) {}

   u32 endtime = timer_counter();

   u32 total = endtime - starttime;

   if (result)
      vdp_printf(&test_disp_font, 20 * 8, 20 * 8, 0xF, "frc: %d (~%d cycles)", total, total*8);
}

void erase_vdp2()
{
   
   volatile u32 *p = (volatile u32 *)(0x260F0000);
   p[0] = 0;
   do_dma(0x260F0000, 0x25E00000, 0, 1,320*224*3, 0, 7);
}

void scu_dma_memset(u32 destination, u32 length)
{

   volatile u32 *p = (volatile u32 *)(0x260F0000);
   p[0] = 0;
   do_dma(0x260F0000, destination, 0, 1, length, 0, 7);
}

void do_analyze_test(u32 src_addr, u32 dst_addr, u32 read_add, u32 write_add, char* test_name, u32 factor)//
{
   volatile u32 *source_ptr = (volatile u32 *)(src_addr);
   volatile u32 *dest_ptr = (volatile u32 *)(dst_addr);

   erase_vdp2();

   vdp_printf(&test_disp_font, 14 * 8, 25 * 8, 0xF, "%s", test_name);

   int i;

   for (i = 0; i < (dma_length/4); i += 4)
   {
      source_ptr[i+0] = 0x0001000f;
      source_ptr[i+1] = 0x0002000a;
      source_ptr[i+2] = 0x0003000c;
      source_ptr[i+3] = 0x0004000e;
   }

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 24 * 8, i * 8, 0xF, "%08X", source_ptr[i]);
   }

//   for (i = 0; i < 1000; i++)
   {
      //int j;
      //for (j = 0; j < 1024; j++)
      //{
      //   dest_ptr[j] = 0x7;
      //}

      do_dma(src_addr, dst_addr, read_add, write_add, dma_length, 1, factor);
   }

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 0 * 8, (i+1) * 8, 0xF, "%08X", dest_ptr[i]);
   }

   for (i = 0; i < 24; i++)
   {
      vdp_printf(&test_disp_font, 12 * 8, (i + 1) * 8, 0xF, "%08X", dest_ptr[i + ((dma_length/4) - 16)]);
   }

   int q = 0;

   int j = 0;

   for (;;)
   {
      vdp_vsync();

      //volatile u8 * ptr = (volatile u8 *)(0x25E00000);

      //for (i = 0; i < 320; i++)
      //{
      //   for (j = (8 * 27); j < 8 * 28; j++)
      //   {
      //      ptr[(j * 512) + i] = 0;
      //   }
      //}

      q++;

      source_ptr[0] = q;

      char null_str[8] = { 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa };

      vdp_printf(&test_disp_font, 0 * 8, (26) * 8, 1, null_str, dest_ptr[0]);

      vdp_printf(&test_disp_font, 0 * 8, (26) * 8, 0xF, "%08X", dest_ptr[0]);

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_B)
      {
         SCUREG_D0R = 0;
      }

      if (per[0].but_push_once & PAD_C)
      {
         SCUREG_D0W = 0;
      }

      if (per[0].but_push_once & PAD_X)
      {
         SCUREG_D0C = 0;
      }

      if (per[0].but_push_once & PAD_Z)
      {
         SCUREG_D0MD = 0x00000007;
      }

      if (per[0].but_push_once & PAD_L)
      {
         SCUREG_D0AD = (0 << 8) | 0;
      }

      

      
      
      
      
      
//      SCUREG_D0EN = 0x101;

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}

void memory_setup(u32 addr1, u32 addr2, u32 length)
{
   int i;

   volatile u32 *dest_1 = (volatile u32*)(addr1);
   volatile u32 *dest_2 = (volatile u32*)(addr2);

   for (i = 0; i < (length / 4); i += 4)
   {
      dest_1[i + 0] = 0x00000008;
      dest_1[i + 1] = 0x00010009;
      dest_1[i + 2] = 0x0002000a;
      dest_1[i + 3] = 0x0003000b;
   }

   for (i = 0; i < (length / 4); i += 4)
   {
      dest_2[i + 0] = 0x0004000c;
      dest_2[i + 1] = 0x0005000d;
      dest_2[i + 2] = 0x0006000e;
      dest_2[i + 3] = 0x0007000f;
   }
}

void finish_loop()
{
   for (;;)
   {
      vdp_vsync();

      if (interrupt_error)
      {
         vdp_printf(&test_disp_font, 0 * 8, 0 * 8, 0xF, "error");
      }

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}

void double_dma_test()
{
   int length = 0x1000;

   int i;

//   volatile u32 *source_ptr = (volatile u32 *)(0x260F0000);
   volatile u32 *dest_ptr = (volatile u32 *)(0x25E00000);

//   volatile u32 *source_ptr_len = (volatile u32 *)(0x260F2000);
   volatile u32 *dest_ptr_len = (volatile u32 *)(0x25E02000);

   memory_setup(0x260F0000, 0x260F2000, length);


   for (i = 0; i < 20000; i++)
   {
      do_double_dma(
         0x260F0000, 0x25E00000, 1, 1, length,
         0x260F2000, 0x25E02000, 1, 1, length,
         1, 1);

      vdp_vsync();
   }


   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 0 * 8, (i + 1) * 8, 0xF, "%08X", dest_ptr[i]);
   }

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 12 * 8, (i + 1) * 8, 0xF, "%08X", dest_ptr_len[i]);
   }

   finish_loop();

}


//const u32 vdp2_vram = 0x25E00000 + 0x00010008;
//const u32 scsp_ram = 0x25a00000 + 0x00010008;
//const u32 vdp1_ram_addr = 0x25C00000 + 0x00010008;
//const u32 scsp_regs = 0x25a10000;

void indirect_dma_test()
{
   int length = 0x1000;

   int i;

//   volatile u32 *source_ptr = (volatile u32 *)(0x260F0000);
   volatile u32 *dest_ptr = (volatile u32 *)(0x25E00000);

//   volatile u32 *source_ptr_len = (volatile u32 *)(0x260F2000);
   volatile u32 *dest_ptr_len = (volatile u32 *)(0x25E02000);

   memory_setup(0x260F0000, 0x260F2000, length);

   u32 indirect_table_addr = 0x260FF000;

   volatile u32 *indirect_table = (volatile u32 *)(indirect_table_addr);

   indirect_table[0] = length;
   indirect_table[1] = 0x25E00000;
   indirect_table[2] = 0x260F0000;

   indirect_table[3] = length;
   indirect_table[4] = 0x25E02000;
   indirect_table[5] = 0x260F2000 | 0x80000000;

   bios_set_scu_interrupt(0x4B, dma_intr);
   bios_change_scu_interrupt_mask(~MASK_DMA0, 0);
   finished = 0;

   SCUREG_D0W = indirect_table_addr;
   SCUREG_D0AD = 0x101;
   SCUREG_D0MD = (1 << 24) | 0x00000007;
   SCUREG_D0EN = 0x101;

   interrupt_set_level_mask(0x4);

   while (!finished) {}

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 0 * 8, (i + 4) * 8, 0xF, "%08X", dest_ptr[i]);
   }

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 12 * 8, (i + 4) * 8, 0xF, "%08X", dest_ptr_len[i]);
   }

   finish_loop();
}

void dma_update_test(u32 write_address_update, u32 read_address_update, u32 write_add, u32 read_add)
{

   int i;

   u32 src_addr = 0x260F0000;
   u32 dest_addr = 0x25E00000;

   int length = 0x1000;

   volatile u32 *dest_ptr = (volatile u32 *)(dest_addr);
   volatile u32 *dest_ptr2 = (volatile u32 *)(dest_addr + length);

   erase_vdp2();

   memory_setup(src_addr, src_addr + length, length);

   bios_set_scu_interrupt(0x4B, dma_intr);
   bios_change_scu_interrupt_mask(~MASK_DMA0, 0);

   SCUREG_D0R = src_addr;
   SCUREG_D0W = dest_addr;
   SCUREG_D0C = length;
   SCUREG_D0AD = (read_add << 8) | write_add;
   SCUREG_D0MD = 0x00000007 | (write_address_update << 8) | (read_address_update << 16);
   SCUREG_D0EN = 0x101;

   interrupt_set_level_mask(0x4);

   finished = 0;

   while (!finished) {}

   volatile u32 *ack = (volatile u32 *)(0x25fe00a4);

   ack[0] = 0;

   SCUREG_D0EN = 0x101;

   finished = 0;

   //dma occurs, but interrupt doesn't fire?
//   while (!finished) {}

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 0 * 8, (i + 4) * 8, 0xF, "%08X", dest_ptr[i]);
   }

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, 12 * 8, (i + 4) * 8, 0xF, "%08X", dest_ptr2[i]);
   }

   finish_loop();
}

void print_data(u32 x_start, u32 y_start, u32 addr, u32 length, char * str)
{
   volatile u32 *ptr = (volatile u32 *)(addr);
   int i;

   vdp_printf(&test_disp_font, x_start * 8, y_start * 8, 0xF, str);

   vdp_printf(&test_disp_font, x_start * 8, (y_start+1) * 8, 0xF, "start");

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, x_start * 8, (i + y_start + 2) * 8, 0xF, "%08X", ptr[i]);
   }

   vdp_printf(&test_disp_font, (x_start + 9) * 8, (y_start+1) * 8, 0xF, "end");

   for (i = 0; i < 16; i++)
   {
      vdp_printf(&test_disp_font, (x_start + 9) * 8, (i + y_start + 2) * 8, 0xF, "%08X", ptr[i + ((length / 4) - 16)]);
   }
}

void clear_all(u32 length)
{
   scu_dma_memset(0x25E00000, 320 * 224 * 2);
   scu_dma_memset(0x25a00000, length);
   scu_dma_memset(0x25C00000, length);
}

void test_boilerplate(u32 src_addr, u32 length, char* test_name)
{
   clear_all(length);

   memory_setup(src_addr, src_addr + length, length);

   vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "%s", test_name);

   //source data
   print_data(0, 3, src_addr, length, "source");

   dma_print_pos = 22;
}

void test_direct_dma(u32 src_addr, u32 dst_addr, u32 read_add, u32 write_add, u32 factor, u32 length, char* test_name)
{
   test_boilerplate(src_addr, length, test_name);

   int i;
   for (i = 0; i < 5; i++)
   {
      //vdp2 dma takes much longer during active display
      vdp_wait_vblankin();
      do_dma(src_addr, dst_addr, read_add, write_add, length, 1, factor);
   }

   //destination data
   print_data(18, 3, dst_addr, length, "destination");

   finish_loop();
}

void test_multiple_direct_dma(u32 dst_addr, u32 read_add, u32 write_add, char* test_name, u32 num_dmas)
{
   u32 length = 0x800;
   clear_all(length);

   vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "%s", test_name);

   //do_dma_multiple_struct

   u32 src_addrs[32] = { 0 };
   u32 dst_addrs[32] = { 0 };
   u32 data[32] = { 0 };

   struct DmaStruct dma_structs[16] = { { 0 } };

   int i;

   int q = 0;

   for (i = 0; i < num_dmas; i++)
   {
      dma_structs[i].source_addr = 0x260F0000 + length*i;
      dma_structs[i].dest_addr = 0x25E00000 + length*i;
      dma_structs[i].length = length;
      dma_structs[i].read_add = read_add;
      dma_structs[i].write_add = write_add;
      dma_structs[i].factor = 7;
      dma_structs[i].enable = 0x101;
      dma_structs[i].level = 1;

      src_addrs[i] = 0x260F0000 + length*i;
      dst_addrs[i] = 0x25E00000 + length*i;
      data[i] = 0xcafe0000 + i;
   }

   //source data
   print_data(0, 3, src_addrs[0], length, "source");

   dma_print_pos = 22;
   
   int j;

   for (i = 0; i < num_dmas; i++)
   {
      volatile u32 *dst = (volatile u32 *)(src_addrs[i]);

      for (j = 0; j < (length / 4); j += 4)
      {
         dst[j + 0] = data[i];
         dst[j + 1] = data[i];
         dst[j + 2] = data[i];
         dst[j + 3] = data[i];
      }
   }

   num_interrupts = 0;

   vdp_wait_vblankin();
   //do_dma_multiple(src_addrs, dst_addrs, read_add, write_add, length, 1, 7, num_dmas);

   do_dma_multiple_struct(dma_structs, 1, num_dmas);

   for (i = 0; i < num_dmas; i++)
   {
      volatile u32 *ptr = (volatile u32 *)(dst_addrs[i]);
      u32 x_start = 18;
      u32 y_start = 3;
      vdp_printf(&test_disp_font, (x_start + 9) * 8, (i + y_start + 2) * 8, 0xF, "%08X", ptr[(length / 4) - 1]);
   }

   vdp_printf(&test_disp_font, 18 * 8, 27 * 8, 0xF, "%08X", num_interrupts);

   //destination data
   //print_data(18, 3, dst_addr, length, "destination");

   finish_loop();
}

void test_all_direct_dma()
{
   u32 high_wram_addr = 0x260F0000;
   u32 vdp2_vram_addr = 0x25E00000;
   u32 scsp_ram_addr = 0x25a00000;
   u32 vdp1_ram_addr = 0x25C00000;
   u32 vdp1_fb_addr = 0x25C80000;
   
   u32 length = 0x1000;
   
   int i;

   test_multiple_direct_dma(vdp2_vram_addr, 1, 1, "multiple dma %d", 1);

   for (i = 1; i < 17; i++)
   {
      test_multiple_direct_dma(vdp2_vram_addr, 1, 1, "multiple dma %d", i);
   }

   test_direct_dma(high_wram_addr, vdp2_vram_addr, 1, 1, 7, length, "wram to vdp2 vram");
   test_direct_dma(high_wram_addr, scsp_ram_addr, 1, 1, 7, length, "wram to scsp ram");
   test_direct_dma(high_wram_addr, vdp1_ram_addr, 1, 1, 7, length, "wram to vdp1 ram");

   vdp_vsync();
   VDP1_REG_FBCR = 3;
   vdp_vsync();

   test_direct_dma(high_wram_addr, vdp1_fb_addr, 1, 1, 7, length, "wram to vdp1 fb");

   test_direct_dma(vdp2_vram_addr, high_wram_addr, 1, 2, 7, length, "vdp2 vram to wram");
   test_direct_dma(scsp_ram_addr, high_wram_addr, 1, 2, 7, length, "scsp ram to wram");
   test_direct_dma(vdp1_ram_addr, high_wram_addr, 1, 2, 7, length, "vdp1 ram to wram");

   vdp_vsync();
   VDP1_REG_FBCR = 3;
   vdp_vsync();

   test_direct_dma(vdp1_fb_addr, high_wram_addr, 1, 2, 7, length, "vdp1 fb to wram");

   vdp_vsync();
   VDP1_REG_FBCR = 0;
   vdp_vsync();




}

void direct_dma_timing()
{

}

#define WTCSR (*(volatile u16 *)0xFFFFFE80)
#define WTCNT_W (*(volatile u16 *)0xFFFFFE80)
#define WTCNT_R (*(volatile u8 *)0xFFFFFE81)
#define RSTCSR_W (*(volatile u16 *)0XFFFFFE82)
#define RSTCSR_R (*(volatile u8 *)0XFFFFFE83)

#define VDP2_RAM_ADDR (*(volatile u32 *)0x25E00000)

#define WRITE_DEST(DESTINATION_ADDR) \
   *DESTINATION_ADDR = 1; \
   *DESTINATION_ADDR = 2; \
   *DESTINATION_ADDR = 3; \
   *DESTINATION_ADDR = 2;

void write_timing(u32 destination, char*test_name)
{
   clear_all(0x1000);

   u32 vdp2_vram_addr = 0x25E00000;
//#define WANT_32
#ifdef WANT_32
   volatile u32 *dest_ptr = (volatile u32 *)(destination);
#else
   volatile u16 *dest_ptr = (volatile u16 *)(destination);
#endif

   dma_print_pos = 10;

   int i;

   int num_writes = 16;

   for (i = 0; i < 5; i++)
   {
      vdp_wait_vblankin();

      //zero watchdog timer
      WTCNT_W = 0x5A00 | 0;
      RSTCSR_W = 0;

      //enable timer
      WTCSR = 0xA500 | (1 << 5);
      
      WRITE_DEST(dest_ptr);
      WRITE_DEST(dest_ptr);
      WRITE_DEST(dest_ptr);
      WRITE_DEST(dest_ptr);

      u8 end_time = WTCNT_R;

      //disable timer
      WTCSR = 0xA500 | (0 << 5);

      frc_clear();

      WRITE_DEST(dest_ptr);
      WRITE_DEST(dest_ptr);
      WRITE_DEST(dest_ptr);
      WRITE_DEST(dest_ptr);

      u32 endtime = frc_get();
      
      vdp_printf(&test_disp_font, 0 * 8, 1 * 8, 0xF, "%s",test_name);

      vdp_printf(&test_disp_font, 0 * 8, dma_print_pos * 8, 0xF, "frc: %d (~%d cycles), ~%d cycles per write", endtime, endtime * 8, (endtime * 8) / num_writes);
      vdp_printf(&test_disp_font, 0 * 8, (dma_print_pos + 6) * 8, 0xF, "wdt: %d (~%d cycles), ~%d cycles per write", end_time, end_time * 2, (end_time * 2) / num_writes);

      dma_print_pos++;
   }

   for (;;)
   {
      vdp_wait_vblankin();

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
   }
}


void analyze_test()
{
   //volatile u32 *source_ptr = (volatile u32 *)(0x260F0000);
   //volatile u32 *dest_ptr = (volatile u32 *)(dma_destination);

   u32 high_wram_addr = 0x260F0000;
   u32 vdp2_vram_addr = 0x25E00000;

   volatile u32 *dest_ptr = (volatile u32 *)(vdp2_vram_addr);
   volatile u16 *dest_ptr_16 = (volatile u16 *)(vdp2_vram_addr);

   SH2_TCR = 0;
   SH2_FTCSR = 0;

   dma_interrupt_setup();

   u32 low_workram = 0x20200000;
   u32 scsp_ram_addr = 0x25a00000;
   u32 vdp1_ram_addr = 0x25C00000;
   u32 vdp1_fb_addr = 0x25C80000;

   write_timing(vdp2_vram_addr, "to vdp2 vram");

   write_timing(scsp_ram_addr, "to scsp ram");

   write_timing(vdp1_ram_addr, "to vdp1 ram");

   write_timing(vdp1_fb_addr, "to vdp1 framebuffer");

   write_timing(low_workram, "to low work ram");

   write_timing(0x260FF000, "to high work ram");

   int i;

   for (;;)
   {
      vdp_wait_vblankin();

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }
#if 0
      for (i = 0; i < 2000; i += 8)
      {
         dest_ptr_16[0] = 1;//0,1
         dest_ptr_16[0] = 2;//2,3
         dest_ptr_16[0] = 3;
         dest_ptr_16[0] = 3;
         dest_ptr_16[0] = 1;
         dest_ptr_16[0] = 2;
         dest_ptr_16[0] = 3;
         dest_ptr_16[0] = 3;
      }

      for (i = 0; i < 2000; i += 8)
      {
         dest_ptr[0] = 1;//0,1
         dest_ptr[0] = 2;//2,3
         dest_ptr[0] = 3;
         dest_ptr[0] = 3;
         dest_ptr[0] = 1;
         dest_ptr[0] = 2;
         dest_ptr[0] = 3;
         dest_ptr[0] = 3;
      }
#endif
   }

#if 0
   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_A)
      {
         break;
      }

      if (per[0].but_push_once & PAD_Y)
      {
         reset_system();
      }

      frc_clear();

      erase_vdp2();

      int counter = frc_get();

      vdp_printf(&test_disp_font, 0 * 8, 0 * 8, 0xF, "%04X", counter);
   }
#endif

   test_all_direct_dma();
#if 0

   do_analyze_test(high_wram_addr, vdp2_vram_addr, 1, 1, "wram to vdp2 ", 0);

   do_analyze_test(high_wram_addr, vdp2_vram_addr, 1, 1, "wram to vdp2 ", 7);
   do_analyze_test(high_wram_addr, scsp_ram_addr, 1, 1, "wram to scsp ", 7);
   do_analyze_test(high_wram_addr, vdp1_ram_addr, 1, 1, "wram to vdp1 ", 7);

   do_analyze_test(vdp2_vram_addr, high_wram_addr, 1, 2, "vdp2 to wram ", 7);
   do_analyze_test(scsp_ram_addr, high_wram_addr, 1, 2, "scsp to wram ", 7);
   do_analyze_test(vdp1_ram_addr, high_wram_addr, 1, 2, "vdp1 to wram ", 7);
#else

   test_direct_dma(high_wram_addr, vdp2_vram_addr, 1, 1, 0, 0x1000, "wram to vdp2");

   //dma_update_test(1, 1, 1, 1);
   //dma_update_test(0, 1, 1, 1);
   //dma_update_test(1, 0, 1, 1);

   //indirect_dma_test();
   //double_dma_test();
#endif
}