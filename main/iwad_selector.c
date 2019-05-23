/* 
 * This file is part of doom-ng-odroid-go.
 * Copyright (c) 2019 ducalex.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_attr.h"

#include "odroid.h"

extern void I_BeginDiskAccess(void);
extern void I_EndDiskAccess(void);
extern const char *I_DoomExeDir(void);

static int selected = 0;
static bool a_pressed = false;
static int files_count = 0;

void input_callback(odroid_input_state state)
{
	if (state.values[ODROID_INPUT_UP])
		if (--selected < 0) selected = 0;
	if (state.values[ODROID_INPUT_DOWN])
		if (++selected >= files_count) selected = files_count - 1;
	if (state.values[ODROID_INPUT_A])
		a_pressed = true;
}

void iwad_selector(char **selected_iwad)
{
	DIR *dir;
	struct dirent *ent;
	char files[16][64];
	char buffer[64];
	
	I_BeginDiskAccess();
	if ((dir = opendir(I_DoomExeDir())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (strcasecmp(ent->d_name, "prboom.wad") != 0 && 
				strcasecmp(&ent->d_name[strlen(ent->d_name)-4], ".wad") == 0) {
				memcpy(files[files_count++], ent->d_name, 64);
			}
		}
		closedir (dir);
	}
	I_EndDiskAccess();

	spi_lcd_fb_setFontColor(2);
	
	if (files_count > 1) {

		odroid_input_set_callback(&input_callback);
		while(1) {
			if (a_pressed) {
				spi_lcd_fb_clear();
				spi_lcd_fb_print(0, 0, "Starting Doom...");
				spi_lcd_fb_flush();
				*selected_iwad = strdup(files[selected]);
				return;
			}

			int row = 0;
			
			spi_lcd_fb_clear();
			spi_lcd_fb_print(0, row++ * 30, "Please select a WAD:");

			for (int i = 0; i < files_count; i++) {
				spi_lcd_fb_print(5, row * 30, selected == i ? ">" : " ");
				spi_lcd_fb_print(30, row++ * 30, files[i]);
			}

			spi_lcd_fb_flush();
			// vTaskDelay
		}
	}
}