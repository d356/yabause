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

#ifndef SCUH
#define SCUH

#define SCUREG_D0R   (*(volatile u32 *)0x25FE0000)
#define SCUREG_D0W   (*(volatile u32 *)0x25FE0004)
#define SCUREG_D0C   (*(volatile u32 *)0x25FE0008)
#define SCUREG_D0AD  (*(volatile u32 *)0x25FE000C)
#define SCUREG_D0EN  (*(volatile u32 *)0x25FE0010)
#define SCUREG_D0MD  (*(volatile u32 *)0x25FE0014)

#define SCUREG_D1R   (*(volatile u32 *)0x25FE0020)
#define SCUREG_D1W   (*(volatile u32 *)0x25FE0024)
#define SCUREG_D1C   (*(volatile u32 *)0x25FE0028)
#define SCUREG_D1AD  (*(volatile u32 *)0x25FE002C)
#define SCUREG_D1EN  (*(volatile u32 *)0x25FE0030)
#define SCUREG_D1MD  (*(volatile u32 *)0x25FE0034)

#define SCUREG_D2R   (*(volatile u32 *)0x25FE0040)
#define SCUREG_D2W   (*(volatile u32 *)0x25FE0044)
#define SCUREG_D2C   (*(volatile u32 *)0x25FE0048)
#define SCUREG_D2AD  (*(volatile u32 *)0x25FE004C)
#define SCUREG_D2EN  (*(volatile u32 *)0x25FE0050)
#define SCUREG_D2MD  (*(volatile u32 *)0x25FE0054)

#define SCUREG_DSTP  (*(volatile u32 *)0x25FE0060)
#define SCUREG_DSTA  (*(volatile u32 *)0x25FE0070)

#define SCUREG_T0C   (*(volatile u32 *)0x25FE0090)
#define SCUREG_T1S   (*(volatile u32 *)0x25FE0094)
#define SCUREG_T1MD  (*(volatile u32 *)0x25FE0098)

#define SCUREG_IMS   (*(volatile u32 *)0x25FE00A0)
#define SCUREG_IST   (*(volatile u32 *)0x25FE00A4)

#define SCUREG_AIACK (*(volatile u32 *)0x25FE00A8)
#define SCUREG_ASR0  (*(volatile u32 *)0x25FE00B0)
#define SCUREG_ASR1  (*(volatile u32 *)0x25FE00B4)

#define SCUREG_RSEL  (*(volatile u32 *)0x25FE00C4)
#define SCUREG_VER   (*(volatile u32 *)0x25FE00C8)

void scu_test();

void test_scu_ver_register();

void test_vblank_in_interrupt();
void test_vblank_out_interrupt();
void test_hblank_in_interrupt();
void test_timer0_interrupt();
void test_timer1_interrupt();
void test_dsp_end_interrupt();
void test_sound_request_interrupt();
void test_smpc_interrupt();
void test_pad_interrupt();
void test_dma0_interrupt();
void test_dma1_interrupt();
void test_dma2_interrupt();
void test_dma_illegal_interrupt();
void test_sprite_draw_end_interrupt();
void test_cd_block_interrupt();

void TestDMA();
void test_dma0();
void test_dma1();
void test_dma2();
void test_dma_misalignment();
void test_indirect_dma();
void test_ist_and_ims();
void test_dsp();
void test_mvi_imm_d();

void scu_register_test(void);
void scu_int_test(void);
void scu_dma_test();
void scu_dsp_test();

#endif
