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
#if 0
//////////////////////////////////////////////////////////////////////////////

volatile int hline_count_this_frame = 0;

void smpc_test_vblank_out_handler()
{
   hline_count_this_frame = 0;
}

//////////////////////////////////////////////////////////////////////////////

volatile int hblank_timer = 0;

void smpc_test_hblank_in_handler()
{
   hblank_timer++;
   hline_count_this_frame++;
}

//////////////////////////////////////////////////////////////////////////////

void print_result(u32 starttime, int*line_count, int nops)
{
   u32 endtime = hblank_timer;
   vdp_printf(&test_disp_font, 0 * 8, *line_count * 8, 15, "%06d   | %03d            | %03d", nops, endtime - starttime, hline_count_this_frame);
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
   u32 starttime = hblank_timer;
   smpc_wait_till_ready();
   print_result(starttime, line_count,  num_nops);
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
   bios_change_scu_interrupt_mask(0xFFFFFFFF, MASK_VBLANKOUT | MASK_HBLANKIN);
   bios_set_scu_interrupt(0x41, smpc_test_vblank_out_handler);
   bios_set_scu_interrupt(0x42, smpc_test_hblank_in_handler);
   bios_change_scu_interrupt_mask(~(MASK_VBLANKOUT | MASK_HBLANKIN), 0);
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
      vdp_wait_vblankin();
      intback_wait(num_nops);
      timed_command_issue(&freq, &line_count, SMPC_CMD_INTBACK, num_nops);
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
#else
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////

volatile int hblank_timer = 0;
volatile int issue_intback = 0;

volatile int line_count = 4;

volatile int lines_since_vblank_out = 0;
volatile int lines_since_vblank_in = 0;

volatile int result_pos = 0;
volatile int system_manager_occured = 0;

struct LineTiming {
   int start_line;
   int finish_line;
};

//struct Results
//{
//   struct LineTiming since_vblank_out;
//   struct LineTiming since_vblank_in;
//   struct LineTiming free_timer;
//} results[25] = { { 0 } };

struct ResultStrings
{
   char str[128];
}result_strings[128] = { { { 0 } } };

int result_str_pos = 0;

//struct Results continue_results = {{0}};

volatile int continue_results_pos = 0;
volatile int test_started = 0;
volatile int frame_count = 0;

void smpc_test_vblank_out_handler()
{
   lines_since_vblank_out = 0;
   frame_count++;
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

volatile int m = 0;
volatile int stored_timer = 0;

void smpc_test_system_manager_handler()
{
   //results[result_pos].since_vblank_out.finish_line = lines_since_vblank_out;
   //results[result_pos].since_vblank_in.finish_line = lines_since_vblank_in;
   //results[result_pos].free_timer.finish_line = hblank_timer;
   //result_pos++;

 //  int free_time = results[which].free_timer.finish_line - results[which].free_timer.start_line;

   int vbo_lines = lines_since_vblank_out;
   int vbi_lines = lines_since_vblank_in;
   int hb = hblank_timer;
   int fc = frame_count;

   int free_time = hb - stored_timer;

   strcpy(result_strings[result_str_pos].str, "sys man");
   result_str_pos++;

   sprintf(result_strings[result_str_pos].str, "%04d  | %04d  | %04d | %04d | %04d",
      vbo_lines,
      vbi_lines,
      hb,
      fc,
      free_time);

   result_str_pos++;

   system_manager_occured = 1;
}

//////////////////////////////////////////////////////////////////////////////
#if 0
void print_result(int which)
{
#if 0
   int free_time = results[which].free_timer.finish_line - results[which].free_timer.start_line;

   vdp_printf(&test_disp_font, 0 * 8, line_count * 8, 15, "%04d  | %04d  | %04d | %04d",
      results[which].since_vblank_out.start_line,
      results[which].since_vblank_in.start_line,
      results[which].free_timer.start_line,
      frame_count);

   line_count++;

   vdp_printf(&test_disp_font, 0 * 8, line_count * 8, 15, "%04d  | %04d  | %04d | %04d",
      results[which].since_vblank_out.finish_line,
      results[which].since_vblank_in.finish_line,
      results[which].free_timer.finish_line,
      free_time);

   line_count+=2;
#else


   int free_time = results[which].free_timer.finish_line - results[which].free_timer.start_line;

   sprintf(result_strings[result_str_pos].str, "%04d  | %04d  | %04d | %04d",
      results[which].since_vblank_out.finish_line,
      results[which].since_vblank_in.finish_line,
      results[which].free_timer.finish_line,
      free_time);

   result_str_pos++;
#endif
}
#endif
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

void do_test_start()
{
   int vbo_lines = lines_since_vblank_out;
   int vbi_lines = lines_since_vblank_in;
   int hb = hblank_timer;
   int fc = frame_count;

   sprintf(result_strings[result_str_pos].str, "%04d  | %04d  | %04d | %04d",
      vbo_lines,
      vbi_lines,
      hb,
      fc);

   stored_timer = hb;

   result_str_pos++;
}

//////////////////////////////////////////////////////////////////////////////

void smpc_intback_issue_timing_test()
{
   disable_iapetus_handler();
   test_disp_font.transparent = 0;

   smpc_intback_set_interrupts();

   int non_peripheral_test = 1;
   int issue_continue_next_frame = 0;

   int printed_results_pos = 0;
   
   for (;;)
   {
      if (issue_intback)
      {
         issue_intback = 0;

         if (non_peripheral_test)
            intback_write_iregs(1, 0, 0, 1, 0);
         else
            intback_write_iregs(0, 0, 0, 1, 0);

         do_test_start();

         //results[result_pos].since_vblank_out.start_line = lines_since_vblank_out;
         //results[result_pos].since_vblank_in.start_line = lines_since_vblank_in;
         //results[result_pos].free_timer.start_line = hblank_timer;

         smpc_issue_command(SMPC_CMD_INTBACK);
      }

      if (system_manager_occured)
      {
         //print_result(printed_results_pos);
         //printed_results_pos++;

       //  if (non_peripheral_test)
         {
            if (SMPC_REG_SR & (1 << 5))
            {
               //remaining peripheral data
               //issue continue
            //   strcpy(result_strings[result_str_pos].str, "dr");
               //sprintf(result_strings[result_str_pos].str, "%s", "dr");
             //  result_str_pos++;
               do_test_start();
               //results[result_pos].since_vblank_out.start_line = lines_since_vblank_out;
               //results[result_pos].since_vblank_in.start_line = lines_since_vblank_in;
               //results[result_pos].free_timer.start_line = hblank_timer;
               SMPC_REG_IREG(0) = 0x80;
               
            }
            else
            {
               //sprintf(result_strings[result_str_pos].str, "%04d",0);

               //result_str_pos++;
       /*        result_strings[result_str_pos].str[0] = 'N';
               result_strings[result_str_pos].str[1] = '\0';*/
             //  sprintf(result_strings[result_str_pos].str, "%s", "nd");
               //strcpy(result_strings[result_str_pos].str, "nd");
             //  printf(result_strings[result_str_pos].str, "%s", "nd");
             //  result_str_pos++;
            }
         }
      }

      if (result_pos > 10)
         break;
   }

   int i;
   for (i = 0; i < 25; i++)
   {
      vdp_printf(&test_disp_font, 0, 8 * i, 15, result_strings[i].str);
   }

   // Re-enable Peripheral Handler
   per_init();

   for (;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_START)
      {
         reset_system();
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
