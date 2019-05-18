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

extern void spi_lcd_fb_flush();
extern void spi_lcd_fb_clear();
extern void spi_lcd_fb_print(int x, int y, char *string);
extern void I_BeginDiskAccess(void);
extern void I_EndDiskAccess(void);
extern const char *I_DoomExeDir(void);


extern volatile int joyVal;

void iwad_selector(char **selected_iwad)
{
	DIR *dir;
	struct dirent *ent;
	char files[16][64];
	char buffer[64];
	int index = 0;

	I_BeginDiskAccess();
	if ((dir = opendir(I_DoomExeDir())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (strcasecmp(ent->d_name, "prboom.wad") != 0 && 
				strcasecmp(&ent->d_name[strlen(ent->d_name)-4], ".wad") == 0) {
				memcpy(files[index++], ent->d_name, 64);
			}
		}
		closedir (dir);
	}
	I_EndDiskAccess();

	if (index > 1) {
		int selected = 0;
		int prevJoyVal = 0;

		while(1) {
			if (joyVal != prevJoyVal) {
				
				// To do: use odroid states instead of mapped keys?
				if ((joyVal & 0x10) == 0) {
					if (--selected < 0) selected = 0;
				} 
				else if ((joyVal & 0x40) == 0) {
					if (++selected >= index) selected = index - 1;
				}
				else if ((joyVal & 0x2000) == 0) {
					spi_lcd_fb_clear();
					spi_lcd_fb_print(0, 0, "Starting Doom...");
					spi_lcd_fb_flush();
					*selected_iwad = strdup(files[selected]);
					return;
				}

				prevJoyVal = joyVal;
				
				int row = 0;
				
				spi_lcd_fb_clear();
				spi_lcd_fb_print(0, row++ * 30, "Please select a WAD:");

				for (int i = 0; i < index; i++) {
					spi_lcd_fb_print(5, row * 30, selected == i ? ">" : " ");
					spi_lcd_fb_print(30, row++ * 30, files[i]);
				}

				spi_lcd_fb_flush();
				
			}
			// vTaskDelay
		}
	}
}