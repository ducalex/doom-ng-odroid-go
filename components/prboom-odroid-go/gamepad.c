// Copyright 2016-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdlib.h>

#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "d_event.h"
#include "g_game.h"
#include "d_main.h"
#include "m_cheat.h"
#include "doomstat.h"
#include "lprintf.h"
#include "gamepad.h"
#include "odroid.h"

extern void saveAudioSettings();
extern int snd_MasterVolume;

//The gamepad uses keyboard emulation, but for compilation, these variables need to be placed
//somewhere. THis is as good a place as any.
int usejoystick=0;
int joyleft, joyright, joyup, joydown;

typedef struct {
	int ps2mask;
	int *key;
} JsKeyMap;

static char *key_yes = 'y';
static char *key_no = 'n';

static const JsKeyMap keymap[]={
	{1 << ODROID_INPUT_UP,    &key_up},
	{1 << ODROID_INPUT_DOWN,  &key_down},
	{1 << ODROID_INPUT_LEFT,  &key_left},
	{1 << ODROID_INPUT_RIGHT, &key_right},

	{1 << ODROID_INPUT_A, &key_yes},
	{1 << ODROID_INPUT_A, &key_fire},
	{1 << ODROID_INPUT_A, &key_menu_enter},

	{1 << ODROID_INPUT_B, &key_no},
	{1 << ODROID_INPUT_B, &key_speed},
	{1 << ODROID_INPUT_B, &key_strafe},
	{1 << ODROID_INPUT_B, &key_menu_backspace},

	{1 << ODROID_INPUT_MENU,   &key_escape},
	{1 << ODROID_INPUT_VOLUME, &key_map},
	{1 << ODROID_INPUT_START,  &key_use},
	{1 << ODROID_INPUT_SELECT, &key_weapontoggle},

	{0, NULL},
};

static int cheatCurrentLevel = 1;
static volatile int joyVal = 0;


static void ApplyCheat(char *code)
{
	for (int i = 0; i < strlen(code); i++){
		M_FindCheats(code[i]);
	}
}


static void JoystickReadCallback(odroid_input_state gamepad_state)
{
	int result = 0;

	for (int i = 0; i < ODROID_INPUT_MAX; i++) {
		result |= (gamepad_state.values[i] ? 0 : 1) << i;
	}

	if (gamepad_state.values[ODROID_INPUT_START]) {
		if (gamepad_state.values[ODROID_INPUT_UP] && !gamepad_state.previous[ODROID_INPUT_UP]) { // brightness up
			backlight_percentage_set(backlight_percentage_get() + 25);
			doom_printf("Brightness: %d", backlight_percentage_get());
		}
		if (gamepad_state.values[ODROID_INPUT_DOWN] && !gamepad_state.previous[ODROID_INPUT_DOWN]) { // brightness down
			backlight_percentage_set(backlight_percentage_get() - 25);
			doom_printf("Brightness: %d", backlight_percentage_get());
		}
		if (gamepad_state.values[ODROID_INPUT_RIGHT] && !gamepad_state.previous[ODROID_INPUT_RIGHT]) { // volume up
			if (++snd_MasterVolume > 15) snd_MasterVolume = 15;
			saveAudioSettings();
			doom_printf("Volume: %d", snd_MasterVolume);
		}
		if (gamepad_state.values[ODROID_INPUT_LEFT] && !gamepad_state.previous[ODROID_INPUT_LEFT]) { // volume down
			if (--snd_MasterVolume < 0) snd_MasterVolume = 0;
			saveAudioSettings();
			doom_printf("Volume: %d", snd_MasterVolume);
		}
		result = ~(1 << ODROID_INPUT_START);
	}

	if (gamepad_state.values[ODROID_INPUT_MENU]) {
		if (gamepad_state.values[ODROID_INPUT_UP] && !gamepad_state.previous[ODROID_INPUT_UP]) {
			// Invulnerability
			ApplyCheat("iddqd");
		}
		if (gamepad_state.values[ODROID_INPUT_DOWN] && !gamepad_state.previous[ODROID_INPUT_DOWN]) {
			//Full megaarmor protection (200%), all weapons, full ammo, and all the keys
			ApplyCheat("idkfa");
		}
		if (gamepad_state.values[ODROID_INPUT_LEFT] && !gamepad_state.previous[ODROID_INPUT_LEFT]) {
			//No clipping
			ApplyCheat("idclip");
		}
		if (gamepad_state.values[ODROID_INPUT_RIGHT] && !gamepad_state.previous[ODROID_INPUT_RIGHT]) {
			//200% health
			ApplyCheat("idbeholdh");
		}
		result = ~(1 << ODROID_INPUT_MENU);
	}

	if (gamepad_state.values[ODROID_INPUT_VOLUME]) {
		if (gamepad_state.values[ODROID_INPUT_UP] && !gamepad_state.previous[ODROID_INPUT_UP]) {
			//Temporary light
			ApplyCheat("idbeholdl");
		}
		if (gamepad_state.values[ODROID_INPUT_DOWN] && !gamepad_state.previous[ODROID_INPUT_DOWN]) {
			//Temporary berserk
			ApplyCheat("idbeholds");
		}
		if (gamepad_state.values[ODROID_INPUT_LEFT] && !gamepad_state.previous[ODROID_INPUT_LEFT]) {
			//Temporary invisibility
			ApplyCheat("idbeholdi");
		}
		if (gamepad_state.values[ODROID_INPUT_RIGHT] && !gamepad_state.previous[ODROID_INPUT_RIGHT]) {
			//Warp to different levels
			char code[9];
			if(cheatCurrentLevel == 1 && gamemission == doom){
				cheatCurrentLevel = 11; //Doom idclev codes are in the form E#M#
			}
			lprintf(LO_INFO, "cheatCurrentLevel = %02d\n", cheatCurrentLevel);
			doom_printf("Cheat level %02d", cheatCurrentLevel);

			sprintf(code, "idclev%02d", cheatCurrentLevel);
			lprintf(LO_INFO, "idclev code: %s\n", code);
			ApplyCheat(code);
			cheatCurrentLevel++;
			if(cheatCurrentLevel > 49){
				cheatCurrentLevel = 1; 
			}
		}
		result = ~(1 << ODROID_INPUT_VOLUME);
	}

	joyVal = result;
}


void gamepadPoll(void)
{
	static int oldPollJsVal = 0xffff;
	event_t ev;

	for (int i = 0; keymap[i].key != NULL; i++) {
		if ((oldPollJsVal^joyVal) & keymap[i].ps2mask) {
			ev.type=(joyVal & keymap[i].ps2mask) ? ev_keyup : ev_keydown;
			ev.data1 = *keymap[i].key;
			D_PostEvent(&ev);
		}
	}

	oldPollJsVal = joyVal;
}


void gamepadInit(void)
{
	odroid_input_init();
	odroid_input_set_callback(&JoystickReadCallback);
}
