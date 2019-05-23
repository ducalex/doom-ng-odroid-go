/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2006 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *
 * DESCRIPTION:
 *  DOOM graphics stuff for SDL
 *
 *-----------------------------------------------------------------------------
 */

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "gamepad.h"
#include "i_video.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "lprintf.h"

#include "rom/ets_sys.h"
#include "odroid.h"

#include "esp_heap_caps.h"

int use_fullscreen = 0;
int use_doublebuffer = 0;
int16_t lcdpal[256];

void I_StartTic(void)
{
	gamepadPoll();
}


static void I_InitInputs(void)
{
  gamepadInit();
}


//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
}


void I_StartFrame (void)
{
}


int I_StartDisplay(void)
{
	spi_lcd_wait_finish();
  return true;
}


void I_EndDisplay(void)
{
}


//
// I_FinishUpdate
//

void I_FinishUpdate (void)
{
	spi_lcd_fb_write(screens[0].data);
	//Flip framebuffers
//	if (scr==screena) screens[0].data=screenb; else screens[0].data=screena;
}


void I_SetPalette (int pal)
{
	int i, v;
	int pplump = W_GetNumForName("PLAYPAL");
	const byte * palette = W_CacheLumpNum(pplump);
	palette+=pal*(3*256);
	for (i=0; i<255 ; i++) {
		v=((palette[0]>>3)<<11)+((palette[1]>>2)<<5)+(palette[2]>>3);
		lcdpal[i]=(v>>8)+(v<<8);
//		lcdpal[i]=v;
		palette += 3;
	}
  spi_lcd_fb_setPalette(lcdpal);
	W_UnlockLumpNum(pplump);
}



void I_PreInitGraphics(void)
{
	lprintf(LO_INFO, "preinitgfx");
}


// CPhipps -
// I_SetRes
// Sets the screen resolution
void I_SetRes(void)
{
  int i;

//  I_CalculateRes(SCREENWIDTH, SCREENHEIGHT);

  // set first three to standard values
  for (i=0; i<3; i++) {
    screens[i].width = SCREENWIDTH;
    screens[i].height = SCREENHEIGHT;
    screens[i].byte_pitch = SCREENPITCH;
    screens[i].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
    screens[i].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
  }

  // statusbar
  screens[4].width = SCREENWIDTH;
  screens[4].height = (ST_SCALED_HEIGHT+1);
  screens[4].byte_pitch = SCREENPITCH;
  screens[4].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
  screens[4].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  static int    firsttime=1;

  if (firsttime)
  {
    firsttime = 0;

    atexit(I_ShutdownGraphics);
    lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

    /* Set the video mode */
    I_UpdateVideoMode();

    /* Initialize the input system */
    I_InitInputs();
  }
}


void I_UpdateVideoMode(void)
{
  lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

  V_InitMode(VID_MODE8);
  V_DestroyUnusedTrueColorPalettes();
  V_FreeScreens();

  I_SetRes();

  V_AllocScreens();

  R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);
}
