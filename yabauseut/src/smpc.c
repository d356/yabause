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

#include <stdio.h>
#include <string.h>
#include "tests.h"
#include "smpc.h"

u8 per_fetch_continue();

//////////////////////////////////////////////////////////////////////////////

void smpc_test()
{
   int choice;

   menu_item_struct smpc_menu[] = {
   { "SMPC commands" , &smpc_cmd_test, },
   { "SMPC command timing", &smpc_cmd_timing_test, },
//   { "Keyboard test" , &kbd_test, },
//   { "Mouse target test" , &mouse_test, },
//   { "Stunner target test" , &stunner_test, },
   { "Peripheral test" , &per_test, },
//   { "Misc" , &misc_smpc_test, },
   { "\0", NULL }
   };

   for (;;)
   {
      choice = gui_do_menu(smpc_menu, &test_disp_font, 0, 0, "SMPC Tests", MTYPE_CENTER, -1);
      if (choice == -1)
         break;
   }   
}

//////////////////////////////////////////////////////////////////////////////

void disable_iapetus_handler()
{
   bios_change_scu_interrupt_mask(0xFFFFFFF, MASK_VBLANKOUT | MASK_SYSTEMMANAGER);
   bios_set_scu_interrupt(0x41, NULL);
   bios_set_scu_interrupt(0x47, NULL);
}
#if 1
//////////////////////////////////////////////////////////////////////////////

volatile int hline_count_this_frame = 0;
volatile int hline_count_since_vblank_in = 0;
volatile int total_hlines = 0;
volatile int vblank_in_occurred = 0;

void smpc_test_vblank_out_handler()
{
   hline_count_this_frame = 0;
}

void smpc_test_vblank_in_handler()
{
   vblank_in_occurred = 1;
   hline_count_since_vblank_in = 0;
}

//////////////////////////////////////////////////////////////////////////////

void smpc_test_hblank_in_handler()
{
   if((hline_count_this_frame % 16) == 0)
      VDP2_REG_COAR = 0xff;
   else
      VDP2_REG_COAR = 0;

   total_hlines++;
   hline_count_this_frame++;
   hline_count_since_vblank_in++;
}

//////////////////////////////////////////////////////////////////////////////

void print_result(int starttime, int*line_count, int nops, int hlines_since_vblank_in, int hlines_this_frame)
{
   int endtime = total_hlines;
   vdp_printf(&test_disp_font, 0 * 8, *line_count * 8, 15, "%06d %04d %04d %04d %05d %05d", nops, endtime - starttime, hlines_this_frame, hlines_since_vblank_in, starttime, endtime);
   *line_count = *line_count + 1;
}

//////////////////////////////////////////////////////////////////////////////

void intback_write_iregs(int status, int p2md, int p1md, int pen, int ope)
{
   SMPC_REG_IREG(0) = status;
   SMPC_REG_IREG(1) = (p2md << 6) | (p1md << 4) | (pen << 3) | (ope << 0);
   SMPC_REG_IREG(2) = 0xF0;
}

//////////////////////////////////////////////////////////////////////////////

void timed_command_issue(u32*freq,int*line_count, int command, int num_nops)
{
   smpc_issue_command(command);
   int starttime = total_hlines;
   smpc_wait_till_ready();
   int current_hline = hline_count_this_frame;
   int hline_since_vblank_in = hline_count_since_vblank_in;
   print_result(starttime, line_count, num_nops, hline_since_vblank_in, current_hline);
}

//////////////////////////////////////////////////////////////////////////////

//not exact since the for loop will cost cycles
void intback_wait(int delay)
{
   int i = 0;
   for (i = 0; i < delay; i++)
      asm("nop");
}

//////////////////////////////////////////////////////////////////////////////

void smpc_intback_set_interrupts()
{
   interrupt_set_level_mask(0xF);
   bios_change_scu_interrupt_mask(0xFFFFFFFF, MASK_VBLANKOUT | MASK_HBLANKIN | MASK_VBLANKIN);
   bios_set_scu_interrupt(0x40, smpc_test_vblank_in_handler);
   bios_set_scu_interrupt(0x41, smpc_test_vblank_out_handler);
   bios_set_scu_interrupt(0x42, smpc_test_hblank_in_handler);
   bios_change_scu_interrupt_mask(~(MASK_VBLANKOUT | MASK_HBLANKIN | MASK_VBLANKIN), 0);
   interrupt_set_level_mask(0x1);
}

//////////////////////////////////////////////////////////////////////////////

void smpc_intback_issue_timing_test()
{
   disable_iapetus_handler();
   test_disp_font.transparent = 0;

   u32 freq;

   smpc_intback_set_interrupts();

   vdp_vsync();

   VDP2_REG_CLOFEN = 0x7f;

   //28.6364 mhz = 28.64 cycles per microsecond
   //26.8741 mhz = 26.87 cycles per microsecond
   //intback is supposed to be issued 300 microseconds after vblankin
   //300 * 28.64 = 8592 cycles
   //300 * 26.87 = 8061 cycles
   int nop_increment = 8061 / 2;
   
   vdp_printf(&test_disp_font, 0, 0, 15,   "Smpc intback command issue timing test");
   vdp_printf(&test_disp_font, 0, 8, 15,   "--------------------------------------");
   vdp_printf(&test_disp_font, 0, 16, 15,  "Nops     | Hblanks until  | Issued at ");
   vdp_printf(&test_disp_font, 0, 24, 15,  "         | complete       | hblank #  ");
   vdp_printf(&test_disp_font, 0, 32, 15,  "--------------------------------------");
   int line_count = 5;
   int i;
   for (i = 0; i < 27; i++)
   {
      int num_nops = nop_increment * i;
      intback_write_iregs(0, 0, 0, 1, 0);
      //vdp_wait_vblankin();

      while (!vblank_in_occurred){}
      vblank_in_occurred = 0;

      intback_wait(num_nops);
      timed_command_issue(&freq, &line_count, SMPC_CMD_INTBACK, num_nops);
   }

   // Re-enable Peripheral Handler
//   per_init();

   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_START)
      {
         reset_system();
        // ar_menu();
      }
   }

   test_disp_font.transparent = 1;
   gui_clear_scr(&test_disp_font);
}






///////////////////////
///////////////////////
///////////////////////
///////////////////////
///////////////////////

void hblank_wait(int num)
{
   int i;
   for (i = 0; i < num; i++)
   {
      vdp_wait_hblankout();
   }
}

volatile int hblanks_until_smpc_finished = 0;

void smpc_wait_hblank()
{
   // Wait until SF register is cleared
   while (SMPC_REG_SF & 0x1) 
   {
      vdp_wait_hblankin();
      hblanks_until_smpc_finished++;
   }
}

void print_result2(u32 num_hblanks_before_issue, int*line_count)
{
   vdp_printf(&test_disp_font, 0 * 8, *line_count * 8, 15, "%06d   | %03d            ", num_hblanks_before_issue, hblanks_until_smpc_finished);
   *line_count = *line_count + 1;
}

void timed_command_issue2(int*line_count, int command, int num_hblanks_before_issue)
{
   smpc_issue_command(command);
   smpc_wait_hblank();
   print_result2(num_hblanks_before_issue, line_count);
}

void smpc_intback_issue_timing_test2()
{
   disable_iapetus_handler();
   test_disp_font.transparent = 0;

   //u32 freq;

   smpc_intback_set_interrupts();

   vdp_vsync();

   //28.6364 mhz = 28.64 cycles per microsecond
   //26.8741 mhz = 26.87 cycles per microsecond
   //intback is supposed to be issued 300 microseconds after vblankin
   //300 * 28.64 = 8592 cycles
   //300 * 26.87 = 8061 cycles
   //int nop_increment = 1;// 8061 / 2;

   vdp_printf(&test_disp_font, 0, 0, 15,  "Smpc intback command issue timing test");
   vdp_printf(&test_disp_font, 0, 8, 15,  "--------------------------------------");
   vdp_printf(&test_disp_font, 0, 16, 15, "Issued # Hblanks after | Hblanks until");
   vdp_printf(&test_disp_font, 0, 24, 15, "vblank in              | completed    ");
   vdp_printf(&test_disp_font, 0, 32, 15, "--------------------------------------");
   int line_count = 5;
   int i;
   for (i = 0; i < 27; i++)
   {
      //int num_nops = nop_increment * i;
      intback_write_iregs(0, 0, 0, 1, 0);
      vdp_wait_vblankin();
      hblank_wait(i);
      hblanks_until_smpc_finished = 0;
      timed_command_issue2(&line_count, SMPC_CMD_INTBACK, i);
   }

   // Re-enable Peripheral Handler
  // per_init();

   for (;;)
   {
      vdp_vsync();
      //int j = 0;
      //for (i = 0; i < 40; i++)
      //{
      //   //vdp_wait_hblankin();
      //   while (((VDP2_REG_TVSTAT >> 2) & 1) == 0) {}

      //   VDP2_REG_COAR = j;

      //   if ((i & 1) == 0)
      //   {
      //      j=0xff;
      //   }
      //   else
      //   {
      //      j = 0;
      //   }
      //}

      if (per[0].but_push_once & PAD_START)
      {
         reset_system();
      }
   }

   test_disp_font.transparent = 1;
   gui_clear_scr(&test_disp_font);
}
#else
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////

volatile int hblank_timer = 0;
volatile int issue_intback = 0;

volatile int line_count = 5;

volatile int lines_since_vblank_out = 0;
volatile int lines_since_vblank_in = 0;

volatile int result_pos = 0;

struct Results
{
   volatile int issue_line;
   volatile int finish_line;

   volatile int start_time;
   volatile int end_time;

} volatile results[25] = { 0 };

void smpc_test_vblank_out_handler()
{
   lines_since_vblank_out = 0;
//   results[result_pos].start_time = 0xdead;
//   result_pos++;
}

void smpc_test_hblank_in_handler()
{
   hblank_timer++;
   lines_since_vblank_out++;
   lines_since_vblank_in++;
}

void smpc_test_vblank_in_handler()
{
   lines_since_vblank_in = 0;
   issue_intback = 1;
}

//////////////////////////////////////////////////////////////////////////////

void print_result(int which)
{
   if (results[which].start_time == 0xdead)
   {
      vdp_printf(&test_disp_font, 0 * 8, line_count * 8, 15, "Vblank-out");

   }
   else
   {
      int time = results[which].end_time - results[which].start_time;
      vdp_printf(&test_disp_font, 0 * 8, line_count * 8, 15, "%04d  | %04d  | %03d  | %03d | %03d",
         results[which].start_time, results[which].end_time, time, results[which].issue_line, results[which].finish_line);
   }
   line_count++;
}

//////////////////////////////////////////////////////////////////////////////

void intback_write_iregs(int status, int p2md, int p1md, int pen, int ope)
{
   SMPC_REG_IREG(0) = status;
   SMPC_REG_IREG(1) = (p2md << 6) | (p1md << 4) | (pen << 3) | (ope << 0);
   SMPC_REG_IREG(2) = 0xF0;
   SMPC_REG_IREG(6) = 0xFE;
}

//////////////////////////////////////////////////////////////////////////////

//not exact since the for loop will cost cycles
void intback_wait(int delay)
{
   int i = 0;
   for (i = 0; i < delay; i++)
      asm("nop");
}



void smpc_test_system_manager_handler()
{
   results[result_pos].end_time = hblank_timer;
   results[result_pos].finish_line = lines_since_vblank_out;
   result_pos++;
}

//////////////////////////////////////////////////////////////////////////////

void smpc_intback_set_interrupts()
{
   interrupt_set_level_mask(0xF);
   bios_change_scu_interrupt_mask(0xFFFFFFFF, MASK_VBLANKOUT | MASK_HBLANKIN | MASK_VBLANKIN | MASK_SYSTEMMANAGER);
   bios_set_scu_interrupt(0x47, smpc_test_system_manager_handler);
   bios_set_scu_interrupt(0x40, smpc_test_vblank_in_handler);
   bios_set_scu_interrupt(0x41, smpc_test_vblank_out_handler);
   bios_set_scu_interrupt(0x42, smpc_test_hblank_in_handler);
   bios_change_scu_interrupt_mask(~(MASK_VBLANKOUT | MASK_HBLANKIN | MASK_VBLANKIN | MASK_SYSTEMMANAGER), 0);
   interrupt_set_level_mask(0x1);
}

//////////////////////////////////////////////////////////////////////////////

void smpc_intback_issue_timing_test()
{
   disable_iapetus_handler();
   test_disp_font.transparent = 0;

   smpc_intback_set_interrupts();

//   vdp_vsync();

   //28.6364 mhz = 28.64 cycles per microsecond
   //26.8741 mhz = 26.87 cycles per microsecond
   //intback is supposed to be issued 300 microseconds after vblankin
   //300 * 28.64 = 8592 cycles
   //300 * 26.87 = 8061 cycles
   //int nop_increment = 8061;

   vdp_printf(&test_disp_font, 0, 0, 15, "Smpc intback command issue timing test");
   vdp_printf(&test_disp_font, 0, 8, 15, "--------------------------------------");
   vdp_printf(&test_disp_font, 0, 16, 15, "Nops     | Hblanks until  | Issued at ");
   vdp_printf(&test_disp_font, 0, 24, 15, "         | complete       | hblank #  ");
   vdp_printf(&test_disp_font, 0, 32, 15, "--------------------------------------");

   
   int count = 224;
   for (;;)
   {
      if (issue_intback)
      {
         int lines = lines_since_vblank_out;

         if (lines >= count)
         {
            issue_intback = 0;
            results[result_pos].start_time = hblank_timer;
            results[result_pos].issue_line = lines;
            intback_write_iregs(0, 0, 0, 1, 0);
            smpc_issue_command(SMPC_CMD_INTBACK);
            count++;
         }
      }

      if (result_pos > 20)
         break;
#if 0
      if (issue_intback)
      { 
         issue_intback = 0;
         int num_nops = nop_increment * i;
       //  intback_wait(num_nops);
         timed_command_issue(&line_count, SMPC_CMD_INTBACK, num_nops);
         i++;
      }

      if (i > 26)
         break;
#endif
   }

   int i;
   for (i = 0; i < 25; i++)
   {
      print_result(i);
   }

   // Re-enable Peripheral Handler
   per_init();

   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_START)
      {
         ar_menu();
      }
   }

   test_disp_font.transparent = 1;
   gui_clear_scr(&test_disp_font);
}
#endif

void smpc_cmd_test2()
{
   int i, j, k;
   int a = 0, b = 0, c = 0;
again:

   // Disable Peripheral handler
   disable_iapetus_handler();
   test_disp_font.transparent = 0;



   //   SMPC_REG_IREG(0) = 0x40;
   SMPC_REG_IREG(0) = a;
   SMPC_REG_IREG(1) = b;
   SMPC_REG_IREG(2) = c;

   smpc_issue_command(0x1e);
   smpc_wait_till_ready();

   

 //  for (i = 9; i > 0; i--)
//   {
      for (j = 0; j < 16; j++)
         vdp_printf(&test_disp_font, 2 * 8, (6 + j) * 8, 15, "OREG%02d = %08X", j, SMPC_REG_OREG(j));
      for (j = 0; j < 16; j++)
         vdp_printf(&test_disp_font, 20 * 8, (6 + j) * 8, 15, "OREG%d = %08X", 16 + j, SMPC_REG_OREG(16 + j));

   //   vdp_printf(&test_disp_font, 18 * 8, (6 + 17) * 8, 15, "%d", i);
         
//

      // Re-enable Peripheral Handler
      per_init();
   for (;;)
   {
      vdp_vsync();
      vdp_printf(&test_disp_font, 0 * 8, 0 * 8, 15, "a = %02X b = %02X c = %02X", a, b, c);

      if (per[0].but_push_once & PAD_START)
      {
         ar_menu();
      }

      if (per[0].but_push_once & PAD_A)
      {
         goto again;
      }

      if (per[0].but_push_once & PAD_X)
      {
         a++;
      }
      if (per[0].but_push_once & PAD_Y)
      {
         b++;
      }
      if (per[0].but_push_once & PAD_Z)
      {
         c++;
      }
   }



   test_disp_font.transparent = 1;
   gui_clear_scr(&test_disp_font);
}
void smpc_cmd_test()
{
	// Intback IREG test
//   unregister_all_tests();
//   register_test(&SmpcSNDONTest, "SNDOFF");
//   register_test(&SmpcSNDOFFTest, "SNDON");
//   register_test(&SmpcCDOFFTest, "CDOFF");
//   register_test(&SmpcCDONTest, "CDON");
//   register_test(&SmpcNETLINKOFFTest, "NETLINKOFF");
//   register_test(&SmpcNETLINKONTest, "NETLINKON");
//   register_test(&SmpcINTBACKTest, "INTBACK");
//   register_test(&SmpcSETTIMETest, "SETTIME");
//   register_test(&SmpcSETSMEMTest, "SETSMEM");
//   register_test(&SmpcNMIREQTest, "NMIREQ");
//   register_test(&SmpcRESENABTest, "RESENAB");
//   register_test(&SmpcRESDISATest, "RESDISA");
//   DoTests("SMPC Command tests", 0, 0);

   int i, j, k;

   // Disable Peripheral handler
   disable_iapetus_handler();
   test_disp_font.transparent = 0;
   
   vdp_printf(&test_disp_font, 2 * 8, 2 * 16, 15, "Starting test in X second(s)");
   
   for (i = 5; i > 0; i--)
   {
      vdp_printf(&test_disp_font, 19 * 8, 2 * 16, 15, "%d", i);
   	  
      for (j = 0; j < 60; j++)
         vdp_vsync();
   }     

//   SMPC_REG_IREG(0) = 0x40;
   SMPC_REG_IREG(0) = 0xFF;
   SMPC_REG_IREG(1) = 0x08;
   SMPC_REG_IREG(2) = 0xF0;
   
   smpc_issue_command(SMPC_CMD_INTBACK);
   smpc_wait_till_ready();

   // Display OREGs

   // Delay for a bit
   vdp_printf(&test_disp_font, 2 * 8, (6+17) * 8, 15, "Finishing up in X second(s)");
   
   for (i = 9; i > 0; i--)
   {
      for (j = 0; j < 16; j++)
         vdp_printf(&test_disp_font, 2 * 8, (6+j) * 8, 15, "OREG%d = %08X", j, SMPC_REG_OREG(j));
      for (j = 0; j < 16; j++)
         vdp_printf(&test_disp_font, 20 * 8, (6+j) * 8, 15, "OREG%d = %08X", 16+j, SMPC_REG_OREG(16+j));
      
   	vdp_printf(&test_disp_font, 18 * 8, (6+17) * 8, 15, "%d", i);
   	  
      for (k = 0; k < 60; k++)
         vdp_vsync();
   }

   // Re-enable Peripheral Handler
   per_init();

   test_disp_font.transparent = 1;
   gui_clear_scr(&test_disp_font);
}

//////////////////////////////////////////////////////////////////////////////

void smpc_cmd_timing_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void pad_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void kbd_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void mouse_test()
{
}

//////////////////////////////////////////////////////////////////////////////

void analogtest()
{
}

//////////////////////////////////////////////////////////////////////////////

void stunner_test()
{
}

//////////////////////////////////////////////////////////////////////////////

char *pad_string[16] = {
"",
"",
"",
"L",
"Z",
"Y",
"X",
"R",
"B",
"C",
"A",
"START",
"UP",
"DOWN",
"LEFT",
"RIGHT"
};

//////////////////////////////////////////////////////////////////////////////

u8 per_fetch_continue()
{
   // Issue a continue
   SMPC_REG_IREG(0) = 0x80;

   // Wait till command is finished
   while(SMPC_REG_SF & 0x1) {}
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u8 get_oreg(u8 *oreg_counter)
{
   u8 ret;

   if (oreg_counter[0] >= 32)
      oreg_counter[0] = per_fetch_continue();

   ret = SMPC_REG_OREG(oreg_counter[0]);
   oreg_counter[0]++;
   return ret;
}

//////////////////////////////////////////////////////////////////////////////

int end_test;

u8 disp_pad_data(u8 oreg_counter, int x, int y, u8 id)
{
   int i;
   u16 data;

   data = get_oreg(&oreg_counter) << 8;
   data |= get_oreg(&oreg_counter);

   if (!(data & 0x0F00))
      end_test = 1;

   for (i = 0; i < 16; i++)
   {
      if (!(data & (1 << i)))
      {        
         vdp_printf(&test_disp_font, x, y, 0xF, "%s ", pad_string[i]);
         x += (strlen(pad_string[i])+1) * 8;
      }
   }

   for (i = 0; i < ((352 - x) >> 3); i++)
      vdp_printf(&test_disp_font, x+(i << 3), y, 0xF, " ");

   return oreg_counter;
}

//////////////////////////////////////////////////////////////////////////////

u8 disp_nothing(u8 oreg_counter, int x, int y, u8 id)
{
   int i;
   for (i = 0; i < ((352 - x) >> 3); i++)
      vdp_printf(&test_disp_font, x+(i << 3), y, 0xF, " ");

   if (id == 0xFF)
      return oreg_counter;
   else
      return (oreg_counter+(id & 0xF));
}

//////////////////////////////////////////////////////////////////////////////

typedef struct
{
  u8 id;
  char *name;
  u8 (*disp_data)(u8 oreg_counter, int xoffset, int yoffset, u8 id);
} per_info_struct;

per_info_struct per_info[] =
{
   { 0x02, "Standard Pad", disp_pad_data },
   { 0x13, "Racing Wheel", disp_nothing },
   { 0x15, "Analog Pad", disp_nothing },
   { 0x23, "Saturn Mouse", disp_nothing },
   { 0x25, "Gun", disp_nothing },
   { 0x34, "Keyboard", disp_nothing },
   { 0xE1, "Megadrive 3 button", disp_nothing },
   { 0xE2, "Megadrive 6 button", disp_nothing },
   { 0xE3, "Shuttle Mouse", disp_nothing },
   { 0xFF, "Nothing", disp_nothing }
};

//////////////////////////////////////////////////////////////////////////////

u8 per_parse(char *port_name, int y_offset, u8 oreg_counter)
{
   int i;
   u8 perid = get_oreg(&oreg_counter);

   for (i = 0; i < (sizeof(per_info) / sizeof(per_info_struct)); i++)
   {
      if (perid == per_info[i].id)
      {
         char text[50];

         sprintf(text, "Port %s: %s", port_name, per_info[i].name);
         vdp_printf(&test_disp_font, 0 * 8, y_offset, 0xF, "%s ", text);

         // Display peripheral data
         oreg_counter = per_info[i].disp_data(oreg_counter, (strlen(text)+1) * 8, y_offset, perid);
         return oreg_counter;
      }
   }

   vdp_printf(&test_disp_font, 0 * 8, y_offset, 0xF, "Port %s: Unknown(%02X)", port_name, perid);
   return oreg_counter;
}

//////////////////////////////////////////////////////////////////////////////

void per_test()
{
   interrupt_set_level_mask(0xF);

   init_iapetus(RES_352x240);
   vdp_rbg0_init(&test_disp_settings);
   vdp_set_default_palette();

   // Display On
   vdp_disp_on();

   // Disable Lapetus's peripheral handler
   bios_change_scu_interrupt_mask(0xFFFFFFF, MASK_VBLANKOUT | MASK_SYSTEMMANAGER);

   test_disp_font.transparent = 0;

   vdp_printf(&test_disp_font, 0 * 8, 1 * 8, 0xF, "Peripheral test");
   vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "(to exit, press A+B+C+START on any pad)");

   end_test = 0;

   for (;;)
   {
      unsigned char oreg_counter=0;
      unsigned char portcon;
      int i, i2;

      vdp_vsync();

      // Issue intback command
      smpc_wait_till_ready();
      SMPC_REG_IREG(0) = 0x00; // no intback status
      SMPC_REG_IREG(1) = 0x0A; // 15-byte mode, peripheral data returned, time optimized
      SMPC_REG_IREG(2) = 0xF0; // ???
      smpc_issue_command(SMPC_CMD_INTBACK);

      // Wait till command is finished
      while(SMPC_REG_SF & 0x1) {}

#if 0
      for (i = 0; i < 16; i++)
        vdp_printf(&test_disp_font, 0 * 8, (4+i) * 8, 0xF, "OREG %02d: %02X", i, SMPC_REG_OREG(i));

      for (i = 0; i < 16; i++)
        vdp_printf(&test_disp_font, 16 * 8, (4+i) * 8, 0xF, "OREG %02d: %02X", i+16, SMPC_REG_OREG(i+16));
#endif

      // Grab oreg's
      for (i = 0; i < 2; i++)
      {
         portcon = get_oreg(&oreg_counter);

         if (portcon != 0x16)
         {
            if (portcon == 0xF1)
            {
               char text[3];
               sprintf(text, "%d", i+1);
               oreg_counter = per_parse(text, (4+(8*i)) * 8, oreg_counter);
            }
            else if (portcon == 0xF0)
               vdp_printf(&test_disp_font, 0 * 8, (4+(8*i)) * 8, 0xF, "Port %d: Nothing                   ", i+1);

            for (i2 = 0; i2 < 6; i2++)
               vdp_printf(&test_disp_font, 0 * 8, (4+(8*i)+1+i2) * 8, 0xF, "                           ");
         }
         else
         {
            vdp_printf(&test_disp_font, 0 * 8, (4+(8*i)) * 8, 0xF, "Port %d: Multi-tap                 ", i+1);

            for (i2 = 0; i2 < (portcon & 0x0F); i2++)
            {
               char text[3];
               sprintf(text, "%d%c", i+1, 'A' + (char)i2);
               oreg_counter = per_parse(text, (4+(8*i)+1+i2) * 8, oreg_counter);
            }
         }
      }

      if (end_test)
         break;
   }


   test_disp_font.transparent = 1;

   // Re-enable Lapetus's peripheral handler
   bios_change_scu_interrupt_mask(~(MASK_VBLANKOUT | MASK_SYSTEMMANAGER), 0);
}

//////////////////////////////////////////////////////////////////////////////

void misc_smpc_test()
{
}

//////////////////////////////////////////////////////////////////////////////
