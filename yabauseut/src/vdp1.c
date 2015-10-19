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

   SCUREG_T1MD = 0x001;
   SCUREG_T0C = 223;

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

   SCUREG_T1MD = 0x001;
   //SCUREG_T1S = 310;
   SCUREG_T0C = 223;

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
