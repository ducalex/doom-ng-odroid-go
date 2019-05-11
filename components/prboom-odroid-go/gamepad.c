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
#include "gamepad.h"
#include "lprintf.h"
#include "spi_lcd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "driver/adc.h"

//The gamepad uses keyboard emulation, but for compilation, these variables need to be placed
//somewhere. THis is as good a place as any.
int usejoystick=0;
int joyleft, joyright, joyup, joydown;
static odroid_gamepad_state previousJoystickState;

//atomic, for communication between joy thread and main game thread
volatile int joyVal = 0;

typedef struct {
	int ps2mask;
	int *key;
} JsKeyMap;

//Mappings from PS2 buttons to keys
static const JsKeyMap keymap[]={
	{0x10, &key_up},
	{0x40, &key_down},
	{0x80, &key_left},
	{0x20, &key_right},

	{0x4000, &key_use},				//start
	{0x2000, &key_fire},			//circle
	{0x2000, &key_menu_enter},		//circle
	{0x8000, &key_pause},			//square
	{0x1000, &key_weapontoggle},	//triangle

	{0x8, &key_escape},				//menu
	{0x1, &key_map},				//select

	{0x400, &key_strafeleft},		//L1
	{0x100, &key_speed},			//L2
	{0x800, &key_straferight},		//R1
	{0x200, &key_strafe},			//R2

	{0, NULL},
};

int cheatCurrentLevel = 1;
extern int snd_volume;


static volatile odroid_gamepad_state gamepad_state;
static odroid_gamepad_state previous_gamepad_state;
static uint8_t debounce[ODROID_INPUT_MAX];
static volatile bool input_gamepad_initialized = false;
static SemaphoreHandle_t xSemaphore;


void ApplyCheat(char *code)
{
	for (int i = 0; i < strlen(code); i++){
		M_FindCheats(code[i]);
	}
}


int JoystickRead()
{
	if (!input_gamepad_initialized) return 0;

	xSemaphoreTake(xSemaphore, portMAX_DELAY);
	odroid_gamepad_state state = gamepad_state;
	xSemaphoreGive(xSemaphore);

	int result = 0;

	if (!state.values[ODROID_INPUT_UP])
			result |= 0x10; //key_up

	if (!state.values[ODROID_INPUT_DOWN])
			result |= 0x40; //key_down

	if (!state.values[ODROID_INPUT_LEFT])
			result |= 0x80; //key_left

	if (!state.values[ODROID_INPUT_RIGHT])
			result |= 0x20; //key_right

	if (!state.values[ODROID_INPUT_A])
			result |= 0x2000; //key_fire, key_menu_enter

	if (!state.values[ODROID_INPUT_START])
			result |= 0x4000; //key_use

	if (!state.values[ODROID_INPUT_MENU])
			result |= 0x8; //key_escape

	if (!state.values[ODROID_INPUT_SELECT])
			result |= 0x1000; //key_weapontoggle

	if (!state.values[ODROID_INPUT_VOLUME])
			result |= 0x1; //key_map

	if (!state.values[ODROID_INPUT_B]){
			result |= 0x200; //key_strafe
			result |= 0x100; //key_run
	}

	if (state.values[ODROID_INPUT_START]) {
		if (state.values[ODROID_INPUT_UP] && !previousJoystickState.values[ODROID_INPUT_UP]) { // brightness up
			backlight_percentage_set(backlight_percentage_get() + 25);
			doom_printf("Brightness: %d", backlight_percentage_get());
		}
		if (state.values[ODROID_INPUT_DOWN] && !previousJoystickState.values[ODROID_INPUT_DOWN]) { // brightness down
			backlight_percentage_set(backlight_percentage_get() - 25);
			doom_printf("Brightness: %d", backlight_percentage_get());
		}
		if (state.values[ODROID_INPUT_RIGHT] && !previousJoystickState.values[ODROID_INPUT_RIGHT]) { // volume up
			if (++snd_volume > 15) snd_volume = 15;
			doom_printf("Volume: %d", snd_volume);
		}
		if (state.values[ODROID_INPUT_LEFT] && !previousJoystickState.values[ODROID_INPUT_LEFT]) { // volume down
			if (--snd_volume < 0) snd_volume = 0;
			doom_printf("Volume: %d", snd_volume);
		}
		result |= (0x10 | 0x20 | 0x40 | 0x80); // cancel arrow keys
	}

	if (state.values[ODROID_INPUT_MENU]) {
		if (state.values[ODROID_INPUT_UP] && !previousJoystickState.values[ODROID_INPUT_UP]) {
			// Invulnerability
			ApplyCheat("iddqd");
		}
		if (state.values[ODROID_INPUT_DOWN] && !previousJoystickState.values[ODROID_INPUT_DOWN]) {
			//Full megaarmor protection (200%), all weapons, full ammo, and all the keys
			ApplyCheat("idkfa");
		}
		if (state.values[ODROID_INPUT_LEFT] && !previousJoystickState.values[ODROID_INPUT_LEFT]) {
			//No clipping
			ApplyCheat("idclip");
		}
		if (state.values[ODROID_INPUT_RIGHT] && !previousJoystickState.values[ODROID_INPUT_RIGHT]) {
			//200% health
			ApplyCheat("idbeholdh");
		}
		result |= (0x10 | 0x20 | 0x40 | 0x80); // cancel arrow keys
	}

	if (state.values[ODROID_INPUT_VOLUME]) {
		if (state.values[ODROID_INPUT_UP] && !previousJoystickState.values[ODROID_INPUT_UP]) {
			//Temporary light
			ApplyCheat("idbeholdl");
		}
		if (state.values[ODROID_INPUT_DOWN] && !previousJoystickState.values[ODROID_INPUT_DOWN]) {
			//Temporary berserk
			ApplyCheat("idbeholds");
		}
		if (state.values[ODROID_INPUT_LEFT] && !previousJoystickState.values[ODROID_INPUT_LEFT]) {
			//Temporary invisibility
			ApplyCheat("idbeholdi");
		}
		if (state.values[ODROID_INPUT_RIGHT] && !previousJoystickState.values[ODROID_INPUT_RIGHT]) {
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
		result |= (0x10 | 0x20 | 0x40 | 0x80); // cancel arrow keys
	}

	previousJoystickState = state;
	return result;
}

odroid_gamepad_state odroid_input_read_raw()
{
    odroid_gamepad_state state = {0};

    int joyX = adc1_get_raw(ODROID_GAMEPAD_IO_X);
    int joyY = adc1_get_raw(ODROID_GAMEPAD_IO_Y);

    if (joyX > 2048 + 1024)
    {
        state.values[ODROID_INPUT_LEFT] = 1;
        state.values[ODROID_INPUT_RIGHT] = 0;
    }
    else if (joyX > 1024)
    {
        state.values[ODROID_INPUT_LEFT] = 0;
        state.values[ODROID_INPUT_RIGHT] = 1;
    }
    else
    {
        state.values[ODROID_INPUT_LEFT] = 0;
        state.values[ODROID_INPUT_RIGHT] = 0;
    }

    if (joyY > 2048 + 1024)
    {
        state.values[ODROID_INPUT_UP] = 1;
        state.values[ODROID_INPUT_DOWN] = 0;
    }
    else if (joyY > 1024)
    {
        state.values[ODROID_INPUT_UP] = 0;
        state.values[ODROID_INPUT_DOWN] = 1;
    }
    else
    {
        state.values[ODROID_INPUT_UP] = 0;
        state.values[ODROID_INPUT_DOWN] = 0;
    }

    state.values[ODROID_INPUT_SELECT] = !(gpio_get_level(ODROID_GAMEPAD_IO_SELECT));
    state.values[ODROID_INPUT_START] = !(gpio_get_level(ODROID_GAMEPAD_IO_START));

    state.values[ODROID_INPUT_A] = !(gpio_get_level(ODROID_GAMEPAD_IO_A));
    state.values[ODROID_INPUT_B] = !(gpio_get_level(ODROID_GAMEPAD_IO_B));

    state.values[ODROID_INPUT_MENU] = !(gpio_get_level(ODROID_GAMEPAD_IO_MENU));
    state.values[ODROID_INPUT_VOLUME] = !(gpio_get_level(ODROID_GAMEPAD_IO_VOLUME));

    return state;
}

static void odroid_input_task(void *arg)
{
    // Initialize state
    for(int i = 0; i < ODROID_INPUT_MAX; ++i)
    {
        debounce[i] = 0xff;
    }

    while(1)
    {
        // Shift current values
        for(int i = 0; i < ODROID_INPUT_MAX; ++i)
		{
			debounce[i] <<= 1;
		}

        // Read hardware
        odroid_gamepad_state state = odroid_input_read_raw();

        // Debounce
        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        for(int i = 0; i < ODROID_INPUT_MAX; ++i)
		{
            debounce[i] |= state.values[i] ? 1 : 0;
            uint8_t val = debounce[i] & 0x03; //0x0f;
            switch (val) {
                case 0x00:
                    gamepad_state.values[i] = 0;
                    break;

                case 0x03: //0x0f:
                    gamepad_state.values[i] = 1;
                    break;

                default:
                    // ignore
                    break;
            }
		}

        previous_gamepad_state = gamepad_state;

		joyVal = JoystickRead();

        xSemaphoreGive(xSemaphore);

        // delay
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
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
	if (input_gamepad_initialized) {
		lprintf(LO_INFO, "gamepadInit: Game pad already Initialized!\n");
		return;
	}

	lprintf(LO_INFO, "gamepadInit: Initializing game pad.\n");

    xSemaphore = xSemaphoreCreateMutex();

    if (xSemaphore == NULL)
    {
        printf("xSemaphoreCreateMutex failed.\n");
        abort();
    }
	
	gpio_set_direction(ODROID_GAMEPAD_IO_SELECT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_SELECT, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_START, GPIO_MODE_INPUT);

	gpio_set_direction(ODROID_GAMEPAD_IO_A, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_A, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_GAMEPAD_IO_B, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_B, GPIO_PULLUP_ONLY);

	adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ODROID_GAMEPAD_IO_X, ADC_ATTEN_11db);
	adc1_config_channel_atten(ODROID_GAMEPAD_IO_Y, ADC_ATTEN_11db);

	rtc_gpio_deinit(ODROID_GAMEPAD_IO_MENU);
	gpio_set_direction(ODROID_GAMEPAD_IO_MENU, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_MENU, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_VOLUME, GPIO_MODE_INPUT);

	input_gamepad_initialized = true;

    // Start background polling
    xTaskCreatePinnedToCore(&odroid_input_task, "odroid_input_task", 1024 * 2, NULL, 6, NULL, 1);
}
