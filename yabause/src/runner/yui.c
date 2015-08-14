/*  Copyright 2003 Guillaume Duhamel
	Copyright 2004-2010 Lawrence Sebald

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

#include <stdio.h>

#include "../yui.h"
#include "../peripheral.h"
#include "../cs0.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vidsoft.h"
#include "../vdp2.h"

#define AUTO_TEST_SELECT_ADDRESS 0x7F000
#define AUTO_TEST_STATUS_ADDRESS 0x7F004
#define AUTO_TEST_OUTPUT_ADDRESS 0x7F008
#define AUTO_TEST_NOT_RUNNING 0
#define AUTO_TEST_BUSY 1
#define AUTO_TEST_FINISHED 2

#define VDP2_VRAM 0x25E00000

SH2Interface_struct *SH2CoreList[] = {
	&SH2Interpreter,
	NULL
};

PerInterface_struct *PERCoreList[] = {
	&PERDummy,
	NULL
};

CDInterface *CDCoreList[] = {
	&DummyCD,
	NULL
};

SoundInterface_struct *SNDCoreList[] = {
	&SNDDummy,
	NULL
};

VideoInterface_struct *VIDCoreList[] = {
	&VIDDummy,
	NULL
};

M68K_struct * M68KCoreList[] = {
	&M68KDummy,
#ifdef HAVE_C68K
	&M68KC68K,
#endif
#ifdef HAVE_Q68
	&M68KQ68,
#endif
	NULL
};

char* text_red = "\033[22;31m";
char* text_green = "\033[22;31m";
char* text_white = "\033[01;37m";

void set_color(char* color)
{
#ifndef _MSC_VER
    printf(color);
#endif
}

static const char *bios = "";
static int emulate_bios = 0;

void YuiErrorMsg(const char *error_text) {
	fprintf(stderr, "Error: %s\n", error_text);
}

void YuiSwapBuffers(void) {
}

void get_test_strings(int *i, char *full_str, char *command)
{
	sprintf(full_str, "%s", Vdp2Ram + AUTO_TEST_OUTPUT_ADDRESS + *i);

	*i = *i + (int)strlen(full_str) + 1;// + 1 because of null char

	sprintf(command, "%s", Vdp2Ram + AUTO_TEST_OUTPUT_ADDRESS + *i);

	*i = *i + (int)strlen(command) + 1;
}

void print_result(char* test_info, char* command,int passed)
{
    set_color(text_white);

    printf("%-32s ", test_info, command);

    if (passed)
    {
        set_color(text_green);
    }
    else
    {
        set_color(text_red);
    }

    printf("%s\n", command);

    set_color(text_white);
}

int main(int argc, char *argv[]) 
{
	yabauseinit_struct yinit = { 0 };
	int current_test = 0;
	int tests_failed = 0;
	int tests_total = 0;
	int tests_passed = 0;

	printf("Running tests...\n\n");

	yinit.percoretype = PERCORE_DUMMY;
	yinit.sh2coretype = SH2CORE_INTERPRETER;
	yinit.vidcoretype = VIDCORE_DUMMY;
	yinit.m68kcoretype = M68KCORE_DUMMY;
	yinit.sndcoretype = SNDCORE_DUMMY;
	yinit.cdcoretype = CDCORE_DUMMY;
	yinit.carttype = CART_NONE;
	yinit.regionid = REGION_AUTODETECT;
	yinit.biospath = emulate_bios ? NULL : bios;
	yinit.cdpath = NULL;
	yinit.buppath = NULL;
	yinit.mpegpath = NULL;
	yinit.cartpath = NULL;
	yinit.frameskip = 0;
	yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
	yinit.clocksync = 0;
	yinit.basetime = 0;
	yinit.skip_load = 1;

next_test:

	if (YabauseInit(&yinit) != 0)
		return -1;

	MappedMemoryLoadExec(argv[1], 0);

	MappedMemoryWriteByte(VDP2_VRAM + AUTO_TEST_SELECT_ADDRESS, current_test);

	for (;;) 
	{
		int status = 0;

		PERCore->HandleEvents();

		status = MappedMemoryReadByte(VDP2_VRAM + AUTO_TEST_STATUS_ADDRESS);

		if (status != AUTO_TEST_NOT_RUNNING &&
			status != AUTO_TEST_BUSY)
		{
			int i = 0;

			for (;;)
			{
				char test_info[256];
				char command[64];

				get_test_strings(&i, &test_info[0], &command[0]);

				if (!strcmp(command, "MESSAGE"))
				{
					//write a message without doing anything else
					printf("%-32s\n", test_info);
				}
				else if (!strcmp(command, "PASS"))
				{
					//sub-test passed, continue to next
                    print_result(&test_info[0], &command[0],1);
					tests_total++;
					tests_passed++;
					continue;
				}
				else if (!strcmp(command, "FAIL"))
				{
					//sub-test failure
                    print_result(&test_info[0], &command[0],0);
					tests_total++;
					tests_failed++;
					continue;
				}
				else if (!strcmp(command, "NEXT"))
				{
					//all sub-tests finished, proceed to next main test
					printf("\n");
					current_test++;
					YabauseDeInit();
					goto next_test;
				}
				else if (!strcmp(command, "QUIT"))
				{
                    printf("%d/%d tests passed.", tests_passed, tests_total);
					goto end;
				}
			}
		}
	}

end:

	return tests_failed;
}