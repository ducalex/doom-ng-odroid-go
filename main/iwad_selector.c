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

typedef struct
{
	bool internal;
	bool selected;
	char name[32];
	char path[64];
} wad_t;


void iwad_selector(char *argv[], int *argc)
{
	DIR *dir;
	struct dirent *ent;
	char buffer[64];
	int cursor = 0;

	wad_t wads[16];
	int wads_count = 0;

	I_BeginDiskAccess();
	if ((dir = opendir(I_DoomExeDir())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (strcasecmp(ent->d_name, "prboom.wad") != 0 && 
					strcasecmp(&ent->d_name[strlen(ent->d_name)-4], ".wad") == 0) {
				
				sprintf(&buffer, "%s/%s", I_DoomExeDir(), ent->d_name);

				wad_t *wad = &wads[wads_count++];

				memcpy(wad->name, ent->d_name, 32);
				memcpy(wad->path, buffer, 64);
				
				FILE *fp = fopen(wad->path, "rb");
				fread(&buffer, 4, 1, fp);
				fclose(fp);
				
				wad->internal = (memcmp(buffer, "IWAD", 4) == 0);
				wad->selected = false;
			}
		}
		closedir (dir);
	}
	I_EndDiskAccess();

	spi_lcd_fb_setFontColor(2);
	
	if (wads_count <= 1)
		return;

	while(1) {
		int row = 0;
		
		spi_lcd_fb_clear();
		spi_lcd_fb_print(0, row++ * 30, "Please select a WAD:");

		for (int i = 0; i < wads_count; i++) {
			spi_lcd_fb_print(5, row * 30, cursor == i ? ">" : " ");
			spi_lcd_fb_setFontColor(wads[i].selected ? 3 : 2);
			spi_lcd_fb_print(30, row++ * 30, wads[i].name);
			spi_lcd_fb_setFontColor(2);
		}

		spi_lcd_fb_flush();

		int btn = odroid_input_wait_for_button_press(1000);
		
		if (btn == ODROID_INPUT_UP) {
			if (--cursor < 0) cursor = 0;
		}
		else if (btn == ODROID_INPUT_DOWN) {
			if (++cursor >= wads_count) cursor = wads_count - 1;
		}
		else if (btn == ODROID_INPUT_B || btn == ODROID_INPUT_A) {
			// Btn A always selects, otherwise we toggle
			wads[cursor].selected = (!wads[cursor].selected || btn == ODROID_INPUT_A);

			// Deselect other IWADs, only one can be selected
			if (wads[cursor].selected && wads[cursor].internal) {
				for (int i = 0; i < wads_count; i++) {
					if (i != cursor && wads[i].internal) {
						wads[i].selected = false;
					}
				}
			}
		}
		
		if (btn == ODROID_INPUT_A) {
			spi_lcd_fb_clear();
			spi_lcd_fb_print(0, 0, "Starting Doom...");
			row = 2;
			
			// Add WADs (mods)
			for (int i = 0; i < wads_count; i++) {
				if (wads[i].selected && !wads[i].internal) {
					argv[(*argc)++] = strdup(wads[i].path);
					spi_lcd_fb_printf(15, row++ * 30, "WAD: %s", wads[i].name);
				}
			}
			
			// Add IWAD (game) at the end
			for (int i = 0; i < wads_count; i++) {
				if (wads[i].selected && wads[i].internal) {
					argv[(*argc)++] = strdup("-iwad");
					argv[(*argc)++] = strdup(wads[i].name);
					spi_lcd_fb_printf(15, row++ * 30, "IWAD: %s", wads[i].name);
				}
			}

			spi_lcd_fb_flush();
			return;
		}
	}
}
