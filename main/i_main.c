/* 
 *  PrBoom: a Doom port based on BOOM merged with LxDoom and LSDLDoo
 * 
 *  Copyright (C) 1999 by id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 */

#include "config.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <odroid.h>

#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "m_fixed.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "lprintf.h"
#include "m_random.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_main.h"
#include "r_fps.h"
#include "lprintf.h"

//The gamepad uses keyboard emulation, but for compilation, these variables need to be placed
//somewhere. THis is as good a place as any.
int usejoystick = 0;
int joyleft, joyright, joyup, joydown;
int realtic_clock_rate = 100;
static int_64_t time_scale = 1 << 24;

extern void saveAudioSettings();
extern int snd_MasterVolume;

static int I_GetTime_Scaled(void)
{
    return (int)((int_64_t)I_GetTime_RealTime() * time_scale >> 24);
}

static int I_GetTime_FastDemo(void)
{
    static int fasttic;
    return fasttic++;
}

static int I_GetTime_Error(void)
{
    I_Error("I_GetTime_Error: GetTime() used before initialization");
    return 0;
}

int (*I_GetTime)(void) = I_GetTime_Error;

static int key_yes = 'y';
static int key_no = 'n';

static const struct
{
    int ps2mask;
    int *key;
} keymap[] = {
    {1 << ODROID_INPUT_UP, &key_up},
    {1 << ODROID_INPUT_DOWN, &key_down},
    {1 << ODROID_INPUT_LEFT, &key_left},
    {1 << ODROID_INPUT_RIGHT, &key_right},

    {1 << ODROID_INPUT_A, &key_yes},
    {1 << ODROID_INPUT_A, &key_fire},
    {1 << ODROID_INPUT_A, &key_menu_enter},

    {1 << ODROID_INPUT_B, &key_no},
    {1 << ODROID_INPUT_B, &key_speed},
    {1 << ODROID_INPUT_B, &key_strafe},
    {1 << ODROID_INPUT_B, &key_menu_backspace},

    {1 << ODROID_INPUT_MENU, &key_escape},
    {1 << ODROID_INPUT_VOLUME, &key_map},
    {1 << ODROID_INPUT_START, &key_use},
    {1 << ODROID_INPUT_SELECT, &key_weapontoggle},

    {0, NULL},
};

static volatile int joyVal = 0;

#define KEY_TRANSITION_DOWN(key) (gamepad_state.values[key] && !gamepad_state.previous[key])
#define KEY_TRANSITION_UP(key) (!gamepad_state.values[key] && gamepad_state.previous[key])
#define KEY_DOWN(key) (gamepad_state.values[key])
#define KEY_UP(key) (!gamepad_state.values[key])

static void JoystickReadCallback(odroid_input_state gamepad_state)
{
    int result = 0;

    for (int i = 0; i < ODROID_INPUT_MAX; i++)
    {
        result |= (KEY_DOWN(i) ? 0 : 1) << i;
    }

    if (KEY_DOWN(ODROID_INPUT_START))
    {
        if (KEY_TRANSITION_DOWN(ODROID_INPUT_UP))
        { // brightness up
            backlight_percentage_set(backlight_percentage_get() + 25);
            doom_printf("Brightness: %d", backlight_percentage_get());
        }
        if (KEY_TRANSITION_DOWN(ODROID_INPUT_DOWN))
        { // brightness down
            backlight_percentage_set(backlight_percentage_get() - 25);
            doom_printf("Brightness: %d", backlight_percentage_get());
        }
        if (KEY_TRANSITION_DOWN(ODROID_INPUT_RIGHT))
        { // volume up
            if (++snd_MasterVolume > 15)
                snd_MasterVolume = 15;
            saveAudioSettings();
            doom_printf("Volume: %d", snd_MasterVolume);
        }
        if (KEY_TRANSITION_DOWN(ODROID_INPUT_LEFT))
        { // volume down
            if (--snd_MasterVolume < 0)
                snd_MasterVolume = 0;
            saveAudioSettings();
            doom_printf("Volume: %d", snd_MasterVolume);
        }
        result = ~(1 << ODROID_INPUT_START);
    }
    joyVal = result;
}

void I_StartTic(void)
{
    static int oldPollJsVal = 0xffff;
    event_t ev;

    for (int i = 0; keymap[i].key != NULL; i++)
    {
        if ((oldPollJsVal ^ joyVal) & keymap[i].ps2mask)
        {
            ev.type = (joyVal & keymap[i].ps2mask) ? ev_keyup : ev_keydown;
            ev.data1 = *keymap[i].key;
            D_PostEvent(&ev);
        }
    }

    oldPollJsVal = joyVal;
}

void I_Init(void)
{
    /* killough 4/14/98: Adjustable speedup based on realtic_clock_rate */
    if (fastdemo)
        I_GetTime = I_GetTime_FastDemo;
    else if (realtic_clock_rate != 100)
    {
        time_scale = ((int_64_t)realtic_clock_rate << 24) / 100;
        I_GetTime = I_GetTime_Scaled;
    }
    else
        I_GetTime = I_GetTime_RealTime;

    if (!(nomusicparm && nosfxparm))
        I_InitSound();

    R_InitInterpolation();
}

static void PrintVer(void)
{
    char vbuf[200];
    lprintf(LO_INFO, "%s\n", I_GetVersionString(vbuf, 200));
}

/* I_SafeExit
 * This function is called instead of exit() by functions that might be called
 * during the exit process (i.e. after exit() has already been called)
 * Prevent infinitely recursive exits -- killough
 */
void I_SafeExit(int rc)
{
    static int has_exited = 0;
    if (!has_exited) /* If it hasn't exited yet, exit now -- killough */
    {
        has_exited = rc ? 2 : 1;
        exit(rc);
    }
}

int doom_main(int argc, char const *const *argv)
{
    myargc = argc;
    myargv = argv;

    /* Version info */
    lprintf(LO_INFO, "\n");
    PrintVer();

    odroid_input_set_callback(&JoystickReadCallback);

    atexit(Z_Close); /* cph - Z_Close must be done after I_Quit, so we register it first. */
    Z_Init();
    I_SetAffinityMask();
    I_PreInitGraphics();
    D_DoomMain();

    return 0;
}
